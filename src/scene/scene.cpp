#include "scene.h"

#include <string>
#include <filesystem>
#include <fstream>

#include <Eigen/Core>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#ifdef _WIN32
    #include <Windows.h>
#endif
#include <glad/glad.h>
#include <nlohmann/json.hpp>

#include "../utils/kinetic_state.h"
#include "../utils/logger.h"
#include "../utils/json_serialize.hpp"

namespace fs = std::filesystem;
using Eigen::Matrix4f;
using Eigen::Vector3f;
using std::chrono::steady_clock;
using std::make_unique;
using std::size_t;
using std::string;
using std::string_view;
using time_point = std::chrono::time_point<std::chrono::steady_clock>;
using duration   = std::chrono::duration<float>;
using std::chrono::duration_cast;
using std::unique_ptr;
using namespace std::chrono_literals;
using nlohmann::json;

Vector3f Scene::initial_camera_pos(5.0f, 5.0f, 5.0f);
Vector3f Scene::initial_camera_target(0.0f, 0.0f, 0.0f);

Scene::Scene() :
    selected_object(nullptr),
    main_camera(
        Vector3f(1.0f, 2.0f, 3.0f), Vector3f(0.0f, 0.0f, 0.0f), 0.1f, 1000.0f, 45.0f, 0.75f
    ),
    camera(initial_camera_pos, initial_camera_target), during_animation(false)
{
    arrows.name = "Scene Arrows";
    logger      = get_logger("Scene");
}

bool Scene::import_model(const string& file_path)
{
    fs::path path(file_path);
    string   group_name = path.stem().generic_string();
    groups.push_back(make_unique<Group>(group_name));
    Group& group   = *(groups.back());
    bool   success = group.load_models(file_path);
    if (!success) {
        logger->warn("fail to import the specified file into current scene");
        groups.erase(groups.end() - 1);
        return false;
    }
    logger->debug("group \"{}\" has beed added into the current scene", group_name);
    return true;
}

bool Scene::save(const string_view directory)
{
    logger->info("saving scene data to {}", directory);
    json     metadata = json::object();
    fs::path base_path(directory);
    if (!fs::exists(base_path)) {
        logger->info(
            "{} does not exist, a directory will be created first", base_path.generic_string()
        );
        fs::create_directory(base_path);
    }
    if (!fs::is_directory(base_path)) {
        logger->error(
            "failed to save scene data because {} is not a directory", base_path.generic_string()
        );
        return false;
    }

    logger->info("saving cameras and lights...");
    // the main (view) camera
    metadata["main_camera"] = main_camera;

    // camera
    metadata["camera"] = camera;

    // lights
    metadata["lights"] = json::array();
    for (const Light& light: lights) {
        metadata["lights"].push_back(light);
    }

    // groups (as external files and extra json)
    logger->info("saving groups...");
    metadata["groups"]    = json::array();
    size_t n_saved_groups = 0ULL;
    for (size_t i = 0; i < groups.size(); i++) {
        auto&  group          = groups[i];
        string group_filename = std::format("{:02d}_{}.obj", i, group->name);
        // save mesh and material to external obj file
        bool result = group->save_models((base_path / group_filename).generic_string());
        if (result)
            ++n_saved_groups;
        // record other attributes in json
        json group_metadata = group->dump_metadata();
        metadata["groups"].push_back({
            {"filename", group_filename},
            {"metadata", group_metadata},
        });
    }

    // write metadata to file
    logger->info("writing metadata file...");
    fs::path      metadata_path = base_path / metadata_filename;
    std::ofstream metadata_file(metadata_path);
    metadata_file << std::setw(4) << metadata;
    logger->info(
        "scene data saved, {}/{} group(s) exported as model file(s)", n_saved_groups, groups.size()
    );

    return true;
}

bool Scene::load(const string_view directory)
{
    logger->info("loading scene from {}", directory);
    fs::path base_path(directory);
    if (!fs::is_directory(base_path)) {
        logger->error("folder does not exist or path is a file");
        return false;
    }
    logger->info("reading metadata...");
    fs::path      metadata_path = base_path / "metadata.json";
    std::ifstream metadata_file(metadata_path);
    json          metadata;
    metadata_file >> metadata;
    logger->info("clearing the current scene...");
    this->clear();

    try {
        // load camera and lights
        logger->info("loading cameras and lights...");
        metadata.at("main_camera").get_to(main_camera);
        metadata.at("camera").get_to(camera);

        for (const json& light_info: metadata.at("lights")) {
            Light light(Vector3f(0, 0, 0), 0);
            light_info.get_to(light);
            lights.push_back(light);
        }

        // load groups
        logger->info("loading groups...");
        size_t n_groups_in_metadata = 0ULL;
        for (const json& group_info: metadata.at("groups")) {
            ++n_groups_in_metadata;
            string group_filename = group_info.at("filename");
            json   group_metadata = group_info.at("metadata");
            if (import_model((base_path / group_filename).generic_string())) {
                unique_ptr<Group>& imported_group = groups.back();
                imported_group->load_metadata(group_metadata);
            } else {
                logger->warn("failed to import file {} as a group", group_filename);
            }
        }
        logger->info(
            "scene loaded, {}/{} group(s) imported", this->groups.size(), n_groups_in_metadata
        );
    } catch (std::exception const& e) {
        logger->error("failed to load scene because: {}", e.what());
        this->clear();
        logger->info("clear scene to avoid data corruption");
        return false;
    }

    return true;
}

void Scene::clear()
{
    groups.clear();
    selected_object = nullptr;
    camera          = Camera(initial_camera_pos, initial_camera_target);
    lights.clear();
    halfedge_mesh    = nullptr;
    during_animation = false;
    all_objects.clear();
}

void Scene::start_simulation()
{
    if (during_animation) {
        return;
    }
    all_objects.clear();
    for (const auto& group: groups) {
        for (const auto& object: group->objects) {
            object->backup     = {object->center, object->velocity, object->force / object->mass};
            object->prev_state = object->backup;
            all_objects.push_back(object.get());
        }
    }
    during_animation = true;
    last_update      = steady_clock::now();
}

void Scene::stop_simulation()
{
    during_animation = false;
}

void Scene::reset_simulation()
{
    if (during_animation) {
        stop_simulation();
    }
    for (auto& group: groups) {
        for (auto& object: group->objects) {
            object->center   = object->backup.position;
            object->velocity = object->backup.velocity;
        }
    }
}

bool Scene::check_during_simulation()
{
    return during_animation;
}

void Scene::simulation_update()
{
    // 这次模拟的总时长不是上一帧的时长，而是上一帧时长与之前帧剩余时长的总和，
    // 即上次调用 simulation_update 到现在过了多久。

    // 以固定的时间步长 (time_step) 循环模拟物体运动，每模拟一步，模拟总时长就减去一个
    // time_step ，当总时长不够一个 time_step 时停止模拟。

    // 根据刚才模拟时间步的数量，更新最后一次调用 simulation_update 的时间 (last_update)。
}
