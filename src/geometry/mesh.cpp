#include "mesh.hpp"

#include <cstring>
#include <unordered_set>
#include <utility>

#include <Eigen/Geometry>

using Eigen::Quaternionf;
using Eigen::Vector3f;
using std::array;
using std::make_unique;
using std::memcpy;
using std::unordered_set;
using std::vector;

// -------------------- Mesh --------------------
Mesh::Mesh(const Mesh& other) :
    positions(other.positions), normals(other.normals), faces(other.faces),
    position_buffer(other.position_buffer), normal_buffer(other.normal_buffer),
    edge_index_buffer(other.edge_index_buffer), face_index_buffer(other.face_index_buffer)
{
    switch (other.material->type()) {
    case MaterialType::Phong:
        const PhongMaterial& reference = dynamic_cast<PhongMaterial&>(*other.material);
        material                       = make_unique<PhongMaterial>(reference);
        break;
    }
}

void Mesh::clear() noexcept
{
    positions.clear();
    normals.clear();
    faces.clear();
}

void Mesh::prepare_buffers()
{
    position_buffer.resize(positions.size() * 3);
    memcpy(position_buffer.data(), positions.data(), positions.size() * sizeof(Vector3f));

    // Collect all edges to build line segments for rendering
    const size_t                        half_n_vertices = (positions.size() + 1) / 2;
    vector<unordered_set<unsigned int>> graph;
    graph.resize(half_n_vertices);
    auto insert_edge = [this, &graph](unsigned int v1, unsigned int v2) -> void {
        if (v1 > v2) {
            std::swap(v1, v2);
        }
        if (graph[v1].contains(v2)) {
            return;
        }
        graph[v1].insert(v2);
        this->edge_index_buffer.push_back(v1);
        this->edge_index_buffer.push_back(v2);
    };
    for (const array<unsigned int, 3>& f: faces) {
        insert_edge(f[0], f[1]);
        insert_edge(f[1], f[2]);
        insert_edge(f[2], f[0]);
    }

    face_index_buffer.resize(faces.size() * 3);
    memcpy(face_index_buffer.data(), faces.data(), faces.size() * sizeof(array<unsigned int, 3>));
}

// -------------------- LineSet --------------------
void LineSet::add_line(const Vector3f& from, const Vector3f& to)
{
    const unsigned int index = positions.size();
    positions.emplace_back(from);
    positions.emplace_back(to);
    lines.push_back({index, index + 1});
}

size_t LineSet::n_lines() const noexcept
{
    return lines.size();
}

void LineSet::clear() noexcept
{
    positions.clear();
    lines.clear();
}

void LineSet::prepare_buffers()
{
    position_buffer.resize(positions.size() * 3);
    memcpy(position_buffer.data(), positions.data(), positions.size() * sizeof(Vector3f));

    line_index_buffer.resize(lines.size() * 2);
    memcpy(line_index_buffer.data(), lines.data(), lines.size() * sizeof(array<unsigned int, 2>));
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
    for (size_t index_offset = 0; index_offset < arrow_lines.size(); index_offset += 2) {
        const unsigned int index = static_cast<unsigned int>(index_base + index_offset);
        lines.push_back({index, index + 1});
    }
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
                positions.emplace_back(x[i], y[i], z[i]);
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
}
