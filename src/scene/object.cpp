#include "object.h"

#include <array>
#include <optional>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include "../utils/math.hpp"
#include "../utils/ray.h"
#include "../simulation/solver.h"
#include "../utils/logger.h"

using Eigen::Matrix4f;
using Eigen::Quaternionf;
using Eigen::Vector3f;
using std::array;
using std::make_unique;
using std::optional;
using std::string;
using std::vector;

bool Object::BVH_for_collision   = false;
size_t Object::next_available_id = 0;
std::function<KineticState(const KineticState&, const KineticState&)> Object::step =
    forward_euler_step;

Object::Object(const string& object_name)
    : name(object_name), center(0.0f, 0.0f, 0.0f), scaling(1.0f, 1.0f, 1.0f),
      rotation(1.0f, 0.0f, 0.0f, 0.0f), velocity(0.0f, 0.0f, 0.0f), force(0.0f, 0.0f, 0.0f),
      mass(1.0f), BVH_boxes("BVH", GL::Mesh::highlight_wireframe_color)
{
    visible  = true;
    modified = false;
    id       = next_available_id;
    ++next_available_id;
    bvh                      = make_unique<BVH>(mesh);
    const string logger_name = fmt::format("{} (Object ID: {})", name, id);
    logger                   = get_logger(logger_name);
}

Matrix4f Object::model()
{
    return Matrix4f::Identity();
}

void Object::update(vector<Object*>& all_objects)
{
    // 首先调用 step 函数计下一步该物体的运动学状态。
    KineticState current_state{center, velocity, force / mass};
    KineticState next_state = step(prev_state, current_state);
    (void)next_state;
    // 将物体的位置移动到下一步状态处，但暂时不要修改物体的速度。
    // 遍历 all_objects，检查该物体在下一步状态的位置处是否会与其他物体发生碰撞。
    for (auto object : all_objects) {
        (void)object;

        // 检测该物体与另一物体是否碰撞的方法是：
        // 遍历该物体的每一条边，构造与边重合的射线去和另一物体求交，如果求交结果非空、
        // 相交处也在这条边的两个端点之间，那么该物体与另一物体发生碰撞。
        // 请时刻注意：物体 mesh 顶点的坐标都在模型坐标系下，你需要先将其变换到世界坐标系。
        for (size_t i = 0; i < mesh.edges.count(); ++i) {
            array<size_t, 2> v_indices = mesh.edge(i);
            (void)v_indices;
            // v_indices 中是这条边两个端点的索引，以这两个索引为参数调用 GL::Mesh::vertex
            // 方法可以获得它们的坐标，进而用于构造射线。
            if (BVH_for_collision) {
            } else {
            }
            // 根据求交结果，判断该物体与另一物体是否发生了碰撞。
            // 如果发生碰撞，按动量定理计算两个物体碰撞后的速度，并将下一步状态的位置设为
            // current_state.position ，以避免重复碰撞。
        }
    }
    // 将上一步状态赋值为当前状态，并将物体更新到下一步状态。
}

void Object::render(const Shader& shader, WorkingMode mode, bool selected)
{
    if (modified) {
        mesh.VAO.bind();
        mesh.vertices.to_gpu();
        mesh.normals.to_gpu();
        mesh.edges.to_gpu();
        mesh.edges.release();
        mesh.faces.to_gpu();
        mesh.faces.release();
        mesh.VAO.release();
    }
    modified = false;
    // Render faces anyway.
    unsigned int element_flags = GL::Mesh::faces_flag;
    if (mode == WorkingMode::MODEL) {
        // For *Model* mode, only the selected object is rendered at the center in the world.
        // So the model transform is the identity matrix.
        shader.set_uniform("model", I4f);
        shader.set_uniform("normal_transform", I4f);
        element_flags |= GL::Mesh::vertices_flag;
        element_flags |= GL::Mesh::edges_flag;
    } else {
        Matrix4f model = this->model();
        shader.set_uniform("model", model);
        shader.set_uniform("normal_transform", (Matrix4f)(model.inverse().transpose()));
    }
    // Render edges of the selected object for modes with picking enabled.
    if (check_picking_enabled(mode) && selected) {
        element_flags |= GL::Mesh::edges_flag;
    }
    mesh.render(shader, element_flags);
}

void Object::rebuild_BVH()
{
    bvh->recursively_delete(bvh->root);
    bvh->build();
    BVH_boxes.clear();
    refresh_BVH_boxes(bvh->root);
    BVH_boxes.to_gpu();
}

void Object::refresh_BVH_boxes(BVHNode* node)
{
    if (node == nullptr) {
        return;
    }
    BVH_boxes.add_AABB(node->aabb.p_min, node->aabb.p_max);
    refresh_BVH_boxes(node->left);
    refresh_BVH_boxes(node->right);
}
