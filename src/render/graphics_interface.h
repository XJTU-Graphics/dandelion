#ifndef DANDELION_RENDER_GRAPHICS_INTERFACE_H
#define DANDELION_RENDER_GRAPHICS_INTERFACE_H

#include <memory>
#include <functional>
#include <queue>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../scene/scene.h"

/*!
 * \file render/shader.h
 * \ingroup rendering
 * \~chinese
 * \brief 软渲染器使用的模拟着色器。
 */

struct VertexShaderPayload
{
    /*! \~chinese 顶点世界坐标系坐标 */
    Eigen::Vector4f world_position;
    /*! \~chinese 顶点视口坐标系坐标 */
    Eigen::Vector4f viewport_position;
    /*! \~chinese 法线向量 */
    Eigen::Vector3f normal;
};

/*!
 * \~chinese
 * \brief 作用于每个屏幕上的片元，通常是计算颜色。
 */
struct FragmentShaderPayload
{
    /*! \~chinese 世界坐标系下的位置 */
    Eigen::Vector3f world_pos;
    /*! \~chinese 世界坐标系下的法向量 */
    Eigen::Vector3f world_normal;
    /*! \~chinese 屏幕的像素坐标*/
    int x, y;
    /*! \~chinese 当前片元的深度*/
    float depth;
    /*! \~chinese 当前片元的颜色*/
    Eigen::Vector3f color;
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
Eigen::Vector3f phong_fragment_shader(const FragmentShaderPayload& payload, const GL::Material& material,
                                      const std::list<Light>& lights, const Camera& camera);

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

class SpinLock
{
public:
    void lock();
    void unlock();

private:
    std::atomic_flag locked;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief color buffer以及depth buffer
 */
class FrameBuffer
{
public:
    FrameBuffer(int width, int height);

    int width, height;

    /*!
     * \~chinese
     * \brief 设置当前像素点的颜色
     *
     * 根据计算出的index，对frame buffer中的color buffer和
     * depth buffer进行填充
     *
     * \param index 填充位置对应的索引
     * \param depth 当前着色点计算出的深度
     * \param color 当前着色点计算出的颜色值
     */
    void set_pixel(int index, float depth, const Eigen::Vector3f& color);

    /*!
     * \~chinese
     * \brief 将color buffer和depth buffer内容初始化
     *
     * color buffer最后会传递给rendering_res，所以可以直接初始化为背景颜色，
     * depth buffer则可以初始化为最大值
     *
     * \param buff 根据传入的buffer类型，初始化相应类型的buffer
     */
    void clear(BufferType buff);

    /*! \~chinese frame buffer的存储元素为Eigen::Vector3f,范围在[0,255]，三个分量分别表示(R,G,B) */
    std::vector<Eigen::Vector3f> color_buffer;
    /*! \~chinese depth buffer也可以叫做z-buffer，用于判断像素点相较于观察点的前后关系 */
    std::vector<float> depth_buffer;

private:
    /*! \~chinese spin_locks用于在深度测试和着色时对depth buffer以及frame buffer加锁*/
    std::vector<SpinLock> spin_locks;
};

/*!
 * \~chinese
 * \brief 用于存储RasterizerRenderer所需的全局变量
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

    /*! \~chinese 渲染物体的材质 */
    static GL::Material& material;
    /*! \~chinese 场景内的光源 */
    static std::list<Light>& lights;
    /*! \~chinese 当前渲染视角的相机 */
    static Camera& camera;
};

struct Context
{
    /*！\~chinese 保护队列的互斥锁 */
    static std::mutex vertex_queue_mutex;
    /*！\~chinese 保护队列的互斥锁 */
    static std::mutex rasterizer_queue_mutex;
    /*! \~chinese vertex shader的输出队列 */
    static std::queue<VertexShaderPayload> vertex_shader_output_queue;
    /*! \~chinese rasterizer的输出队列 */
    static std::queue<FragmentShaderPayload> rasterizer_output_queue;

    static bool vertex_finish;
    static bool rasterizer_finish;
    static bool fragment_finish;

    /*! \~chinese 渲染使用的frame buffer(color buffer & depth buffer)*/
    static FrameBuffer frame_buffer;
};

#endif // DANDELION_RENDER_GRAPHICS_INTERFACE_H