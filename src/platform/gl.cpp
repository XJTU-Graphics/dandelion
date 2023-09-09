#include "gl.hpp"

#include <cmath>

#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../utils/math.hpp"

using namespace GL;
using Eigen::Quaternionf;
using Eigen::Vector3f;
using std::array;
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

void VertexArrayObject::bind()
{
    glBindVertexArray(descriptor);
}

void VertexArrayObject::release()
{
    glBindVertexArray(0);
}

void VertexArrayObject::draw(GLenum mode, int first, size_t count)
{
    glBindVertexArray(descriptor);
    glDrawArrays(mode, first, GLsizei(count));
    glBindVertexArray(0);
}

Material::Material(const Vector3f& K_ambient, const Vector3f& K_diffuse, const Vector3f& K_specular,
                   float shininess)
    : ambient(K_ambient), diffuse(K_diffuse), specular(K_specular), shininess(shininess)
{
}

const Vector3f Mesh::default_wireframe_color(RGB(255, 194, 75));
const Vector3f Mesh::default_face_color(RGB(255, 255, 255));
const Vector3f Mesh::highlight_wireframe_color(RGB(115, 206, 244));
const Vector3f Mesh::highlight_face_color(RGB(115, 206, 244));

Mesh::Mesh()
    : vertices(GL_DYNAMIC_DRAW, vertex_position_location),
      normals(GL_DYNAMIC_DRAW, vertex_normal_location), edges(GL_DYNAMIC_DRAW),
      faces(GL_DYNAMIC_DRAW)
{
    VAO.bind();
    vertices.bind();
    normals.bind();
    glDisableVertexAttribArray(vertex_color_location);
    VAO.release();
}

Mesh::Mesh(Mesh&& other)
    : VAO(std::move(other.VAO)), vertices(std::move(other.vertices)),
      normals(std::move(other.normals)), edges(std::move(other.edges)),
      faces(std::move(other.faces)), material(std::move(other.material))
{
    VAO.bind();
    vertices.bind();
    normals.bind();
    glDisableVertexAttribArray(vertex_color_location);
    VAO.release();
}

/*
 * 由于这个 Mesh 类直接用 `std::vector<float>` 存储数据，直接访问数据比较不便。
 * 这个方法从存储信息的 `std::vector` 中打包一个三维向量。
 * \param data 指定要从哪个 `vector` 中获取数据
 * \param index 按每 `size` 个元素为一个向量计，指定要打包的向量是第几个
 */
Vector3f pack(const std::vector<float>& data, size_t index)
{
    return Vector3f(data.at(index * 3), data.at(index * 3 + 1), data.at(index * 3 + 2));
}

Vector3f Mesh::vertex(size_t index) const
{
    return pack(vertices.data, index);
}

Vector3f Mesh::normal(size_t index) const
{
    return pack(normals.data, index);
}

array<size_t, 2> Mesh::edge(size_t index) const
{
    return {edges.data.at(index * 2), edges.data.at(index * 2 + 1)};
}

array<size_t, 3> Mesh::face(size_t index) const
{
    return {faces.data.at(index * 3), faces.data.at(index * 3 + 1), faces.data.at(index * 3 + 2)};
}

void Mesh::clear()
{
    vertices.data.clear();
    normals.data.clear();
    edges.data.clear();
    faces.data.clear();
}

void Mesh::to_gpu()
{
    VAO.bind();
    vertices.to_gpu();
    normals.to_gpu();
    edges.to_gpu();
    edges.release();
    faces.to_gpu();
    faces.release();
    VAO.release();
}

void Mesh::render(const Shader& shader, unsigned int element_flags, bool face_shading,
                  const Vector3f& global_color)
{
    VAO.bind();
    if (element_flags & faces_flag) {
        if (face_shading) {
            normals.bind();
            normals.specify_vertex_attribute();
            shader.set_uniform("use_global_color", false);
            shader.set_uniform("material.ambient", material.ambient);
            shader.set_uniform("material.diffuse", material.diffuse);
            shader.set_uniform("material.specular", material.specular);
            shader.set_uniform("material.shininess", material.shininess);
        } else {
            normals.bind();
            normals.disable();
            shader.set_uniform("global_color", highlight_face_color);
        }
        faces.bind();
        glDrawElements(GL_TRIANGLES, GLsizei(faces.data.size()), GL_UNSIGNED_INT, (void*)0);
        faces.release();
    } else {
        normals.bind();
        normals.disable();
    }
    // Render some elements with a uniform color.
    shader.set_uniform("use_global_color", true);
    shader.set_uniform("global_color", global_color);
    if (element_flags & vertices_flag) {
        glDrawArrays(GL_POINTS, 0, GLsizei(vertices.count()));
    }
    if (element_flags & edges_flag) {
        edges.bind();
        glDrawElements(GL_LINES, GLsizei(edges.data.size()), GL_UNSIGNED_INT, (void*)0);
        edges.release();
    }
    VAO.release();
}

