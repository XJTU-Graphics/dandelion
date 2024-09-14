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
 * \file render/rasterizer_mt.h
 * \ingroup rendering
 * \~chinese
 * 给定材质、Model/View/Projection Matrix 的光栅化器。
 */

float sign(Eigen::Vector2f p1, Eigen::Vector2f p2, Eigen::Vector2f p3);

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 软光栅渲染器
 *
 * 这个类主要由光栅化所需的各种属性（如成像平面的长宽，以及M,V,P矩阵等），以及光栅化
 * 整个流程所需的各种操作对应的函数构成（如vertex shader, fragment shader,等）
 */
class Rasterizer
{
public:
    void worker_thread();
    void rasterize_triangle(Triangle& t);

private:
    /*!
     * \~chinese
     * \brief 光栅化当前三角形
     *
     * 整个光栅化渲染应当包含：VERTEX SHADER -> MVP -> Clipping -> VIEWPORT -> DRAWLINE/DRAWTRI ->
     * FRAGMENT SHADER 在进入rasterize_triangle之前已经完成了VERTEX
     * SHADER到VIEWPORT的整个过程，可以被看作几何阶段
     * 在进入rasterize_triangle之后，就开始进行真正的光栅化了，逐片元的进行着色
     *
     * \param t 当前进行光栅化的三角形
     * \param world_pos 三角形三个顶点的坐标(已经变换到world_space下)
     * \param material 当前三角形所在物体的材质
     * \param lights 当前场景的所有光源(已经变换到world_space下)
     * \param camera 当前使用的相机（观察点）
     */

    /*! \~chinese 判断像素坐标(x,y)是否在给定三个顶点的三角形内 */
    static bool inside_triangle(int x, int y, const Eigen::Vector4f* vertices);
    /*! \~chinese 计算像素坐标(x,y)在给定三个顶点的三角形内的重心坐标 */
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
     * \param weight Vector3f{v[0].w(), v[1].w(), v[2].w()}
     * \param Z 1 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
     */
    static Eigen::Vector3f interpolate(float alpha, float beta, float gamma,
                                       const Eigen::Vector3f& vert1, const Eigen::Vector3f& vert2,
                                       const Eigen::Vector3f& vert3, const Eigen::Vector3f& weight,
                                       const float& Z);
};

#endif // DANDELION_RENDER_RASTERIZER_H
