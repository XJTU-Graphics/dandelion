#ifndef DANDELION_PLATFORM_GL_HPP
#define DANDELION_PLATFORM_GL_HPP

#include <cstddef>
#include <type_traits>
#include <vector>
#include <string>
#include <array>

#include <Eigen/Core>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>

#include "shader.hpp"
#include "../utils/rendering.hpp"

/*!
 * \file platform/gl.hpp
 */

/*!
 * \ingroup platform
 */
namespace GL {

/* --------------------------------------------------------
 * The definition region.
 * --------------------------------------------------------
 */

/*!
 * \ingroup platform
 * \~chinese
 * \brief 获取某个基本数据类型对应的枚举值（用于传递给某些 OpenGL API）。
 * \tparam DataType 指定的基本数据类型
 * \returns 如果是基本数据类型，则返回对应的枚举值；如果不是，则返回 GL_NONE。
 */
template<typename DataType>
constexpr GLenum get_GL_type_enum();

/*!
 * \ingroup platform
 * \~chinese
 * \brief 对 OpenGL 顶点数组对象 (Vertex Array Object) 的封装。
 *
 * 这个结构体只对 VAO 做了最基本的封装，稍微简化了手动构造和使用 VAO
 * 的过程。由于此结构体的实例将持有 OpenGL VAO 的名字 (name) 或者叫描述符，它不允许被复制构造。
 */
struct VertexArrayObject
{
    /*! \~chinese
     *  构造函数将调用 glGenVertexArrays 创建一个 OpenGL VAO，其名字存储于 `descriptor` 属性中。
     */
    VertexArrayObject();
    VertexArrayObject(VertexArrayObject& other)            = delete;
    VertexArrayObject& operator=(VertexArrayObject& other) = delete;
    /*! \~chinese
     * 移动构造函数是为满足 MoveInsertable 条件而写的，保证持有 VAO 的其他对象能使用 `std::vector`
     * 之类的容器存储。此构造函数会将 `other` 的 `descriptor` 设置成 0，从而避免 `other`
     * 析构时将真正的 OpenGL VAO 删除掉。
     */
    VertexArrayObject(VertexArrayObject&& other);
    /*! \~chinese 调用 glDeleteVertexArrays 删除 VAO。 */
    ~VertexArrayObject();
    /*! \~chinese 绑定 VAO，仅用于更新它持有的 buffer 数据或格式时才需要专门调用。 */
    void bind();
    /*! \~chinese 解绑 VAO。 */
    void release();
    /*! \~chinese 绘制这个 VAO 记录的所有内容，无需专门绑定和解绑。 */
    void draw(GLenum mode, int first, std::size_t count);

    unsigned int descriptor;
};

/*!
 * \ingroup platform
 * \~chinese
 * \brief 对 OpenGL 数组缓冲 (Array Buffer) 的封装。
 *
 * Array Buffer 通常用于创建顶点缓冲对象 (Vertex Buffer Object, VBO)，用于存储顶点属性。
 * 与 VertexArrayObject 对象不同的是，ArrayBuffer 对象持有数据。如果希望更新一个
 * ArrayBuffer 中的数据，首先应当直接修改它持有的 `std::vector<T>`，然后调用 `to_gpu`
 * 复制到显存。
 * \tparam T 此缓冲区中存放的数据类型，应当指定为基本数据类型，否则没有实际意义。
 * \tparam size 每个顶点的数据个数，例如 size 是 3 表示缓冲区中每三个数据是一组，
 * 这一组数据属于同一个顶点。
 */
template<typename T, std::size_t size>
struct ArrayBuffer
{
    /*! \~chinese
     * 调用 glGenBuffers 创建 Array Buffer，并设置此缓冲区绘制时的 hint 信息。
     * \param buffer_usage 绘制 hint 信息，参考 `usage` 属性。
     */
    ArrayBuffer(GLenum buffer_usage, unsigned int layout_location);
    ArrayBuffer(const ArrayBuffer& other)      = delete;
    ArrayBuffer& operator=(ArrayBuffer& other) = delete;
    /*! \~chinese
     * 为满足 MoveInsertable 编写的移动构造函数，参考 `VertexArrayObject` 的移动构造函数。
     */
    ArrayBuffer(ArrayBuffer&& other);
    /*! \~chinese 调用 glDeleteBuffers 删除 Array Buffer。 */
    ~ArrayBuffer();
    /*! \~chinese
     * 将 `size` 个数据附加到现有数据的末尾。
     */
    template<typename... Ts>
    void append(Ts... values);
    /*!
     * \~chinese
     * \brief 更新指定位置的 `size` 个数据。
     *
     * \param index 要更新的顶点索引
     * \param value 新的值
     */
    void update(size_t index, const Eigen::Vector3f& value);
    /*! \~chinese
     * 统计这个 `ArrayBuffer` 中有多少个顶点的数据，也就是数据个数除以 `size`。
     */
    std::size_t count() const;
    /*! \~chinese 绑定 ArrayBuffer。 */
    void bind();
    /*! \~chinese 解绑 ArrayBuffer。 */
    void release();
    /*! \~chinese 指定数据格式并使该 location 位置的属性生效。 */
    void specify_vertex_attribute();
    /*! \~chinese 使该 location 位置的属性无效。 */
    void disable();
    /*! \~chinese 将数据传送到 GPU，调用前无需绑定。 */
    void to_gpu();

