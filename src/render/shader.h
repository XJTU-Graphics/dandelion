#ifndef DANDELION_RENDER_SHADER_PAYLOAD_H
#define DANDELION_RENDER_SHADER_PAYLOAD_H

#include <vector>
#include <list>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "../platform/gl.hpp"
#include "../scene/light.h"
#include "../scene/camera.h"

using Eigen::Vector3f;

/*!
 * \file render/shader.h
 * \ingroup rendering
 * \~chinese
 * \brief 软渲染器使用的模拟着色器。
 */

/*!
 * \~chinese
 * \brief 用于存储shader所需的全局变量
 */
struct Uniforms
{
    /*! \~chinese 当前渲染物体的MVP矩阵 */
    static Eigen::Matrix4f MVP;
    /*! \~chinese 当前渲染物体的model.inverse().transpose() */
    static Eigen::Matrix4f inv_trans_M;
    ///@{
    /*! \~chinese 当前成像平面的长和宽 */
    static int width;
    static int height;
    ///@}
};

/*!
 * \~chinese
 * \brief 作用于每个屏幕上的片元，通常是计算颜色。
 */
struct FragmentShaderPayload
{
    FragmentShaderPayload(const Vector3f& world_pos, const Vector3f& world_normal)
        : world_pos(world_pos), world_normal(world_normal)
    {
    }
    /*! \~chinese 世界坐标系下的位置 */
    Eigen::Vector3f world_pos;
    /*! \~chinese 世界坐标系下的法向量 */
    Eigen::Vector3f world_normal;
};

/*!
 * \~chinese
 * \brief 作用于每个顶点，通常是处理从世界空间到屏幕空间的坐标变化，后面紧接着光栅化。
 */
struct VertexShaderPayload
{
    /*! \~chinese 顶点位置 */
    Eigen::Vector4f position;
    /*! \~chinese 法线向量 */
    Eigen::Vector3f normal;
};

/*!
 * \~chinese
 * \brief 计算顶点的各项属性几何变化
 *
 * 首先是将顶点坐标变换到投影平面，再进行视口变换；
 * 其次是将法线向量变换到世界坐标系
 *
 * \param payload 输入时顶点和法线均为模型坐标系
 * 输出时顶点经过视口变换变换到了屏幕空间，法线向量则为世界坐标系
 */
VertexShaderPayload vertex_shader(const VertexShaderPayload& payload);

/*!
 * \~chinese
 * \brief 计算每个片元（像素）的颜色
 *
 * 根据输入参数：计算好的片元的位置和法线方向；材质(ka,kd,ks);场景光源以及视角
 * 计算当前片元的颜色
 *
 * \param payload 装的是相机坐标系下的片元位置以及法向量
 * \param material 当前片元所在物体的材质，包括ka,kd,ks
 * \param lights 当前场景中的所有光源
 * \param camera 当前观察相机
 *
 */
Eigen::Vector3f phong_fragment_shader(const FragmentShaderPayload& payload, GL::Material material,
                                      const std::list<Light>& lights, Camera camera);

#endif // DANDELION_RENDER_SHADER_PAYLOAD_H
