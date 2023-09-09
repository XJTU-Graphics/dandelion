#ifndef DANDELION_UTILS_AABB_H
#define DANDELION_UTILS_AABB_H

#include <limits>
#include <array>

#include <Eigen/Core>
#include <spdlog/spdlog.h>

#include "ray.h"

/*!
 * \file utils/aabb.h
 */

/*!
 * \ingroup utils
 * \~chinese
 * \brief BVH中的Aligned-axis bounding box
 */
class AABB
{
public:
    /*! \~chinese x,y,z最小的点以及最大的点，两个点即能确定一个AABB */
    Eigen::Vector3f p_min, p_max;

    AABB();
    AABB(const Eigen::Vector3f& p) : p_min(p), p_max(p)
    {
    }
    AABB(const Eigen::Vector3f& p1, const Eigen::Vector3f& p2);

    /*! \~chinese 返回p_max和p_min的距离，即AABB對角线的长度 */
    Eigen::Vector3f diagonal() const;
    /*! \~chinese 返回AABB的x,y,z中最长的一维 */
    int max_extent() const;
    /*! \~chinese 返回AABB的中心坐标 */
    Eigen::Vector3f centroid();

    /*
    AABB intersect(const AABB& b);
    bool inside(const Eigen::Vector3f& p, const AABB& b);
    inline const Eigen::Vector3f& operator[](int i) const{
        return (i==0) ? p_min : p_max;
    }
    */

    /*!
     * \~chinese
     * \brief BVH加速求交的函数调用接口
     *
     * \param ray 求交的射线
     * \param inv_dir (1/dir.x, 1/dir.y, 1/dir.z)，乘法比除法快
     * \param dir_is_neg 判断x,y,z方向是否为负，为负则交换t_min和t_max
     */
    bool intersect(const Ray& ray, const Eigen::Vector3f& inv_dir,
                   const std::array<int, 3>& dir_is_neg);
};

/*! \~chinese
 * 将两个AABB融合，融合之后的AABB的p_min各维度取二者最小值，p_max各维度取二者最大值，返回融合之后的AABB
 */
AABB union_AABB(const AABB& b1, const AABB& b2);

/*! \~chinese
 * 将一个AABB和一个点融合，融合之后的AABB的p_min各维度取二者最小值，p_max各维度取二者最大值，返回融合之后的AABB
 */
AABB union_AABB(const AABB& b, const Eigen::Vector3f& p);

/*!
 * \~chinese
 * \brief BVH加速求交的函数调用接口
 *
 * \param mesh 当前AABB所在的mesh
 * \param face_idx 当前的面片所对应的index
 */
AABB get_aabb(const GL::Mesh& mesh, size_t face_idx);

#endif // DANDELION_UTILS_AABB_H
