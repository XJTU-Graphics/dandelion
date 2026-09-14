#pragma once

#include <array>
#include <vector>

#include <Eigen/Core>

/*!
 * \file geometry/mesh.hpp
 * \~chinese
 * 提供用于渲染数据同步的内存 Mesh 数据结构。
 */

/*!
 * \ingroup geometry
 * \ingroup rendering
 * \~chinese
 * \brief 用于同步渲染数据的三角形 Mesh 。
 *
 * 这个类负责表示可以直接同步到 GPU 的三角形 Mesh 数据。
 *
 * 外界读取 Mesh 中的顶点坐标、法线，以及边和三角形的索引时一般应该使用
 * `positions`/`normals` 等存储向量或定长数组的容器；而渲染器需要向 GPU
 * 同步数据时，则应该直接使用存储 `float` 或 `unsigned int` 的扁平 `vector` 。
 */
struct Mesh
{
    /*! \~chinese 默认构造一个空的 Mesh 。 */
    Mesh() = default;
    /*! \~chinese 默认复制另一个 Mesh 中所有的数据。 */
    Mesh(const Mesh& other) = default;
    /*! \~chinese 调用各成员的移动构造。 */
    Mesh(Mesh&& other) = default;
    /*!
     * \~chinese
     * \brief 清空 Mesh 中所有的图元数据。
     *
     * 该方法只清空直接保存数据的容器（公有成员），不会清空用于向 GPU
     * 传输数据的 buffer （私有成员）。
     */
    void clear() noexcept;
    /*!
     * \~chinese
     * \brief 重新填充所有的 buffer ，确保数据可以被传输到 GPU 。
     */
    void fill_buffers();

    /*! \~chinese 顶点坐标。 */
    std::vector<Eigen::Vector3f> positions;
    /*! \~chinese 顶点法线。 */
    std::vector<Eigen::Vector3f> normals;
    /*! \~chinese 面片的顶点索引。 */
    std::vector<std::array<unsigned int, 3>> faces;

    /*! \~chinese 用于同步 GPU 的顶点坐标 buffer 。 */
    std::vector<float> position_buffer;
    /*! \~chinese 用于同步 GPU 的顶点法线 buffer 。 */
    std::vector<float> normal_buffer;
    /*! \~chinese 用于同步 GPU 的边线索引 buffer ，只在需要显示边时才有用。 */
    std::vector<unsigned int> edge_index_buffer;
    /*! \~chinese 用于同步 GPU 的三角形面片索引 buffer 。 */
    std::vector<unsigned int> face_index_buffer;
};

/*!
 * \ingroup geometry
 * \ingroup rendering
 * \~chinese
 * \brief 用于同步线条渲染数据的内存对象。
 *
 * 这个类负责表示可以直接同步到 GPU 的线条数据。
 *
 * 外界读取顶点和线条数据时一般应该选择 `positions`/`edges`
 * 属性，而渲染器需要向 GPU 同步数据时而渲染器需要向 GPU 同步数据时则应该直接使用扁平的 `vector` 。
 */
struct LineSet
{
    /*! \~chinese 默认构造一个空的 LineSet 。 */
    LineSet() = default;
    /*! \~chinese 默认复制另一个 LineSet 中的所有数据。 */
    LineSet(const LineSet& other) = default;
    /*! \~chinese 调用各成员的移动构造。 */
    LineSet(LineSet&& other) = default;
    /*!
     * \~chinese
     * \brief 清空 Mesh 中所有的图元数据。
     *
     * 该方法只清空直接保存数据的容器（公有成员），不会清空用于向 GPU
     * 传输数据的 buffer （私有成员）。
     */
    void clear() noexcept;
    /*!
     * \~chinese
     * \brief 重新填充所有的 buffer ，确保数据可以被传输到 GPU 。
     */
    void fill_buffers();

    /*! \~chinese 顶点坐标。 */
    std::vector<Eigen::Vector3f> positions;
    /*! \~chinese 线条的顶点索引。 */
    std::vector<std::array<unsigned int, 2>> lines;

    /*! \~chinese 用于同步 GPU 的顶点坐标 buffer 。 */
    std::vector<float> position_buffer;
    /*! \~chinese 用于同步 GPU 的线条索引 buffer 。 */
    std::vector<unsigned int> line_index_buffer;
};
