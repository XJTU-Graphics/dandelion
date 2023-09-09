#include <array>
#include <limits>
#include <tuple>
#include <vector>
#include <algorithm>
#include <cmath>
#include <mutex>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "rasterizer.h"
#include "triangle.h"
#include "render_engine.h"
#include "../utils/math.hpp"

using Eigen::Matrix4f;
using Eigen::Vector2i;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::fill;
using std::tuple;

// 给定坐标(x,y)以及三角形的三个顶点坐标，判断(x,y)是否在三角形的内部
bool Rasterizer::inside_triangle(int x, int y, const Vector4f* vertices)
{
    Vector3f v[3];
    for (int i = 0; i < 3; i++) v[i] = {vertices[i].x(), vertices[i].y(), 1.0};

    Vector3f p(float(x), float(y), 1.0f);

    return false;
}

// 给定坐标(x,y)以及三角形的三个顶点坐标，计算(x,y)对应的重心坐标[alpha, beta, gamma]
tuple<float, float, float> Rasterizer::compute_barycentric_2d(float x, float y, const Vector4f* v)
{
    float c1 = 0.f, c2 = 0.f, c3 = 0.f;
    
    // these lines below are just for compiling and can be deleted
    (void)x;
    (void)y;
    (void)v;
    // these lines above are just for compiling and can be deleted

    return {c1, c2, c3};
}

// 对当前渲染物体的所有三角形面片进行遍历，进行几何变换以及光栅化
void Rasterizer::draw(const std::vector<Triangle>& TriangleList, const GL::Material& material,
                      const std::list<Light>& lights, const Camera& camera)
{
    // these lines below are just for compiling and can be deleted
    (void)material;
    (void)lights;
    (void)camera;
    // these lines above are just for compiling and can be deleted

    // iterate over all triangles in TriangleList
    for (const auto& t : TriangleList) {
        Triangle newtriangle = t;
        (void)newtriangle;
        

        // transform vertex position to world space for interpolating
        std::array<Vector3f, 3> worldspace_pos;
        

        // Use vetex_shader to transform vertex attributes(position & normals) to
        // view port and set a new triangle
        

        // call rasterize_triangle()
        
    }
}

// 对顶点的某一属性插值
Vector3f Rasterizer::interpolate(float alpha, float beta, float gamma, const Eigen::Vector3f& vert1,
                                 const Eigen::Vector3f& vert2, const Eigen::Vector3f& vert3,
                                 const Eigen::Vector3f& weight, const float& Z)
{
    Vector3f interpolated_res;
    for (int i = 0; i < 3; i++) {
        interpolated_res[i] = alpha * vert1[i] / weight[0] + beta * vert2[i] / weight[1] +
                              gamma * vert3[i] / weight[2];
    }
    interpolated_res *= Z;
    return interpolated_res;
}

// 对当前三角形进行光栅化
void Rasterizer::rasterize_triangle(const Triangle& t, const std::array<Vector3f, 3>& world_pos,
                                    GL::Material material, const std::list<Light>& lights,
                                    Camera camera)
{
    // these lines below are just for compiling and can be deleted
    (void)t;
    (void)world_pos;
    (void)material;
    (void)lights;
    (void)camera;
    // these lines above are just for compiling and can be deleted

    // discard all pixels out of the range(including x,y,z)

    
    
    // if current pixel is in current triange:
    // 1. interpolate depth(use projection correction algorithm)
    // 2. interpolate vertex positon & normal(use function:interpolate())
    // 3. fragment shading(use function:fragment_shader())
    // 4. set pixel
            
}

// 初始化整个光栅化渲染器
void Rasterizer::clear(BufferType buff)
{
    if ((buff & BufferType::Color) == BufferType::Color) {
        fill(frame_buf.begin(), frame_buf.end(), RenderEngine::background_color * 255.0f);
    }
    if ((buff & BufferType::Depth) == BufferType::Depth) {
        fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

Rasterizer::Rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

// 给定像素坐标(x,y)，计算frame buffer里对应的index
int Rasterizer::get_index(int x, int y)
{
    return (height - 1 - y) * width + x;
}

// 给定像素点以及fragement shader得到的结果，对frame buffer中对应存储位置进行赋值
void Rasterizer::set_pixel(const Vector2i& point, const Vector3f& res)
{
    int idx        = get_index(point.x(), point.y());
    frame_buf[idx] = res;
}
