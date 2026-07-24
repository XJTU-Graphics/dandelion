#include "scene.h"

#include <cmath>
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

#include "../platform/gl.hpp"
#include "../utils/math.hpp"
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
    camera(initial_camera_pos, initial_camera_target), during_animation(false),
    arrows("Scene arrows", GL::Mesh::highlight_wireframe_color)
{
    logger = get_logger("Scene");
}

void Scene::render_ground(const Shader& shader)
{
    static constexpr float           far_distance = 1e3;
    static constexpr float           baseline_gap = 1.0f;
    static bool                      initialized  = false;
    static size_t                    n_baselines  = 1'000;
    static GL::VertexArrayObject     baselines;
    static GL::ArrayBuffer<float, 3> vertices(GL_STATIC_DRAW, vertex_position_location);
    static GL::ArrayBuffer<float, 3> colors(GL_STATIC_DRAW, vertex_color_location);

    if (!initialized) {
        // x axis
        vertices.append(0.0f, 0.0f, 0.0f); // x+ highlighted as a red line
        vertices.append(far_distance, 0.0f, 0.0f);
        colors.append(RGB_COLOR(226, 53, 79));
        colors.append(RGB_COLOR(226, 53, 79));
        vertices.append(0.0f, 0.0f, 0.0f); // x- rendered as a normal line;
        vertices.append(-far_distance, 0.0f, 0.0f);
        colors.append(RGB_COLOR(68, 68, 68));
        colors.append(RGB_COLOR(68, 68, 68));
        // y axis
        vertices.append(0.0f, 0.0f, 0.0f); // y+ highlighted as a green line
        vertices.append(0.0f, far_distance, 0.0f);
        colors.append(RGB_COLOR(131, 204, 6));
        colors.append(RGB_COLOR(131, 204, 6));
        // z axis
        vertices.append(0.0f, 0.0f, 0.0f); // z+ highlighted as a blue line
        vertices.append(0.0f, 0.0f, far_distance);
        colors.append(RGB_COLOR(43, 134, 232));
        colors.append(RGB_COLOR(43, 134, 232));
        vertices.append(0.0f, 0.0f, 0.0f); // z- rendered as a normal line
        vertices.append(0.0f, 0.0f, -far_distance);
        colors.append(RGB_COLOR(68, 68, 68));
        colors.append(RGB_COLOR(68, 68, 68));
        for (size_t i = 1; i <= n_baselines; i++) {
            vertices.append(-far_distance, 0.0f, -baseline_gap * i);
            vertices.append(far_distance, 0.0f, -baseline_gap * i);
            vertices.append(-far_distance, 0.0f, baseline_gap * i);
            vertices.append(far_distance, 0.0f, baseline_gap * i);
            vertices.append(-baseline_gap * i, 0.0f, -far_distance);
            vertices.append(-baseline_gap * i, 0.0f, far_distance);
            vertices.append(baseline_gap * i, 0.0f, -far_distance);
            vertices.append(baseline_gap * i, 0.0f, far_distance);
            for (size_t j = 0; j < 8; j++) {
                colors.append(RGB_COLOR(68, 68, 68));
            }
        }
        baselines.bind();
        vertices.to_gpu();
        colors.to_gpu();
        glDisableVertexAttribArray(vertex_normal_location);
        initialized = true;
    }
    shader.set_uniform("model", I4f);
    shader.set_uniform("color_per_vertex", true);
    baselines.draw(GL_LINES, 0, vertices.count());
    baselines.release();
    shader.set_uniform("color_per_vertex", false);
}

void Scene::render_camera(const Shader& shader)
{
    static GL::VertexArrayObject     camera_vao;
    static GL::ArrayBuffer<float, 3> camera_vertices(GL_DYNAMIC_DRAW, vertex_position_location);
    static GL::ElementArrayBuffer<2> camera_edges(GL_DYNAMIC_DRAW);
    static bool                      initialized = false;
    if (!initialized) {
        camera_edges.append(0u, 1u);
        camera_edges.append(0u, 2u);
        camera_edges.append(0u, 3u);
        camera_edges.append(0u, 4u);
        camera_edges.append(0u, 5u);
        camera_edges.append(1u, 2u);
        camera_edges.append(2u, 3u);
        camera_edges.append(3u, 4u);
        camera_edges.append(4u, 1u);
        camera_vao.bind();
        camera_vertices.bind();
        glDisableVertexAttribArray(vertex_color_location);
        glDisableVertexAttribArray(vertex_normal_location);
        camera_edges.to_gpu();
        camera_vao.release();
        initialized = true;
    }
    float far_plane       = camera.far_plane;
    float tan_half_fov_y  = std::tan(0.5f * radians(camera.fov_y_degrees));
    float half_height     = camera.far_plane * tan_half_fov_y;
    float half_width      = half_height * camera.aspect_ratio;
    float target_distance = (camera.target - camera.position).norm();
    camera_vao.bind();
    camera_vertices.data.clear();
    camera_vertices.append(0.0f, 0.0f, 0.0f);
    camera_vertices.append(-half_width, half_height, -far_plane);
    camera_vertices.append(half_width, half_height, -far_plane);
    camera_vertices.append(half_width, -half_height, -far_plane);
    camera_vertices.append(-half_width, -half_height, -far_plane);
    camera_vertices.append(0.0f, 0.0f, -target_distance);
    camera_vertices.to_gpu();
    shader.set_uniform("model", (Matrix4f)(camera.view().inverse()));
    shader.set_uniform("color_per_vertex", false);
    shader.set_uniform("use_global_color", true);
    shader.set_uniform("global_color", GL::Mesh::default_wireframe_color);
    glDrawArrays(GL_POINTS, 0, GLsizei(camera_vertices.count()));
    glDrawElements(GL_LINES, GLsizei(camera_edges.data.size()), GL_UNSIGNED_INT, (void*)0);
    camera_vao.release();
}

