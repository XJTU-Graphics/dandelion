#pragma once

#include <cstddef>
#include <type_traits>
#include <vector>

#include <Eigen/Core>
#ifdef _WIN32
    #include <Windows.h>
#endif
#include <glad/glad.h>

/*!
 * \file platform/gl.hpp
 * \~chinese
 * 提供 OpenGL API 的封装和一些向 GPU 传递数据的工具。
 */

/*!
 * \ingroup platform
 * \~chinese
 * \brief 所有对 OpenGL 的封装类型和工具函数均在此命名空间中。
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
    void bind() const noexcept;
    /*! \~chinese 解绑 VAO。 */
    void release() const noexcept;
    /*! \~chinese 绘制这个 VAO 记录的所有内容，无需专门绑定和解绑。 */
    void draw(GLenum mode, int first, std::size_t count);

    /*! \~chinese OpenGL VAO 的名字 (name)，是该 VAO 的唯一标识。 */
    unsigned int descriptor;
};

/*!
 * \ingroup platform
 * \~chinese
 * \brief 对 OpenGL 数组缓冲 (Array Buffer) 的封装。
 *
 * Array Buffer 通常用于创建顶点缓冲对象 (Vertex Buffer Object, VBO)，用于存储顶点属性。
 * 与 `VertexArrayObject` 对象不同的是，`ArrayBuffer` 对象持有内存数据副本。
 * 如果希望更新显存中的数据，首先应当直接修改它持有的 `data` 成员，然后调用 `to_gpu`
 * 复制到显存。
 * \tparam T 此缓冲区中存放的数据类型，应当指定为基本数据类型，否则行为未定义。
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
    /*! \~chinese 禁止拷贝构造。 */
    ArrayBuffer(const ArrayBuffer& other) = delete;
    /*! \~chinese 禁止拷贝赋值。 */
    ArrayBuffer& operator=(ArrayBuffer& other) = delete;
    /*! \~chinese
     * 为满足 MoveInsertable 编写的移动构造函数，参考 `VertexArrayObject` 的移动构造函数。
     */
    ArrayBuffer(ArrayBuffer&& other);
    /*! \~chinese 调用 `glDeleteBuffers` 删除 Array Buffer。 */
    ~ArrayBuffer();
    /*! \~chinese
     * 将 `size` 个数据附加到现有数据的末尾。
     */
    template<typename... Ts>
    void append(Ts... values);
    /*!
     * \~chinese
     * \brief 更新指定位置的 3 个 `float` 数据，仅限 `T = float` 时使用，否则行为未定义。
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
    void bind() const noexcept;
    /*! \~chinese 解绑 ArrayBuffer。 */
    void release() const noexcept;
    /*! \~chinese 指定数据格式并使 `layout_location` 位置的属性生效。 */
    void specify_vertex_attribute() const noexcept;
    /*! \~chinese 使 `layout_location` 位置的属性失效。 */
    void disable() const noexcept;
    /*! \~chinese 将数据传送到 GPU，已经包含了绑定操作，但不包含解绑操作。 */
    void to_gpu() const noexcept;

    /*! \~chinese OpenGL Array Buffer 的名字 (name)，是它的唯一标识。 */
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
    /*! \~chinese 内存中的数据副本。 */
    std::vector<T> data;
};

/*!
 * \ingroup platform
 * \~chinese
 * \brief 对 OpenGL 索引数组缓冲 (Element Array Buffer) 的封装。
 *
 * Element Array Buffer 通常用于创建索引缓冲对象 (Element Buffer Object, EBO)，用于存储顶点索引。
 * EBO 通常保存边或者面对应的顶点索引，从而避免多次存储重复的顶点数据。
 * `ElementArrayBuffer` 对象持有内存索引数据副本。如果希望更新显存中的数据，
 * 首先应当直接修改它的 `data` 成员，然后调用 `to_gpu` 复制到显存。
 *
 * 遵循 OpenGL 的规范，`ElementArrayBuffer` 只允许使用 `unsigned int` 存储索引，
 * 不允许指定其他的索引数据类型。
 * \tparam size 每个基元（边或者面）对应的顶点索引个数，例如三角形面的 `size` 是 3。
 */
template<std::size_t size>
struct ElementArrayBuffer
{
    /*!
     * \~chinese
     * \brief 构造一个 EBO 对象。
     *
     * \param buffer_usage 该对象的使用方式，可以是 `GL_STATIC_DRAW` / `GL_DYNAMIC_DRAW`
     * / `GL_STREAM_DRAW` 其中之一。该信息只作为 hint，没有强制力。
     */
    ElementArrayBuffer(unsigned int buffer_usage);
    /*! \~chinese 禁止拷贝构造。 */
    ElementArrayBuffer(const ElementArrayBuffer& other) = delete;
    /*! \~chinese 禁止拷贝赋值。 */
    ElementArrayBuffer& operator=(ElementArrayBuffer& other) = delete;
    /*! \~chinese 参考 `VertexArrayObject` 的移动构造函数。 */
    ElementArrayBuffer(ElementArrayBuffer&& other);
    /*!
     * \~chinese
     * \brief 调用 `glDeleteBuffers` 销毁该 EBO。
     */
    ~ElementArrayBuffer();
    /*! \~chinese 将 `size` 个数据附加到末尾。 */
    template<typename... Ts>
    void append(Ts... values);
    /*! \~chinese 统计总共有多少个 **基元** （而不是顶点）。 */
    std::size_t count() const;
    /*! \~chinese 绑定该 EBO。 */
    void bind() const noexcept;
    /*! \~chinese 解绑该 EBO。 */
    void release() const noexcept;
    /*! \~chinese 将内存数据复制到显存，已包含绑定操作，但不包含解绑操作。 */
    void to_gpu() const noexcept;