    unsigned int descriptor;
    /*! \~chinese
     * 绘制时的 hint 信息，可以是 `GL_STATIC_DRAW / GL_DYNAMIC_DRAW / GL_STREAM_DRAW`
     * 其中之一。根据 OpenGL 标准，这只是一个提示信息，不具有任何强制性。
     */
    unsigned int usage;
    /*! \~chinese
     * 这个 ArrayBuffer 存储的属性在 vertex shader 中对应的位置。
     */
    unsigned int layout_location;
    std::vector<T> data;
};

/*!
 * \ingroup platform
 * \~chinese
 * \brief 对 OpenGL 索引数组缓冲 (Element Array Buffer) 的封装。
 *
 * Element Array Buffer 通常用于创建索引缓冲对象 (Element Buffer Object, EBO)，用于存储顶点索引。
 * EBO 通常保存边或者面对应的顶点索引，从而避免直接存储数据。
 * `ElementArrayBuffer` 对象持有数据。如果希望更新一个 `ElementArrayBuffer`
 * 中的数据，首先应当直接修改它的 `data` 成员，然后调用 `to_gpu` 复制到显存。
 * \tparam size 每个基元（边或者面）对应的顶点索引个数，例如三角形面的 `size` 是 3。
 */
template<std::size_t size>
struct ElementArrayBuffer
{
    ElementArrayBuffer(unsigned int buffer_usage);
    ElementArrayBuffer(const ElementArrayBuffer& other)      = delete;
    ElementArrayBuffer& operator=(ElementArrayBuffer& other) = delete;
    /*! \~chinese 参考 `VertexArrayObject` 的移动构造函数。 */
    ElementArrayBuffer(ElementArrayBuffer&& other);
    ~ElementArrayBuffer();
    /*! \~chinese 将 `size` 个数据附加到末尾。 */
    template<typename... Ts>
    void append(Ts... values);
    /*! \~chinese 统计总共有多少个 **基元** （而不是顶点）。 */
    std::size_t count() const;
    void bind();
    void release();
    void to_gpu();

    unsigned int descriptor;
    unsigned int usage;
    /*! \~chinese 使用 `unsigned int` 而非 `size_t` 的原因是 OpenGL 不接受 `size_t`。 */
    std::vector<unsigned int> data;
};

/*!
 * \ingroup platform
 * \ingroup rendering
 * \~chinese
 * \brief 物体材质。
 */
struct Material
{
    Material(const Eigen::Vector3f& K_ambient  = Eigen::Vector3f(1.0f, 1.0f, 1.0f),
             const Eigen::Vector3f& K_diffuse  = Eigen::Vector3f(0.5f, 0.5f, 0.5f),
             const Eigen::Vector3f& K_specular = Eigen::Vector3f(0.0f, 0.0f, 0.0f),
             float shininess                   = 5.0f);
    /*! \~chinese 环境光反射系数（颜色）。 */
    Eigen::Vector3f ambient;
    /*! \~chinese 漫反射光反射系数（颜色）。 */
    Eigen::Vector3f diffuse;
    /*! \~chinese 镜面反射光反射系数（颜色）。 */
    Eigen::Vector3f specular;
    /*! \~chinese Phong 模型计算镜面反射时的指数 */
    float shininess;
};

/*!
 * \~chinese
 * \brief 用于场景预览渲染的 Mesh 类。
 *
 * 这个类为了便于和 OpenGL 交互，不会使用 Eigen
 * 中的各种向量存储顶点坐标、法线和颜色等信息，而是直接持有 VAO、VBO 和 EBO。
 *
 * 由于 OpenGL API 只支持绘制三角形，GL::Mesh 存储的面片 (face) 只能是三角形。
 * 四边形乃至任意多边形面片需要先三角化成三角形才能被渲染。
 *
 * 外界读取 Mesh 中的顶点、边等基元时应当调用 `vertex/normal/edge/face` 方法，
 * 而不是直接访问 `vertices.data` 等内部存储。这些 ArrayBuffer 或 ElementArrayBuffer
 * 之所以被设为公有成员，是因为在修改数据或进行渲染时需要操作它们，
 * 其他情况下都不必也不应该使用这些扁平存储的数据。
 */
struct Mesh
{
    Mesh();
    /*! \~chinese 由于 VAO 和 ArrayBuffer 不允许复制构造，Mesh 也不允许复制构造。 */
    Mesh(const Mesh& other) = delete;
    Mesh(Mesh&& other);
    /*! \~chinese 读取编号为 index 的顶点。 */
    Eigen::Vector3f vertex(size_t index) const;
    /*! \~chinese 读取编号为 index 的顶点法线。 */
    Eigen::Vector3f normal(size_t index) const;
    /*! \~chinese 读取编号为 index 的边。 */
    std::array<size_t, 2> edge(size_t index) const;
    /*! \~chinese 读取编号为 index 的面片。 */
    std::array<size_t, 3> face(size_t index) const;
    void clear();
    void to_gpu();
    /*!
     * \~chinese
     * \brief 渲染这个 mesh。
     *
     * \param element_flags 指定渲染哪些元素的二进制串，可以是 `vertices_flag` / `edges_flag` /
     * `faces_flag` 中的任意一个或多个
     * \param face_shading 面片是否根据光照和材质进行着色，若否，则统一使用全局颜色。
     */
    void render(const Shader& shader, unsigned int element_flags, bool face_shading = true,
                const Eigen::Vector3f& global_color = default_wireframe_color);

