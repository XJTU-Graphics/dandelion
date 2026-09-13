#include "mesh.hpp"

#include <cstring>

#include <Eigen/Geometry>

using Eigen::Quaternionf;
using Eigen::Vector3f;
using std::array;

// -------------------- Mesh --------------------
Mesh::Mesh(const Mesh& other) :
    positions(other.positions), normals(other.normals), faces(other.faces)
{
}

void Mesh::clear() noexcept
{
    positions.clear();
    normals.clear();
    faces.clear();
    modified = true;
}

// -------------------- LineSet --------------------
void LineSet::add_line(const Vector3f& from, const Vector3f& to)
{
    const unsigned int index = positions.size();
    positions.emplace_back(from);
    positions.emplace_back(to);
    lines.push_back({index, index + 1});
    modified = true;
}

size_t LineSet::n_lines() const noexcept
{
    return lines.size();
}

void LineSet::clear() noexcept
{
    positions.clear();
    lines.clear();
    modified = true;
}

// -------------------- ArrowSet --------------------
constexpr size_t                               n_arrow_vertices = 6;
const static array<Vector3f, n_arrow_vertices> arrow_vertices   = {
    Vector3f(0.0f, 0.0f, 0.0f),   Vector3f(1.0f, 0.0f, 0.0f),  Vector3f(0.8f, 0.02f, 0.0f),
    Vector3f(0.8f, -0.02f, 0.0f), Vector3f(0.8f, 0.0f, 0.02f), Vector3f(0.8f, 0.0f, -0.02f)
};
constexpr array<size_t, 10> arrow_lines     = {0, 1, 1, 2, 1, 3, 1, 4, 1, 5};
constexpr size_t            lines_pre_arrow = arrow_lines.size() / 2;
const static Vector3f       base_direction(1.0f, 0.0f, 0.0f);

void ArrowSet::add_arrow(const Vector3f& from, const Vector3f& to)
{
    const Vector3f    direction  = (to - from).normalized();
    const Quaternionf rotation   = Quaternionf::FromTwoVectors(base_direction, direction);
    const float       length     = (to - from).norm();
    const size_t      index_base = positions.size();
    for (const Vector3f& v: arrow_vertices) {
        const Vector3f v_transformed = length * (rotation * v) + from;
        positions.emplace_back(v_transformed);
    }
    for (size_t i = 0; i < arrow_lines.size(); i += 2) {
        const unsigned int index1 = static_cast<unsigned int>(index_base + arrow_lines[i]);
        const unsigned int index2 = static_cast<unsigned int>(index_base + arrow_lines[i + 1]);
        lines.push_back({index1, index2});
    }
    modified = true;
}

void ArrowSet::update_arrow(size_t index, const Vector3f& from, const Vector3f& to)
{
    const Vector3f    direction    = (to - from).normalized();
    const Quaternionf rotation     = Quaternionf::FromTwoVectors(base_direction, direction);
    const float       length       = (to - from).norm();
    size_t            vertex_index = index * n_arrow_vertices;
    for (const Vector3f& v: arrow_vertices) {
        positions[vertex_index] = length * (rotation * v) + from;
        ++vertex_index;
    }
    modified = true;
}

size_t ArrowSet::n_arrows() const noexcept
{
    return n_lines() / lines_pre_arrow;
}

// -------------------- AABBSet --------------------
void AABBSet::add_AABB(const Vector3f& p_min, const Vector3f& p_max)
{
    const float        x[2]       = {p_min.x(), p_max.x()};
    const float        y[2]       = {p_min.y(), p_max.y()};
    const float        z[2]       = {p_min.z(), p_max.z()};
    const unsigned int base_index = static_cast<unsigned int>(positions.size());
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                positions.emplace_back(x[i], y[j], z[k]);
            }
        }
    }
    // 4 lines on the x = p_min.x() plane.
    lines.push_back({base_index + 0u, base_index + 1u});
    lines.push_back({base_index + 0u, base_index + 2u});
    lines.push_back({base_index + 1u, base_index + 3u});
    lines.push_back({base_index + 2u, base_index + 3u});
    // 4 lines on the x = p_max.x() plane.
    lines.push_back({base_index + 4u, base_index + 5u});
    lines.push_back({base_index + 4u, base_index + 6u});
    lines.push_back({base_index + 5u, base_index + 7u});
    lines.push_back({base_index + 6u, base_index + 7u});
    // 4 lines between the two planes.
    lines.push_back({base_index + 0u, base_index + 4u});
    lines.push_back({base_index + 1u, base_index + 5u});
    lines.push_back({base_index + 2u, base_index + 6u});
    lines.push_back({base_index + 3u, base_index + 7u});

    modified = true;
}
