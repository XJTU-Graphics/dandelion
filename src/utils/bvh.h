#ifndef DANDELION_UTILS_BVH_H
#define DANDELION_UTILS_BVH_H

#include <vector>
#include <memory>
#include <algorithm>

#include "../src/platform/gl.hpp"
#include "./ray.h"
#include "aabb.h"

/*!
 * \file utils/bvh.h
 */

/*!
 * \ingroup utils
 * \~chinese
 * \brief 表示的是BVH建立的树中的节点
 */
struct BVHNode
{
    /*! \~english Initialization of the BVHNode */
    BVHNode();
    /*! \~english Aligned-axis bounding box */
    AABB aabb;
    /*! \~english left node of the BVHNode */
    BVHNode* left;
    /*! \~english right node of the BVHNode */
    BVHNode* right;
    /*! \~english face index of current node, initialized to 0
     *  will only >0 in leaf nodes(the number of faces it covers <= 1)
     */
    size_t face_idx;
};

/*!
 * \ingroup utils
 * \~chinese
 * \brief 用于在BVH划分左右子树时作为参与排序的节点
 */
struct SortNode
{
    /*! \~english face index of the sorted node */
    size_t index;
    /*! \~english centroid of the current AABB */
    Eigen::Vector3f centroid;
};

class BVH
{
public:
    /*!
     * \~chinese
     * \brief BVH加速结构的构造函数
     *
     * 传入当前mesh的类型
     *
     * \param mesh_type mesh的类型
     */
    BVH(const GL::Mesh& mesh);

    /*! \~chinese 建立整个object的bvh的函数调用接口 */
    void build();

    /*! \~chinese 删除建立的整个bvh */
    void recursively_delete(BVHNode* node);

    /*! \~chinese 统计当前bvh的节点总数 */
    size_t count_nodes(BVHNode* node);
    /*!
     * \~chinese
     * \brief BVH加速求交的函数调用接口
     *
     * \param ray 求交的射线
     * \param mesh 当前的mesh
     * \param obj_model 当前mesh所在object的model矩阵
     */
    std::optional<Intersection> intersect(const Ray& ray, const GL::Mesh& mesh,
                                          const Eigen::Matrix4f obj_model);

    /*!
     * \~chinese
     * \brief 获取BVH求交的结果
     * \param node 求交的节点
     * \param ray 求交的射线
     */
    std::optional<Intersection> ray_node_intersect(BVHNode* node, const Ray& ray) const;

    /*! \~chinese 整个bvh的根节点 */
    BVHNode* root;

    /*! \~chinese 建立整个object的bvh的函数具体实现，边界条件为覆盖的面片数<=1 */
    BVHNode* recursively_build(std::vector<size_t> faces_idx);

    /*! \~chinese 当前bvh所在object的mesh */
    const GL::Mesh& mesh;
    /*! \~chinese 当前mesh的所有图元索引 */
    std::vector<size_t> primitives;
    /*! \~chinese 当前bvh所在object的model矩阵 */
    Eigen::Matrix4f model;
};

#endif // DANDELION_UTILS_BVH_H
