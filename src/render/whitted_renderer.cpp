#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <vector>
#include <optional>
#include <iostream>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "render_engine.h"
#include "../scene/light.h"
#include "../utils/math.hpp"
#include "../utils/ray.h"
#include "../utils/logger.h"

using std::chrono::steady_clock;
using duration   = std::chrono::duration<float>;
using time_point = std::chrono::time_point<steady_clock, duration>;
using Eigen::Vector3f;

// 最大的反射次数
constexpr int MAX_DEPTH        = 5;
constexpr float INFINITY_FLOAT = std::numeric_limits<float>::max();
// 考虑物体与光线相交点的偏移值
constexpr float EPSILON = 0.00001f;

// 当前物体的材质类型，根据不同材质类型光线会有不同的反射情况
enum class MaterialType
{
    DIFFUSE_AND_GLOSSY,
    REFLECTION
};

// 显示渲染的进度条
void UpdateProgress(float progress)
{
    int barwidth = 70;
    std::cout << "[";
    int pos = static_cast<int>(barwidth * progress);
    for (int i = 0; i < barwidth; i++) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "]" << int(progress * 100.0) << " %\r";
    std::cout.flush();
}

WhittedRenderer::WhittedRenderer(RenderEngine& engine)
    : width(engine.width), height(engine.height), n_threads(engine.n_threads), use_bvh(false),
      rendering_res(engine.rendering_res)
{
    logger = get_logger("Whitted Renderer");
}

// whitted-style渲染的实现
void WhittedRenderer::render(Scene& scene)
{
    time_point begin_time = steady_clock::now();
    width                 = std::floor(width);
    height                = std::floor(height);

    // initialize frame buffer
    std::vector<Vector3f> framebuffer(static_cast<size_t>(width * height));
    for (auto& v : framebuffer) {
        v = Vector3f(0.0f, 0.0f, 0.0f);
    }

    int idx = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // generate ray
            Ray ray = generate_ray(static_cast<int>(width), static_cast<int>(height), i, j,
                                   scene.camera, 1.0f);
            // cast ray
            framebuffer[idx++] = cast_ray(ray, scene, 0);
        }
        UpdateProgress(j / height);
    }
    // save result to whitted_res.ppm
    FILE* fp = fopen("whitted_res.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", (int)width, (int)height);
    static unsigned char color_res[3];
    rendering_res.clear();
    for (long unsigned int i = 0; i < framebuffer.size(); i++) {
        color_res[0] = static_cast<unsigned char>(255 * clamp(0.f, 1.f, framebuffer[i][0]));
        color_res[1] = static_cast<unsigned char>(255 * clamp(0.f, 1.f, framebuffer[i][1]));
        color_res[2] = static_cast<unsigned char>(255 * clamp(0.f, 1.f, framebuffer[i][2]));
        fwrite(color_res, 1, 3, fp);
        rendering_res.push_back(color_res[0]);
        rendering_res.push_back(color_res[1]);
        rendering_res.push_back(color_res[2]);
    }
    time_point end_time         = steady_clock::now();
    duration rendering_duration = end_time - begin_time;
    logger->info("rendering takes {:.6f} seconds", rendering_duration.count());
}

// 菲涅尔定理计算反射光线
float WhittedRenderer::fresnel(const Vector3f& I, const Vector3f& N, const float& ior)
{
    // these lines below are just for compiling and can be deleted
    (void)I;
    (void)N;
    (void)ior;
    return INFINITY_FLOAT - EPSILON;
    // these lines above are just for compiling and can be deleted
}

// 如果相交返回Intersection结构体，如果不相交则返回false
std::optional<std::tuple<Intersection, GL::Material>> WhittedRenderer::trace(const Ray& ray,
                                                                             const Scene& scene)
{
    // this line below is just for compiling and can be deleted
    (void)ray;

    std::optional<Intersection> payload;
    Eigen::Matrix4f M;
    GL::Material material;
    for (const auto& group : scene.groups) {
        for (const auto& object : group->objects) {

            // this line below is just for compiling and can be deleted
            (void)object;

            // if use bvh(exercise 2.4): use object->bvh->intersect
            // else(exercise 2.3): use naive_intersect()
            // pay attention to the range of payload->t
        }
    }

    if (!payload.has_value()) {
        return std::nullopt;
    }
    return std::make_tuple(payload.value(), material);
}

// Whitted-style的光线传播算法实现
Vector3f WhittedRenderer::cast_ray(const Ray& ray, const Scene& scene, int depth)
{
    if (depth > MAX_DEPTH) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }
    // initialize hit color
    Vector3f hitcolor = RenderEngine::background_color;
    // get the result of trace()
    auto result = trace(ray, scene);

    // this line below is just for compiling and can be deleted
    (void)result;

    // if result.has_value():
    // 1.judge the material_type
    // 2.if REFLECTION:
    //(1)use fresnel() to get kr
    //(2)hitcolor = cast_ray*kr
    // if DIFFUSE_AND_GLOSSY:
    //(1)compute shadow result using trace()
    //(2)hitcolor = diffuse*kd + specular*ks

    return hitcolor;
}
