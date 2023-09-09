#include "group.h"

#include <set>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <Eigen/Core>
#include <fmt/format.h>

#include "../utils/logger.h"

using Eigen::Vector3f;
using std::make_pair;
using std::make_unique;
using std::pair;
using std::set;
using std::size_t;
using std::string;

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
    const aiScene* scene =
        importer.ReadFile(file_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                         aiProcess_JoinIdenticalVertices |
                                         aiProcess_GenSmoothNormals | aiProcess_DropNormals);
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
        const aiMesh* mesh         = scene->mMeshes[mesh_id];
        const aiFace* faces        = mesh->mFaces;
        const aiVector3D* vertices = mesh->mVertices;
        const aiVector3D* normals  = mesh->mNormals;
        size_t n_vertices          = mesh->mNumVertices;
        size_t n_faces             = mesh->mNumFaces;
        string name(mesh->mName.C_Str());
        set<pair<size_t, size_t>> edges;
        logger->info("the {}-th mesh has {} faces", mesh_id + 1, n_faces);

        objects.push_back(make_unique<Object>(name));
        Object& object                             = *(objects.back());
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
        for (const auto& edge : edges) {
            object_edges.append((unsigned int)edge.first, (unsigned int)edge.second);
        }
        // Load material if it exists.
        static const string default_material_name("DefaultMaterial");
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        string material_name(material->GetName().C_Str());
        // Assimp's default material should not be used.
        if (material_name != default_material_name) {
            GL::Material& object_material = object.mesh.material;
            aiColor3D color;
            float shininess;
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
        logger->info("summary: {} vertices, {} edges, {} faces", mesh->mNumVertices, edges.size(),
                     object.mesh.faces.count());
        object.rebuild_BVH();
        logger->info("The BVH structure of {} (ID: {}) has {} boxes", object.name, object.id,
                     object.bvh->count_nodes(object.bvh->root));
        object.modified = true;
    }

    return true;
}
