#include "group.h"

#include <set>
#include <utility>
#include <fstream>

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <Eigen/Core>
#include <fmt/format.h>

#include "../utils/logger.h"
#include "../utils/json_serialize.hpp"

using Eigen::Vector3f;
using std::make_pair;
using std::make_unique;
using std::pair;
using std::set;
using std::size_t;
using std::string;
using std::unique_ptr;

size_t Group::next_available_id = 0;

Group::Group(const string& group_name) : name(group_name)
{
    this->id = next_available_id;
    ++next_available_id;
    const string logger_name = fmt::format("{} (Group ID: {})", name, id);
    logger                   = get_logger(logger_name);
}

bool Group::load(const string& file_path)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(
        file_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate
                       | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals
                       | aiProcess_DropNormals
    );
    if (scene == nullptr)
        return false;

    logger->info("file {} loaded", file_path);
    size_t n_meshes = scene->mNumMeshes;
    logger->info("{} mesh(es) in total", n_meshes);
    if (n_meshes == 0) {
        logger->warn("the file specified does not contain any mesh");
        return false;
    }

    objects.reserve(n_meshes);
    logger->info("load into group \"{}\"", this->name);
    for (size_t mesh_id = 0; mesh_id < n_meshes; ++mesh_id) {
        const aiMesh*             mesh       = scene->mMeshes[mesh_id];
        const aiFace*             faces      = mesh->mFaces;
        const aiVector3D*         vertices   = mesh->mVertices;
        const aiVector3D*         normals    = mesh->mNormals;
        size_t                    n_vertices = mesh->mNumVertices;
        size_t                    n_faces    = mesh->mNumFaces;
        string                    name(mesh->mName.C_Str());
        set<pair<size_t, size_t>> edges;
        logger->info("the {}-th mesh has {} faces", mesh_id + 1, n_faces);

        objects.push_back(make_unique<Object>(name));
        Object&                    object          = *(objects.back());
        GL::ArrayBuffer<float, 3>& object_vertices = object.mesh.vertices;
        GL::ArrayBuffer<float, 3>& object_normals  = object.mesh.normals;
        GL::ElementArrayBuffer<2>& object_edges    = object.mesh.edges;
        GL::ElementArrayBuffer<3>& object_faces    = object.mesh.faces;
        logger->info("load mesh (object) {}", object.name);
        // Load vertices and normals into the object's GL::Mesh.
        for (size_t vertex_id = 0; vertex_id < n_vertices; ++vertex_id) {
            const aiVector3D& vertex = vertices[vertex_id];
            const aiVector3D& normal = normals[vertex_id];
            object_vertices.append(vertex.x, vertex.y, vertex.z);
            object_normals.append(normal.x, normal.y, normal.z);
        }
        // Load faces into the object's GL::Mesh.
        for (size_t face_id = 0; face_id < n_faces; ++face_id) {
            size_t vertices_per_face = faces[face_id].mNumIndices;
            for (size_t current_vertex = 0; current_vertex < vertices_per_face; ++current_vertex) {
                size_t vertex_id = faces[face_id].mIndices[current_vertex];
                object_faces.data.push_back(static_cast<unsigned int>(vertex_id));

                size_t next_vertex_id =
                    faces[face_id].mIndices[(current_vertex + 1) % vertices_per_face];
                size_t id_less    = std::min(vertex_id, next_vertex_id);
                size_t id_greater = std::max(vertex_id, next_vertex_id);
                edges.insert(make_pair(id_less, id_greater));
            }
        }
        // Load edges into the object's GL::Mesh.
        for (const auto& edge: edges) {
            object_edges.append((unsigned int)edge.first, (unsigned int)edge.second);
        }
        // Load material if it exists.
        static const string default_material_name("DefaultMaterial");
        const aiMaterial*   material = scene->mMaterials[mesh->mMaterialIndex];
        string              material_name(material->GetName().C_Str());
        // Assimp's default material should not be used.
        if (material_name != default_material_name) {
            GL::Material& object_material = object.mesh.material;
            aiColor3D     color;
            float         shininess;
            if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
                object_material.ambient = Vector3f(color.r, color.g, color.b);
            }
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                object_material.diffuse = Vector3f(color.r, color.g, color.b);
            }
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
                object_material.specular = Vector3f(color.r, color.g, color.b);
            }
            if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                object_material.shininess = shininess;
            }
        }
        logger->info(
            "summary: {} vertices, {} edges, {} faces", mesh->mNumVertices, edges.size(),
            object.mesh.faces.count()
        );
        object.rebuild_BVH();
        logger->info(
            "The BVH structure of {} (ID: {}) has {} boxes", object.name, object.id,
            object.bvh->count_nodes(object.bvh->root)
        );
        object.modified = true;
    }

    return true;
}

