#include "bvh.h"

#include <cassert>
#include <iostream>
#include <optional>

#include <Eigen/Geometry>
#include "formatter.hpp"
#include <spdlog/spdlog.h>

#include "math.hpp"

using Eigen::Vector3f;
using std::optional;
using std::vector;

BVHNode::BVHNode() : left(nullptr), right(nullptr), face_idx(0)
{
}

BVH::BVH(const GL::Mesh& mesh) : root(nullptr), mesh(mesh)
{
}

// 建立bvh，将需要建立BVH的图元索引初始化
void BVH::build()
{
    if (mesh.faces.count() == 0) {
        root = nullptr;
        return;
    }

    primitives.resize(mesh.faces.count());
    for (size_t i = 0; i < mesh.faces.count(); i++) primitives[i] = i;

    root = recursively_build(primitives);
    return;
}
// 删除bvh
void BVH::recursively_delete(BVHNode* node)
{
    if (node == nullptr)
        return;
    recursively_delete(node->left);
    recursively_delete(node->right);
    delete node;
    node = nullptr;
}
// 统计BVH树建立的节点个数
size_t BVH::count_nodes(BVHNode* node)
{
    if (node == nullptr)
        return 0;
    else
        return count_nodes(node->left) + count_nodes(node->right) + 1;
}
// 递归建立BVH
BVHNode* BVH::recursively_build(vector<size_t> faces_idx)
{
    BVHNode* node = new BVHNode();

    AABB aabb;
    for (size_t i = 0; i < faces_idx.size(); i++) {
        aabb = union_AABB(aabb, get_aabb(mesh, faces_idx[i]));
    }
    // if faces_idx.size()==1: return node;
    // if faces_idx.size()==2: recursively_build() & union_AABB(node->left->aabb,
    // node->right->aabb); else:
    // choose the longest dimension among x,y,z
    // devide the primitives into two along the longest dimension
    // recursively_build() & union_AABB(node->left->aabb, node->right->aabb)
    return node;
}
// 使用BVH求交
optional<Intersection> BVH::intersect(const Ray& ray, [[maybe_unused]] const GL::Mesh& mesh,
                                      const Eigen::Matrix4f obj_model)
{
    model = obj_model;
    optional<Intersection> isect;
    if (!root) {
        isect = std::nullopt;
        return isect;
    }
    isect = ray_node_intersect(root, ray);
    return isect;
}
// 发射的射线与当前节点求交，并递归获取最终的求交结果
optional<Intersection> BVH::ray_node_intersect(BVHNode* node, const Ray& ray) const
{
    // these lines below are just for compiling and can be deleted
    (void)ray;
    (void)node;
    // these lines above are just for compiling and can be deleted

    optional<Intersection> isect;
    // The node intersection is performed in the model coordinate system.
    // Therefore, the ray needs to be transformed into the model coordinate system.
    // The intersection attributes returned are all in the model coordinate system.
    // Therefore, They are need to be converted to the world coordinate system.    
    // If the model shrinks, the value of t will also change.
    // The change of t can be solved by intersection point changing simultaneously
            
    return isect;
}
