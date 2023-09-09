#include "rasterizer.h"

#include <array>
#include <limits>
#include <tuple>
#include <vector>
#include <algorithm>
#include <mutex>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "triangle.h"
#include "../utils/math.hpp"

using Eigen::Matrix4f;
using Eigen::Vector2i;
using Eigen::Vector3f;
using Eigen::Vector4f;

void Rasterizer::draw_mt(const std::vector<Triangle>& TriangleList, const GL::Material& material,
                         const std::list<Light>& lights, const Camera& camera)
{
    // these lines below are just for compiling and can be deleted
    (void)TriangleList;
    (void)material;
    (void)lights;
    (void)camera;
    // these lines above are just for compiling and can be deleted
}

// Screen space rasterization
void Rasterizer::rasterize_triangle_mt(const Triangle& t, const std::array<Vector3f, 3>& world_pos,
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
}