    constexpr static unsigned int vertices_flag = 1u;
    constexpr static unsigned int edges_flag    = 1u << 1u;
    constexpr static unsigned int faces_flag    = 1u << 2u;
    const static Eigen::Vector3f default_wireframe_color;
    const static Eigen::Vector3f default_face_color;
    const static Eigen::Vector3f highlight_wireframe_color;
    const static Eigen::Vector3f highlight_face_color;
    VertexArrayObject VAO;
    ArrayBuffer<float, 3> vertices;
    ArrayBuffer<float, 3> normals;
    ElementArrayBuffer<2> edges;
    ElementArrayBuffer<3> faces;
    /*! \~chinese 每个 Mesh 只能有一个材质 */
    Material material;
};

/*!
 * \~chinese
 * \brief 在预览场景时绘制若干线条。
 *
 * 这个类与 `GL::Mesh` 相似但更简单，只有顶点 VBO 和线条 EBO，可用于绘制射线、
 * 半边等线条元素。
 */
struct LineSet
{
    LineSet(const std::string& name, Eigen::Vector3f color = GL::Mesh::default_wireframe_color);
    /*! \~chinese 由于 VAO 和 ArrayBuffer 不允许复制构造，LineSet 也不允许复制构造。 */
    LineSet(const LineSet& other) = delete;
    LineSet(LineSet&& other);
    /*! \~chinese 加入一条从 a 到 b 的线段。 */
    void add_line_segment(const Eigen::Vector3f& a, const Eigen::Vector3f& b);
    /*! \~chinese 加入一个从 from 到 to 的箭头。 */
    void add_arrow(const Eigen::Vector3f& from, const Eigen::Vector3f& to);
    /*! \~chinese 更新索引为 `index` 的箭头，仅当该 `LineSet` 内全部是箭头时才是安全的。 */
    void update_arrow(size_t index, const Eigen::Vector3f& from, const Eigen::Vector3f& to);
    /*! \~chinese 加入一个轴对齐包围盒 (Axis-Aligned Bouding Box, AABB) 。 */
    void add_AABB(const Eigen::Vector3f& p_min, const Eigen::Vector3f& p_max);
    /*! \~chinese 清空所有元素，但只影响内存，不会同步到显存。 */
    void clear();
    /*! \~chinese 将修改同步到显存。 */
    void to_gpu();
    /*!
     * \~chinese
     * \brief 渲染该线条集。
     *
     * 这个函数只会设置对应全局颜色的 uniform 变量，其他所有变量都需要由调用者自行设置。
     */
    void render(const Shader& shader);

