#include "gl.hpp"

#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../utils/rendering.hpp"

using namespace GL;
using Eigen::Quaternionf;
using Eigen::Vector3f;
using std::size_t;
using std::string;

VertexArrayObject::VertexArrayObject()
{
    glGenVertexArrays(1, &descriptor);
}

VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) : descriptor(other.descriptor)
{
    other.descriptor = 0;
}

VertexArrayObject::~VertexArrayObject()
{
    if (descriptor != 0) {
        glDeleteVertexArrays(1, &descriptor);
    }
}

void VertexArrayObject::bind() const noexcept
{
    glBindVertexArray(descriptor);
}

void VertexArrayObject::release() const noexcept
{
    glBindVertexArray(0);
}

void VertexArrayObject::draw(GLenum mode, int first, size_t n_vertices) const
{
    glBindVertexArray(descriptor);
    glDrawArrays(mode, first, GLsizei(n_vertices));
    glBindVertexArray(0);
}

DrawableMesh::DrawableMesh() :
    positions(GL_DYNAMIC_DRAW, vertex_position_location),
    normals(GL_DYNAMIC_DRAW, vertex_normal_location), edges(GL_DYNAMIC_DRAW),
    triangles(GL_DYNAMIC_DRAW)
{
    // Record VBO binding state to VAO
    VAO.bind();
    positions.bind();
    positions.specify_vertex_attribute();
    normals.bind();
    normals.specify_vertex_attribute();
    VAO.release();
}

void DrawableMesh::clear() noexcept
{
    positions.data.clear();
    normals.data.clear();
    edges.data.clear();
    triangles.data.clear();
}

void DrawableMesh::to_gpu() const noexcept
{
    VAO.bind();
    positions.to_gpu();
    normals.to_gpu();
    edges.to_gpu();
    edges.release();
    triangles.to_gpu();
    VAO.release();
}

DrawableLineSet::DrawableLineSet() :
    positions(GL_DYNAMIC_DRAW, vertex_position_location), lines(GL_DYNAMIC_DRAW)
{
    VAO.bind();
    positions.bind();
    positions.specify_vertex_attribute();
    VAO.release();
}

void DrawableLineSet::clear() noexcept
{
    positions.data.clear();
    lines.data.clear();
}

void DrawableLineSet::to_gpu() const
{
    VAO.bind();
    positions.to_gpu();
    lines.to_gpu();
    VAO.release();
}
