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
 * \file render/graphics_interface.h
 * \ingroup rendering
 * \~chinese
 * \brief 一些公用的渲染管线接口。
 */

/*！
 * \ingroup rendering
 * \~chinese
 * \brief 顶点着色器的输入和输出单位。
 */
struct VertexShaderPayload
{
    /*! \~chinese 顶点世界坐标系坐标 */
    Eigen::Vector4f world_position;
    /*! \~chinese 顶点视口坐标系坐标 */
    Eigen::Vector4f viewport_position;
    /*! \~chinese 顶点法线 */
    Eigen::Vector3f normal;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 片元着色器的输入单位。
 * 
 * “片元”相当于未着色的像素，每个片元数据中均包含为该片元着色所需的局部信息（全局信息则位于 `Uniforms` 中）。
 */
struct FragmentShaderPayload
{
    /*! \~chinese 世界坐标系下的位置 */
    Eigen::Vector3f world_pos;
    /*! \~chinese 世界坐标系下的法向量 */
    Eigen::Vector3f world_normal;
    /*! \~chinese 屏幕空间坐标 */
    int x, y;
    /*! \~chinese 当前片元的深度 */
    float depth;
    /*! \~chinese 当前片元的颜色 */
    Eigen::Vector3f color;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 计算顶点的各项属性几何变化。
 *
 * 首先是将顶点坐标变换到投影平面，再进行视口变换；
 * 其次是将法线向量变换到世界坐标系
 *
 * \param payload 输入时顶点和法线均为模型坐标系
 * 输出时顶点经过视口变换变换到了屏幕空间，法线向量则为世界坐标系
 */
VertexShaderPayload vertex_shader(const VertexShaderPayload& payload);

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 使用 Blinn Phong 着色模型计算每个片元（像素）的颜色。
 *
 * \param payload 装的是相机坐标系下的片元位置以及法向量
 * \param material 当前片元所在物体的材质，包括环境光、漫反射和镜面反射系数等
 * \param lights 当前场景中的所有光源
 * \param camera 离线渲染所用的相机
 */
Eigen::Vector3f phong_fragment_shader(const FragmentShaderPayload& payload, const GL::Material& material,
                                      const std::list<Light>& lights, const Camera& camera);

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 用于选择 buffer 的类型
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
 * \ingroup utils
 * \~chinese
 * \brief 自旋锁
 * 
 * 一个比较高效的自旋锁实现，做了局部自旋和主动退避优化。
 */
class SpinLock
{
public:
    /*! \~chinese 加锁。 */
    void lock();
    /*! \~chinese 解锁。 */
    void unlock();

private:
    /*! \~chinese 内部用于实现锁的原子变量。 */
    std::atomic_flag locked;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 一个最简化的 Frame Buffer 。
 * 
 * 要渲染一帧，通常至少需要一个 color buffer 用于输出颜色、一个 depth buffer 用于记录深度。
 */
class FrameBuffer
{
public:
    /*! \~chinese 初始化一个 frame buffer 。 */
    FrameBuffer(int width, int height);

    /*! \~chinese Frame buffer 对应图像的宽度和高度 */
    int width, height;

    /*!
     * \~chinese
     * \brief 设置当前像素点的颜色
     *
     * 根据计算出的 index，对 frame buffer 中的 color buffer 和
     * depth buffer 进行填充
     *
     * \param index 填充位置对应的一维索引
     * \param depth 当前着色点计算出的深度
     * \param color 当前着色点计算出的颜色值
     */
    void set_pixel(int index, float depth, const Eigen::Vector3f& color);

    /*!
     * \~chinese
     * \brief 初始化指定类型的 buffer。
     *
     * color buffer 最后会传递给 rendering_res，所以可以直接初始化为背景颜色
     * depth buffer 则可以初始化为最大值
     *
     * \param buff 根据传入的 buffer 类型，初始化相应类型的 buffer
     */
    void clear(BufferType buff);

    /*! \~chinese color buffer 的存储元素为 Eigen::Vector3f,范围在 [0,255]，三个分量分别表示 (R,G,B) */
    std::vector<Eigen::Vector3f> color_buffer;
    /*! \~chinese depth buffer 也可以叫做 z-buffer，用于判断像素点相较于观察点的前后关系 */
    std::vector<float> depth_buffer;

private:
    /*! \~chinese 用于在深度测试和着色时对相应位置的像素加锁 */
    std::vector<SpinLock> spin_locks;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 用于存储 RasterizerRenderer 所需的全局变量。
 */
struct Uniforms
{
    /*! \~chinese 当前渲染物体的 MVP 矩阵 */
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

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 存放实现渲染管线所需的一些全局数据。
 */
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

    /*! \~chinese 标识顶点着色器是否全部执行完毕。 */
    static bool vertex_finish;
    /*! \~chinese 标识三角形是否全部被光栅化。 */
    static bool rasterizer_finish;
    /*! \~chinese 标识片元着色器是否全部执行完毕。 */
    static bool fragment_finish;

    /*! \~chinese 渲染使用的 frame buffer 。 */
    static FrameBuffer frame_buffer;
};

#endif // DANDELION_RENDER_GRAPHICS_INTERFACE_H
