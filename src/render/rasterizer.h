#ifndef DANDELION_RENDER_RASTERIZER_MT_H
#define DANDELION_RENDER_RASTERIZER_MT_H

#include <algorithm>
#include <functional>
#include <map>
#include <vector>
#include <list>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../platform/gl.hpp"
#include "../scene/light.h"
#include "../scene/camera.h"
#include "shader.h"
#include "triangle.h"

/*!
 * \file render/rasterizer_mt.h
 * \ingroup rendering
 * \~chinese
 * 给定材质、Model/View/Projection Matrix 的光栅化器。
 */

/*!
 * \~chinese
 * \brief 用于选择buffer的类型
 */
enum class BufferType
{
    Color = 1,
    Depth = 2
};

inline BufferType operator|(BufferType a, BufferType b)
{
    return BufferType((int)a | (int)b);
}

inline BufferType operator&(BufferType a, BufferType b)
{
    return BufferType((int)a & (int)b);
}

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
    /*! \~chinese 构造函数：指定成像平面的长宽 */
    Rasterizer(int w, int h);

    int width, height;

    /*! \~chinese 当前渲染物体的model矩阵 */
    Eigen::Matrix4f model;
    /*! \~chinese 当前相机的view矩阵 */
    Eigen::Matrix4f view;
    /*! \~chinese 当前相机的projection矩阵 */
    Eigen::Matrix4f projection;
    /*! \~chinese 指定使用的vertex shader */
    std::function<VertexShaderPayload(VertexShaderPayload)> vertex_shader;
    /*! \~chinese 指定使用的fragment shader */
    std::function<Eigen::Vector3f(FragmentShaderPayload, GL::Material, const std::list<Light>&,
                                  Camera)>
        fragment_shader;
    /*!
     * \~chinese
     * \brief 设置当前像素点的颜色
     *
     * 首先根据成像平面的width,height以及point的x,y计算出当前点在frame buffer中的索引
     * 再将frame buffer中对应索引处赋值为color
     *
     * \param point 当前着色点在成像平面对应的坐标
     * \param color 当前着色点计算出的颜色值
     */
    void set_pixel(const Eigen::Vector2i& point, const Eigen::Vector3f& color);

    /*!
     * \~chinese
     * \brief 对整个物体进行光栅化渲染
     *
     * 对物体的所有三角形面片进行遍历，对三角形的顶点应用vertex shader，对顶点的坐标和法线方向
     * 进行变化；同时获取三角形每个顶点在view space下的坐标便于后续的插值操作；对顶点的法线同样
     * 需要变换到view space下用于后续的插值，最后执行光栅化
     *
     * \param TriangleList 存储了当前渲染物体的所有三角形面片
     * \param material 当前渲染物体的材质
     * \param lights 存储了当前场景中的所有光源
     * \param camera 当前使用的相机（观察点）
     */
    void draw(const std::vector<Triangle>& TriangleList, const GL::Material& material,
              const std::list<Light>& lights, const Camera& camera);
    void draw_mt(const std::vector<Triangle>& TriangleList, const GL::Material& material,
                 const std::list<Light>& lights, const Camera& camera);

    /*!
     * \~chinese
     * \brief 将frame buffer和depth buffer内容初始化
     *
     * frame buffer最后会传递给rendering_res，所以可以直接初始化为背景颜色，
     * depth buffer则可以初始化为最大值
     *
     * \param buff 根据传入的buffer类型，初始化相应类型的buffer
     */
    void clear(BufferType buff);
    /*! \~chinese frame buffer的存储元素为Eigen::Vector3f,范围在[0,255]，三个分量分别表示(R,G,B) */
    std::vector<Eigen::Vector3f> frame_buf;
    /*! \~chinese depth buffer也可以叫做z-buffer，用于判断像素点相较于观察点的前后关系 */
    std::vector<float> depth_buf;

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
    void rasterize_triangle(const Triangle& t, const std::array<Eigen::Vector3f, 3>& world_pos,
                            GL::Material material, const std::list<Light>& lights, Camera camera);
    void rasterize_triangle_mt(const Triangle& t, const std::array<Eigen::Vector3f, 3>& world_pos,
                               GL::Material material, const std::list<Light>& lights,
                               Camera camera);

    /*! \~chinese 判断像素坐标(x,y)是否在给定三个顶点的三角形内 */
    static bool inside_triangle(int x, int y, const Eigen::Vector4f* vertices);
    /*! \~chinese 计算像素坐标(x,y)在给定三个顶点的三角形内的重心坐标 */
    static std::tuple<float, float, float> compute_barycentric_2d(float x, float y,
                                                                  const Eigen::Vector4f* v);
    /*!
     * \~chinese
     * \brief 对顶点的任意属性（如view space坐标，法线向量）利用屏幕空间进行插值
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

    /*! \~chinese 获取给定坐标(x,y)，计算在frame buffer中对应的index */
    int get_index(int x, int y);
};

#endif // DANDELION_RENDER_RASTERIZER_MT_H