    /*! \~chinese 绘制的线条颜色。 */
    Eigen::Vector3f line_color;
    VertexArrayObject VAO;
    ArrayBuffer<float, 3> vertices;
    ElementArrayBuffer<2> lines;
    std::string name;
};

/* ---------------------------------------------------------
 * The implementation region for template class and functions.
 * ---------------------------------------------------------
 */

template<typename DataType>
constexpr GLenum get_GL_type_enum()
{
    if constexpr (std::is_same_v<DataType, char>) {
        return GL_BYTE;
    } else if constexpr (std::is_same_v<DataType, unsigned char>) {
        return GL_UNSIGNED_BYTE;
    } else if constexpr (std::is_same_v<DataType, int>) {
        return GL_INT;
    } else if constexpr (std::is_same_v<DataType, unsigned int>) {
        return GL_UNSIGNED_INT;
    } else if constexpr (std::is_same_v<DataType, float>) {
        return GL_FLOAT;
    } else if constexpr (std::is_same_v<DataType, double>) {
        return GL_DOUBLE;
    } else {
        return GL_NONE;
    }
}

// ArrayBuffer ---------------------------------------------

template<typename T, std::size_t size>
ArrayBuffer<T, size>::ArrayBuffer(GLenum buffer_usage, unsigned int layout_location)
    : usage(buffer_usage), layout_location(layout_location)
{
    glGenBuffers(1, &(this->descriptor));
}

template<typename T, std::size_t size>
ArrayBuffer<T, size>::ArrayBuffer(ArrayBuffer&& other)
    : descriptor(other.descriptor), usage(other.usage), layout_location(other.layout_location),
      data(std::move(other.data))
{
    other.descriptor = 0;
}

template<typename T, std::size_t size>
ArrayBuffer<T, size>::~ArrayBuffer()
{
    if (this->descriptor != 0) {
        glDeleteBuffers(1, &(this->descriptor));
    }
}

template<typename T, std::size_t size>
template<typename... Ts>
void ArrayBuffer<T, size>::append(Ts... values)
{
    static_assert((std::is_same_v<decltype(values), T> && ...),
                  "ArrayBuffer: all values to be appended must have the same type as T");
    static_assert(sizeof...(values) == size,
                  "ArrayBuffer: number of values to be appended must be same as size per vertex");
    (this->data.push_back(values), ...);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::update(size_t index, const Eigen::Vector3f& value)
{
    const GLintptr offset = index * 3 * sizeof(float);
    data[index * 3]       = value.x();
    data[index * 3 + 1]   = value.y();
    data[index * 3 + 2]   = value.z();
    bind();
    glBufferSubData(GL_ARRAY_BUFFER, offset, 3 * sizeof(float), value.data());
}

template<typename T, std::size_t size>
std::size_t ArrayBuffer<T, size>::count() const
{
    return this->data.size() / size;
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, this->descriptor);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::release()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::specify_vertex_attribute()
{
    GLenum data_type = get_GL_type_enum<T>();
    glVertexAttribPointer(this->layout_location, size, data_type, GL_FALSE, size * sizeof(T),
                          (void*)0);
    glEnableVertexAttribArray(this->layout_location);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::disable()
{
    glDisableVertexAttribArray(this->layout_location);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::to_gpu()
{
    this->bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(T) * this->data.size(), this->data.data(), this->usage);
    this->specify_vertex_attribute();
}

// ElementArrayBuffer --------------------------------------

template<std::size_t size>
ElementArrayBuffer<size>::ElementArrayBuffer(unsigned int buffer_usage) : usage(buffer_usage)
{
    glGenBuffers(1, &(this->descriptor));
}

template<std::size_t size>
ElementArrayBuffer<size>::ElementArrayBuffer(ElementArrayBuffer&& other)
    : descriptor(other.descriptor), usage(other.usage), data(std::move(other.data))
{
    other.descriptor = 0;
}

template<std::size_t size>
ElementArrayBuffer<size>::~ElementArrayBuffer()
{
    if (this->descriptor != 0) {
        glDeleteBuffers(1, &(this->descriptor));
    }
}

template<std::size_t size>
std::size_t ElementArrayBuffer<size>::count() const
{
    return this->data.size() / size;
}

template<std::size_t size>
template<typename... Ts>
void ElementArrayBuffer<size>::append(Ts... values)
{
    static_assert((std::is_same_v<decltype(values), unsigned int> && ...),
                  "ElementArrayBuffer: all values to be appended must be unsigned int");
    static_assert(
        sizeof...(values) == size,
        "ElementArrayBuffer: number of values to be appended must be as same as size per vertex");
    (this->data.push_back(values), ...);
}

template<std::size_t size>
void ElementArrayBuffer<size>::bind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->descriptor);
}

template<std::size_t size>
void ElementArrayBuffer<size>::release()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

template<std::size_t size>
void ElementArrayBuffer<size>::to_gpu()
{
    this->bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * this->data.size(),
                 this->data.data(), this->usage);
}

} // namespace GL

#endif // DANDELION_PLATFORM_GL_HPP
