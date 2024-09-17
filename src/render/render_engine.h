#ifndef DANDELION_RENDER_RENDER_ENGINE_H
#define DANDELION_RENDER_RENDER_ENGINE_H

#include <memory>
#include <functional>
#include <queue>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../scene/scene.h"
#include "rasterizer.h"
#include "graphics_interface.h"
#include "rasterizer_renderer.h"

/*!
 * \file render/render_engine.h
 * \ingroup rendering
 * \~chinese
 * \brief 定义了渲染引擎和渲染器。
 */

class RasterizerRenderer;
class WhittedRenderer;

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 可用的渲染器类型
 */
enum class RendererType
{
    RASTERIZER,
    // RASTERIZER_MT,
    WHITTED_STYLE
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 离线渲染的执行入口
 */
class RenderEngine
{
public:
    RenderEngine();

    /*! \~chinese 渲染的结果（以无符号字符型变量进行存储）*/
    std::vector<unsigned char> rendering_res;
    /*! \~chinese 根据aspect_ratio对渲染出图片的长和宽进行设置 */
    float width, height;
    /*! \~chinese whitted_style renderer使用多线程时的线程数设置 */
    int n_threads;

    /*!
     * \~chinese
     * \brief 离线渲染入口，负责调用渲染器的渲染函数
     *
     * 可以选择不同的渲染方式，在设置好场景之后，执行该函数能够得到
     * 使用选定方式对当前场景的渲染结果
     *
     * \param scene 场景
     * \param type 渲染器类型
     */
    void render(Scene& scene, RendererType type);
    /*! \~chinese 渲染结果预览的背景颜色 */
    static Eigen::Vector3f background_color;

    /*! \~chinese 光栅化渲染器 */
    std::unique_ptr<RasterizerRenderer> rasterizer_render;
    /*! \~chinese whitted style渲染器 */
    std::unique_ptr<WhittedRenderer> whitted_render;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 实现光栅化渲染管线。
 */
class RasterizerRenderer
{
public:
    RasterizerRenderer(RenderEngine& engine, int num_vertex_threads, int num_rasterizer_threads,
                       int num_fragment_threads);

    /*! \~chinese 光栅化渲染器的渲染调用接口*/
    void render(const Scene& scene);

    /*! \~chinese 多线程光栅化渲染器的渲染调用接口*/
    // void render_mt(const Scene& scene);
    float& width;
    float& height;
    int n_vertex_threads;
    int n_rasterizer_threads;
    int n_fragment_threads;

    // initialize vertex processor
    VertexProcessor vertex_processor;

    // initialize rasterizer
    Rasterizer rasterizer;

    // initialize fragment processor
    FragmentProcessor fragment_processor;

    std::vector<unsigned char>& rendering_res;

private:
    std::shared_ptr<spdlog::logger> logger;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 实现 Whitted-Style 光线追踪管线。
 */
class WhittedRenderer
{
public:
    WhittedRenderer(RenderEngine& engine);
    /*! \~chinese whitted-style渲染器的渲染调用接口 */
    void render(Scene& scene);
    /*! \~chinese 镜面反射的阈值 */
    static constexpr float mirror_threshold = 1000.0f;
    float& width;
    float& height;
    int& n_threads;
    /*! \~chinese 是否使用 BVH 进行加速*/
    bool use_bvh;
    std::vector<unsigned char>& rendering_res;

private:
    /*!
     * \~chinese
     * \brief 菲涅尔方程根据观察角度计算反射光线的所占百分比
     *
     * \param I 入射光方向
     * \param N 法线向量
     * \param ior 当前反射介质的折射率
     *
     */
    float fresnel(const Eigen::Vector3f& I, const Eigen::Vector3f& N, const float& ior);
    std::optional<std::tuple<Intersection, GL::Material>> trace(const Ray& ray, const Scene& scene);
    /*!
     * \~chinese
     * \brief 追踪指定光线得到颜色
     *
     * \param ray 当前追踪的光线
     * \param scene 当前渲染的场景
     * \param depth 最大反射次数
     */
    Eigen::Vector3f cast_ray(const Ray& ray, const Scene& scene, int depth);
    std::shared_ptr<spdlog::logger> logger;
};

#endif // DANDELION_RENDER_RENDER_ENGINE_H
