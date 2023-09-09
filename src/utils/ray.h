#ifndef DANDELION_UTILS_RAY_H
#define DANDELION_UTILS_RAY_H

/*!
 * \ingroup utils
 * \file utils/ray.h
 * \~chinese
 * \brief 提供生成射线、判定相交的工具函数。
 */

#include <cstddef>
#include <optional>

#include <Eigen/Core>

#include "../platform/gl.hpp"
#include "../scene/camera.h"

/*!
 * \ingroup rendering
 * \ingroup utils
 */
struct Ray
{
    Eigen::Vector3f origin;
    Eigen::Vector3f direction;
};

/*!
 * \ingroup rendering
 * \ingroup utils
 * \~chinese
 * \brief 表示射线与 Mesh 相交结果的结构体。
 *
 * Intersection 对象总是有意义的，一旦被创建就表示相交确实发生了。
 */
struct Intersection
{
    /*! \~chinese Intersection的构造函数：将t设置为无穷，face_index为零 */
    Intersection();
    /*! \~chinese 射线以 \f$\mathrm{o}+t\mathrm{d}\f$ 表示，交点处对应的 \f$t\f$ 值。 */
    float t;
    /*! \~chinese 交点所在面片的序号，可用作 `GL::Mesh::face` 方法的参数。 */
    std::size_t face_index;
    /*! \~chinese 交点处的重心坐标。 */
    Eigen::Vector3f barycentric_coord;
    /*! \~chinese 交点所在面片的法向量。 */
    Eigen::Vector3f normal;
};

/*!
 * \ingroup rendering
 * \ingroup utils
 * \~chinese
 * \brief 给定成像平面的宽度和高度、成像平面上的坐标、成像平面的深度和相机，生成一条射线。
 *
 * \param width 成像平面的宽度（以像素计）
 * \param height 成像平面的高度（以像素计）
 * \param x 成像平面上指定点的 x 坐标（宽度方向，以像素计）
 * \param y 成像平面上指定点的 y 坐标（高度方向，以像素计）
 * \param camera 指定的相机
 * \param depth 成像平面在指定相机观察空间 (view space) 下的深度值（应当为正数）
 *
 * \returns 构造出的射线
 */
Ray generate_ray(int width, int height, int x, int y, Camera& camera, float depth);

/*!
 * \ingroup rendering
 * \ingroup utils
 * \~chinese
 * \brief 判断光线ray是否与某个面片相交
 *
 * 这个函数判断是否与一个三角形面片相交
 *
 * \param ray 射线
 * \param mesh 面片所在的mesh
 * \param index 面片的索引
 *
 * \returns 一个 `std::optional` 对象，若相交，则此对象有值 (`has_value() == true`)；不相交则返回
 * `std::nullopt` 。
 */
std::optional<Intersection> ray_triangle_intersect(const Ray& ray, const GL::Mesh& mesh,
                                                   size_t index);

/*!
 * \ingroup rendering
 * \ingroup utils
 * \~chinese
 * \brief 用朴素方法判断射线是否与给定的 mesh 相交。
 *
 * 这个函数顺序遍历 mesh 中的所有面片，逐一判断射线是否与之相交，最终取所有交点中 \f$t\f$
 * 值最小的一个作为结果。
 *
 * \param ray 射线
 * \param mesh 指定的 mesh
 * \param model 这个 mesh 对应的模型变换矩阵 (model transform)
 *
 * \returns 一个 `std::optional` 对象，若相交，则此对象有值 (`has_value() == true`)；不相交则返回
 * `std::nullopt` 。
 */
std::optional<Intersection> naive_intersect(const Ray& ray, const GL::Mesh& mesh,
                                            const Eigen::Matrix4f model);

#endif // DANDELION_UTILS_RAY_H