void Scene::render_lights(const Shader& shader)
{
    static GL::VertexArrayObject     light_vao;
    static GL::ArrayBuffer<float, 3> light_vertices(GL_DYNAMIC_DRAW, vertex_position_location);
    static bool                      initialized = false;
    if (!initialized) {
        light_vertices.append(0.0f, 0.0f, 0.0f);
        light_vertices.append(0.1f, 0.0f, 0.0f);
        light_vertices.append(-0.1f, 0.0f, 0.0f);
        light_vertices.append(0.0f, 0.1f, 0.0f);
        light_vertices.append(0.0f, -0.1f, 0.0f);
        light_vertices.append(0.0f, 0.0f, 0.1f);
        light_vertices.append(0.0f, 0.0f, -0.1f);
        light_vao.bind();
        light_vertices.bind();
        light_vertices.specify_vertex_attribute();
        light_vertices.to_gpu();
        glDisableVertexAttribArray(vertex_color_location);
        glDisableVertexAttribArray(vertex_normal_location);
        light_vao.release();
        initialized = true;
    }
    light_vao.bind();
    shader.set_uniform("color_per_vertex", false);
    shader.set_uniform("use_global_color", true);
    shader.set_uniform("global_color", GL::Mesh::default_wireframe_color);
    for (auto& light: lights) {
        Matrix4f model          = Matrix4f::Identity();
        model.block<3, 1>(0, 3) = light.position;
        shader.set_uniform("model", model);
        glDrawArrays(GL_POINTS, 0, GLsizei(light_vertices.count()));
    }
    light_vao.release();
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

void Scene::render(const Shader& shader, WorkingMode mode)
{
    shader.set_uniform("color_per_vertex", false);
    shader.set_uniform("global_color", GL::Mesh::default_wireframe_color);
    Scene::render_ground(shader);
    if (mode != WorkingMode::MODEL && halfedge_mesh) {
        logger->info("the halfedge mesh is destructed.");
        halfedge_mesh.reset(nullptr);
        logger->info("re-build BVH for the edited object");
        selected_object->rebuild_BVH();
        logger->info(
            "The BVH structure of {} (ID: {}) has {} boxes", selected_object->name,
            selected_object->id, selected_object->bvh->count_nodes(selected_object->bvh->root)
        );
    }
    for (auto& group: groups) {
        for (auto& object: group->objects) {
            bool selected = selected_object != nullptr && object.get() == selected_object;
            if (mode == WorkingMode::MODEL && selected && !halfedge_mesh) {
                logger->debug("construct a halfedge mesh for object {}", object->name);
                halfedge_mesh = make_unique<HalfedgeMesh>(*object);
                if (halfedge_mesh->error_info.has_value()) {
                    logger->warn("failed to build a halfedge mesh for the current object");
                }
            }
            // Only render the selected object for Model mode.
            if (mode != WorkingMode::MODEL || selected) {
                object->render(shader, mode, selected);
            }
        }
    }
    if (mode == WorkingMode::MODEL && halfedge_mesh && !halfedge_mesh->error_info.has_value()) {
        halfedge_mesh->sync();
        halfedge_mesh->render(shader);
    }
    if (mode == WorkingMode::RENDER) {
        render_camera(shader);
        render_lights(shader);
    }
    if (mode == WorkingMode::SIMULATE) {
        if (during_animation) {
            simulation_update();
        }
        if (selected_object != nullptr) {
            arrows.clear();
            arrows.add_arrow(
                selected_object->center, selected_object->center + selected_object->velocity
            );
            arrows.to_gpu();
            glDisable(GL_DEPTH_TEST);
            shader.set_uniform("model", I4f);
            arrows.render(shader);
            glEnable(GL_DEPTH_TEST);
        }
    }
}

void Scene::simulation_update()
{
    // 这次模拟的总时长不是上一帧的时长，而是上一帧时长与之前帧剩余时长的总和，
    // 即上次调用 simulation_update 到现在过了多久。

    // 以固定的时间步长 (time_step) 循环模拟物体运动，每模拟一步，模拟总时长就减去一个
    // time_step ，当总时长不够一个 time_step 时停止模拟。

    // 根据刚才模拟时间步的数量，更新最后一次调用 simulation_update 的时间 (last_update)。
}