    /*! \~chinese OpenGL EBO 名字 (name)，该对象的唯一标识。 */
    unsigned int descriptor;
    /*! \~chinese 该 EBO 的使用方式，见构造函数说明。 */
    unsigned int usage;
    /*!
     * \~chinese
     * \brief 内存中的索引数据副本。
     *
     * 使用 `unsigned int` 而非 `size_t` 的原因是 OpenGL 不接受 `size_t`。
     * */
    std::vector<unsigned int> data;
};

/*!
 * \~chinese
 * \brief 保存 GPU 所需几何数据的渲染 Mesh 。
 *
 * `DrawableMesh` 只保存渲染所需的几何数据、提供向显存复制数据的方法，
 * 不包含设置 uniform 、发起 draw call 等渲染操作。
 */
struct DrawableMesh
{
    /*! \~chinese 默认构造一个空的 `DrawableMesh` ，只在 VAO 中记录 VBO 的绑定。 */
    DrawableMesh();
    /*! \~chinese 禁止拷贝构造。 */
    DrawableMesh(const DrawableMesh& other) = delete;
    /*! \~chinese 禁止拷贝赋值。 */
    DrawableMesh& operator=(const DrawableMesh& other) = delete;
    /*! \~chinese 清空内存数据副本。 */
    void clear() noexcept;
    /*! \~chinese 将顶点和索引数据复制到显存。 */
    void to_gpu() const noexcept;

    /*! \~chinese OpenGL VAO 对象。 */
    VertexArrayObject VAO;
    /*! \~chinese 顶点坐标。 */
    ArrayBuffer<float, 3> positions;
    /*! \~chinese 顶点法线。 */
    ArrayBuffer<float, 3> normals;
    /*! \~chinese 表示边的顶点索引。 */
    ElementArrayBuffer<2> edges;
    /*! \~chinese 三角形顶点索引。 */
    ElementArrayBuffer<3> triangles;
};

/*!
 * \~chinese
 * \brief 保存 GPU 所需几何数据的渲染线条集。
 *
 * `DrawableLineSet` 类似 `DrawableMesh` 但更简单，只有简单的顶点坐标和线条。
 */
struct DrawableLineSet
{
    /*! \~chinese 默认构造一个空的 `DrawableSet` ，只在 VAO 中记录 VBO 的绑定。 */
    DrawableLineSet();
    /*! \~chinese 禁止拷贝构造。 */
    DrawableLineSet(const DrawableLineSet& other) = delete;
    /*! \~chinese 禁止拷贝赋值。 */
    DrawableLineSet& operator=(const DrawableLineSet& other) = delete;
    /*! \~chinese 清空内存数据副本。 */
    void clear() noexcept;
    /*! \~chinese 将顶点和索引数据复制到显存。 */
    void to_gpu() const;

    /*! \~chinese OpenGL VAO 对象。 */
    VertexArrayObject VAO;
    /*! \~chinese 顶点坐标。 */
    ArrayBuffer<float, 3> positions;
    /*! \~chinese 线段顶点索引。 */
    ElementArrayBuffer<2> lines;
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
ArrayBuffer<T, size>::ArrayBuffer(GLenum buffer_usage, unsigned int layout_location) :
    usage(buffer_usage), layout_location(layout_location)
{
    glGenBuffers(1, &(this->descriptor));
}

template<typename T, std::size_t size>
ArrayBuffer<T, size>::ArrayBuffer(ArrayBuffer&& other) :
    descriptor(other.descriptor), usage(other.usage), layout_location(other.layout_location),
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
    static_assert(
        (std::is_same_v<decltype(values), T> && ...),
        "ArrayBuffer: all values to be appended must have the same type as T"
    );
    static_assert(
        sizeof...(values) == size,
        "ArrayBuffer: number of values to be appended must be same as size per vertex"
    );
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
void ArrayBuffer<T, size>::bind() const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, this->descriptor);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::release() const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::specify_vertex_attribute() const noexcept
{
    GLenum data_type = get_GL_type_enum<T>();
    glVertexAttribPointer(
        this->layout_location, size, data_type, GL_FALSE, size * sizeof(T), (void*)0
    );
    glEnableVertexAttribArray(this->layout_location);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::disable() const noexcept
{
    glDisableVertexAttribArray(this->layout_location);
}

template<typename T, std::size_t size>
void ArrayBuffer<T, size>::to_gpu() const noexcept
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
ElementArrayBuffer<size>::ElementArrayBuffer(ElementArrayBuffer&& other) :
    descriptor(other.descriptor), usage(other.usage), data(std::move(other.data))
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
    static_assert(
        (std::is_same_v<decltype(values), unsigned int> && ...),
        "ElementArrayBuffer: all values to be appended must be unsigned int"
    );
    static_assert(
        sizeof...(values) == size,
        "ElementArrayBuffer: number of values to be appended must be as same as size per vertex"
    );
    (this->data.push_back(values), ...);
}

template<std::size_t size>
void ElementArrayBuffer<size>::bind() const noexcept
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->descriptor);
}

template<std::size_t size>
void ElementArrayBuffer<size>::release() const noexcept
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

template<std::size_t size>
void ElementArrayBuffer<size>::to_gpu() const noexcept
{
    this->bind();
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * this->data.size(), this->data.data(),
        this->usage
    );
}

} // namespace GL