LineSet::LineSet(const string& name, Vector3f color)
    : line_color(color), vertices(GL_DYNAMIC_DRAW, vertex_position_location),
      lines(GL_DYNAMIC_DRAW), name(name)
{
    VAO.bind();
    vertices.bind();
    vertices.specify_vertex_attribute();
    glDisableVertexAttribArray(vertex_color_location);
    glDisableVertexAttribArray(vertex_normal_location);
    lines.bind();
    VAO.release();
}

LineSet::LineSet(LineSet&& other)
    : VAO(std::move(other.VAO)), vertices(std::move(other.vertices)), lines(std::move(other.lines)),
      name(std::move(other.name))
{
    VAO.bind();
    vertices.bind();
    vertices.specify_vertex_attribute();
    glDisableVertexAttribArray(vertex_color_location);
    glDisableVertexAttribArray(vertex_normal_location);
    lines.bind();
    VAO.release();
}

void LineSet::add_line_segment(const Vector3f& a, const Vector3f& b)
{
    unsigned int index = (unsigned int)(vertices.count());
    vertices.append(a.x(), a.y(), a.z());
    vertices.append(b.x(), b.y(), b.z());
    lines.append(index, index + 1);
}

constexpr size_t n_arrow_vertices                             = 6;
const static array<Vector3f, n_arrow_vertices> arrow_vertices = {
    Vector3f(0.0f, 0.0f, 0.0f),   Vector3f(1.0f, 0.0f, 0.0f),  Vector3f(0.8f, 0.02f, 0.0f),
    Vector3f(0.8f, -0.02f, 0.0f), Vector3f(0.8f, 0.0f, 0.02f), Vector3f(0.8f, 0.0f, -0.02f)};
const static array<size_t, 10> arrow_lines = {0, 1, 1, 2, 1, 3, 1, 4, 1, 5};
const static Vector3f base_direction(1.0f, 0.0f, 0.0f);

void LineSet::add_arrow(const Vector3f& from, const Vector3f& to)
{
    const Vector3f direction   = (to - from).normalized();
    const Quaternionf rotation = Quaternionf::FromTwoVectors(base_direction, direction);
    const float length         = (to - from).norm();
    const size_t index_base    = vertices.count();
    for (const Vector3f& v : arrow_vertices) {
        const Vector3f v_transformed = length * (rotation * v) + from;
        vertices.append(v_transformed.x(), v_transformed.y(), v_transformed.z());
    }
    for (const size_t index : arrow_lines) {
        lines.data.push_back((unsigned int)(index_base + index));
    }
}

void LineSet::update_arrow(size_t index, const Vector3f& from, const Vector3f& to)
{
    const Vector3f direction   = (to - from).normalized();
    const Quaternionf rotation = Quaternionf::FromTwoVectors(base_direction, direction);
    const float length         = (to - from).norm();
    size_t i                   = index * n_arrow_vertices;
    for (const Vector3f& v : arrow_vertices) {
        const Vector3f v_transformed = length * (rotation * v) + from;
        vertices.update(i, v_transformed);
        ++i;
    }
}

void LineSet::add_AABB(const Eigen::Vector3f& p_min, const Eigen::Vector3f& p_max)
{
    const float x[2]              = {p_min.x(), p_max.x()};
    const float y[2]              = {p_min.y(), p_max.y()};
    const float z[2]              = {p_min.z(), p_max.z()};
    const unsigned int base_index = static_cast<unsigned int>(vertices.count());
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                vertices.append(x[i], y[j], z[k]);
            }
        }
    }
    // 4 lines on the x = p_min.x() plane.
    lines.append(base_index + 0u, base_index + 1u);
    lines.append(base_index + 0u, base_index + 2u);
    lines.append(base_index + 1u, base_index + 3u);
    lines.append(base_index + 2u, base_index + 3u);
    // 4 lines on the x = p_max.x() plane.
    lines.append(base_index + 4u, base_index + 5u);
    lines.append(base_index + 4u, base_index + 6u);
    lines.append(base_index + 5u, base_index + 7u);
    lines.append(base_index + 6u, base_index + 7u);
    // 4 lines between the two planes.
    lines.append(base_index + 0u, base_index + 4u);
    lines.append(base_index + 1u, base_index + 5u);
    lines.append(base_index + 2u, base_index + 6u);
    lines.append(base_index + 3u, base_index + 7u);
}

void LineSet::clear()
{
    vertices.data.clear();
    lines.data.clear();
}

void LineSet::to_gpu()
{
    VAO.bind();
    vertices.to_gpu();
    lines.to_gpu();
    VAO.release();
}

void LineSet::render(const Shader& shader)
{
    VAO.bind();
    shader.set_uniform("use_global_color", true);
    shader.set_uniform("global_color", line_color);
    glDrawElements(GL_LINES, GLsizei(lines.data.size()), GL_UNSIGNED_INT, (void*)0);
    VAO.release();
}
