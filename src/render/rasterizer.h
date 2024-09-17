#ifndef DANDELION_RENDER_RASTERIZER_H
#define DANDELION_RENDER_RASTERIZER_H

#include <algorithm>
#include <functional>
#include <map>
#include <vector>
#include <list>
#include <queue>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../platform/gl.hpp"
#include "../scene/light.h"
#include "../scene/camera.h"
#include "triangle.h"
#include "rasterizer_renderer.h"

/*!
 * \file render/rasterizer.h
 * \ingroup rendering
 * \~chinese
 * \brief 光栅化渲染器中光栅化阶段的实现。
 */

float sign(Eigen::Vector2f p1, Eigen::Vector2f p2, Eigen::Vector2f p3);

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 光栅化器
 */
class Rasterizer
{
public:
    void worker_thread();

private:
    /*!
     * \~chinese
     * \brief 将指定三角形光栅化为片元。
     *
     * \param t 要进行光栅化的三角形
     */
    void rasterize_triangle(Triangle& t);

    /*! \~chinese 判断像素坐标 (x,y) 是否在给定三个顶点的三角形内 */
    static bool inside_triangle(int x, int y, const Eigen::Vector4f* vertices);
    /*! \~chinese 计算像素坐标 (x,y) 在给定三个顶点的三角形内的重心坐标 */
    static std::tuple<float, float, float> compute_barycentric_2d(float x, float y,
                                                                  const Eigen::Vector4f* v);
    /*!
     * \~chinese
     * \brief 对顶点的任意属性（如world space坐标，法线向量）利用屏幕空间进行插值
     *
     * 这里的插值使用了透视矫正插值
     *
     * \param alpha, beta, gamma 计算出的重心坐标
     * \param vert1, vert2, vert3 三角形的三个顶点的任意待插值属性
     * \param weight 三个顶点的 w 坐标 (`Vector3f{v[0].w(), v[1].w(), v[2].w()}`)
     * \param Z 1 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
     */
    static Eigen::Vector3f interpolate(float alpha, float beta, float gamma,
                                       const Eigen::Vector3f& vert1, const Eigen::Vector3f& vert2,
                                       const Eigen::Vector3f& vert3, const Eigen::Vector3f& weight,
                                       const float& Z);
};

#endif // DANDELION_RENDER_RASTERIZER_H