bool Group::save(const string& file_path)
{
    size_t num_objects = objects.size();
    logger->info("saving group with {} objects to file {}", num_objects, file_path);

    // reconstruct aiScene with Group data
    aiScene scene               = aiScene();
    scene.mRootNode             = new aiNode();
    scene.mRootNode->mName      = aiString(this->name);
    scene.mRootNode->mNumMeshes = num_objects;
    scene.mRootNode->mMeshes    = new unsigned int[num_objects];
    // add mesh indices to root node
    for (size_t i = 0; i < num_objects; ++i) {
        scene.mRootNode->mMeshes[i] = i;
    }

    // create a new material for each object
    scene.mNumMaterials = num_objects;
    scene.mMaterials    = new aiMaterial*[num_objects];
    for (size_t i = 0; i < num_objects; ++i) {
        const auto&         object      = objects[i];
        const GL::Material& material    = object->mesh.material;
        auto*               ai_material = scene.mMaterials[i] = new aiMaterial();
        ai_material->AddProperty(new aiString(object->name), AI_MATKEY_NAME);
        ai_material->AddProperty(
            new aiColor3D{material.ambient.x(), material.ambient.y(), material.ambient.z()}, 1,
            AI_MATKEY_COLOR_AMBIENT
        );
        ai_material->AddProperty(
            new aiColor3D{material.diffuse.x(), material.diffuse.y(), material.diffuse.z()}, 1,
            AI_MATKEY_COLOR_DIFFUSE
        );
        ai_material->AddProperty(
            new aiColor3D{material.specular.x(), material.specular.y(), material.specular.z()}, 1,
            AI_MATKEY_COLOR_SPECULAR
        );
        ai_material->AddProperty(new double(material.shininess), 1, AI_MATKEY_SHININESS);
    }

    // create a mesh for each object
    scene.mNumMeshes = num_objects;
    scene.mMeshes    = new aiMesh*[num_objects];
    for (size_t i = 0; i < num_objects; ++i) {
        const auto& object  = objects[i];
        auto*       ai_mesh = scene.mMeshes[i] = new aiMesh();
        ai_mesh->mName                         = aiString(object->name);
        ai_mesh->mMaterialIndex                = i;

        // transfer vertices and normals from GL::Mesh to aiMesh
        size_t num_vertices = object->mesh.vertices.count();
        size_t num_normals  = object->mesh.normals.count();
        logger->debug("transfering {} vertices and {} normals", num_vertices, num_normals);
        if (num_vertices != num_normals) {
            logger->error("number of vertices and normals are not equal");
            return false;
        }
        ai_mesh->mNumVertices = num_vertices;
        ai_mesh->mVertices    = new aiVector3D[num_vertices];
        ai_mesh->mNormals     = new aiVector3D[num_vertices];
        for (size_t vertex_id = 0; vertex_id < num_vertices; ++vertex_id) {
            auto vertex                   = object->mesh.vertex(vertex_id);
            ai_mesh->mVertices[vertex_id] = {vertex.x(), vertex.y(), vertex.z()};
            auto normal                   = object->mesh.normal(vertex_id);
            ai_mesh->mNormals[vertex_id]  = {normal.x(), normal.y(), normal.z()};
        }

        // transfer faces (detecting and transfering loose edges is not necessary)
        size_t num_faces = object->mesh.faces.count();
        logger->debug("transfering {} faces", num_faces);
        ai_mesh->mNumFaces = num_faces;
        ai_mesh->mFaces    = new aiFace[num_faces];
        for (size_t face_id = 0; face_id < num_faces; ++face_id) {
            auto face                            = object->mesh.face(face_id);
            ai_mesh->mFaces[face_id].mNumIndices = 3;
            ai_mesh->mFaces[face_id].mIndices    = new unsigned int[3]{
                static_cast<unsigned int>(face[0]), static_cast<unsigned int>(face[1]),
                static_cast<unsigned int>(face[2])
            };
        }
    }

    Assimp::Exporter exporter;

    auto export_result = exporter.Export(&scene, "obj", file_path);
    if (export_result != aiReturn_SUCCESS) {
        logger->error("assimp export failed {}", static_cast<int>(export_result));
        return false;
    }
    return true;
}

void Group::load_extra_info(const json& extra_info)
{
    // load name
    extra_info.at("name").get_to(name);

    // load object attributes
    const json& objects_info = extra_info.at("objects_info");
    if (objects_info.size() != objects.size()) {
        logger->warn("mismatching length of objects in extra json data");
        return;
    }
    for (size_t i = 0; i < objects.size(); ++i) {
        const unique_ptr<Object>& object = *std::find_if(
            this->objects.begin(), this->objects.end(),
            [&objects_info, &i](const unique_ptr<Object>& o) -> bool {
                return o->name == objects_info[i]["name"].get<string>();
            }
        );
        // manually call from_json because we already has object instances
        from_json(objects_info[i], *object);
    }
}

void Group::dump_extra_info(json& extra_info)
{
    // save group name to json
    extra_info["name"] = name;

    // save object attributes to json
    json& objects_info = extra_info["objects_info"] = json::object();
    for (size_t i = 0; i < objects.size(); ++i) {
        const unique_ptr<Object>& object = objects[i];
        objects_info[object->name]       = *object;
    }
}
