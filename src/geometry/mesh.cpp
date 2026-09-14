#include "mesh.hpp"

#include <cstring>
#include <unordered_set>
#include <utility>

using Eigen::Vector3f;
using std::array;
using std::memcpy;
using std::unordered_set;
using std::vector;

// -------------------- Mesh --------------------
void Mesh::clear() noexcept
{
    positions.clear();
    normals.clear();
    faces.clear();
}

void Mesh::fill_buffers()
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
void LineSet::clear() noexcept
{
    positions.clear();
    lines.clear();
}

void LineSet::fill_buffers()
{
    position_buffer.resize(positions.size() * 3);
    memcpy(position_buffer.data(), positions.data(), positions.size() * sizeof(Vector3f));

    line_index_buffer.resize(lines.size() * 2);
    memcpy(line_index_buffer.data(), lines.data(), lines.size() * sizeof(array<unsigned int, 2>));
}
