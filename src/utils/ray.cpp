#include "ray.h"

#include <cmath>
#include <array>

#include <Eigen/Dense>
#include <spdlog/spdlog.h>

#include "../utils/math.hpp"

using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::numeric_limits;
using std::optional;
using std::size_t;

constexpr float infinity = 1e5f;
constexpr float eps      = 1e-5f;

Intersection::Intersection() : t(numeric_limits<float>::infinity()), face_index(0)
{
}

Ray generate_ray(int width, int height, int x, int y, Camera& camera, float depth)
{
    // these lines below are just for compiling and can be deleted
    (void)width;
    (void)height;
    (void)x;
    (void)y;
    (void)depth;
    // these lines above are just for compiling and can be deleted


    // The ratio between the specified plane (width x height)'s depth and the image plane's depth.
    
    // Transfer the view-space position to world space.
    Vector3f world_pos;
    return {camera.position, (world_pos - camera.position).normalized()};
}

optional<Intersection> ray_triangle_intersect(const Ray& ray, const GL::Mesh& mesh, size_t index)
{
    // these lines below are just for compiling and can be deleted
    (void)ray;
    (void)mesh;
    (void)index;
    // these lines above are just for compiling and can be deleted
    Intersection result;
    
    if (result.t - infinity < -eps) {
        return result;
    } else {
        return std::nullopt;
    }
}

optional<Intersection> naive_intersect(const Ray& ray, const GL::Mesh& mesh, const Matrix4f model)
{
    // these lines below are just for compiling and can be deleted
    (void)ray;
    (void)model;
    // these lines above are just for compiling and can be deleted

    Intersection result;
    for (size_t i = 0; i < mesh.faces.count(); ++i) {
        // Vertex a, b and c are assumed to be in counterclockwise order.
        // Construct matrix A = [d, a - b, a - c] and solve Ax = (a - origin)
        // Matrix A is not invertible, indicating the ray is parallel with the triangle.
        // Test if alpha, beta and gamma are all between 0 and 1.
    }
    // Ensure result.t is strictly less than the constant `infinity`.
    if (result.t - infinity < -eps) {
        return result;
    }
    return std::nullopt;
}
