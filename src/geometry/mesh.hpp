#pragma once

#include <array>
#include <vector>
#include <string>

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
    /*! \~chinese 复制另一个 Mesh 中所有的数据。 */
    Mesh(const Mesh& other);
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

    /*! \~chinese 顶点坐标。 */
    std::vector<Eigen::Vector3f> positions;
    /*! \~chinese 顶点法线。 */
    std::vector<Eigen::Vector3f> normals;
    /*! \~chinese 边的顶点索引。 */
    std::vector<std::array<unsigned int, 2>> edges;
    /*! \~chinese 面片的顶点索引。 */
    std::vector<std::array<unsigned int, 3>> faces;
    /*! \~chinese 相较于上次渲染，该 Mesh 是否被修改过。 */
    bool modified;
    /*! \~chinese 名称。 */
    std::string name;
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
 *
 * 线条只支持单色渲染，着色效果不受光照的影响。
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
     * 添加一条从 `from` 到 `to` 的线段。
     * \param from 起始点。
     * \param to 终止点。
     */
    void add_line(const Eigen::Vector3f& from, const Eigen::Vector3f& to);
    /*!
     * \~chinese
     * 统计当前总共有多少条线段。
     * \returns 线段总数。
     */
    std::size_t n_lines() const noexcept;
    /*!
     * \~chinese
     * \brief 清空 Mesh 中所有的图元数据。
     *
     * 该方法只清空直接保存数据的容器（公有成员），不会清空用于向 GPU
     * 传输数据的 buffer （私有成员）。
     */
    void clear() noexcept;

    /*! \~chinese 顶点坐标。 */
    std::vector<Eigen::Vector3f> positions;
    /*! \~chinese 线条的顶点索引。 */
    std::vector<std::array<unsigned int, 2>> lines;
    /*! \~chinese 线条的颜色，每个分量取值范围在 0 到 1 之间。 */
    Eigen::Vector3f color;
    /*! \~chinese 相较于上次渲染，该线条集是否被修改过。 */
    bool modified;
    /*! \~chinese 名称。 */
    std::string name;
};

/*!
 * \~chinese
 * \brief 用于渲染箭头的特殊线条集。
 *
 * `ArrowSet` 本质上还是一个 `LineSet` ，但提供了添加和更新箭头的便捷方法。
 * 这个类并不强制保证所有的线条能正确地组成箭头，所以使用 `ArrowSet` 时，
 * 应该尽可能避免直接更新 `positions` 和 `lines` ，只使用添加、更新、清空操作。
 *
 * 出于性能考虑，`ArrowSet` **不会强制检查其中的线段是否能组成箭头** 。
 * 如果随意插入线段，很可能导致更新和统计总数时得不到预期的结果。
 */
struct ArrowSet : public LineSet
{
    /*!
     * \~chinese
     * 添加一个箭头。
     * \param from 起始点。
     * \param to 终止点。
     */
    void add_arrow(const Eigen::Vector3f& from, const Eigen::Vector3f& to);
    /*!
     * \~chinese
     * \brief 更新指定的箭头。
     * \param index 线条集中全都是箭头的前提下，要更新的箭头索引。
     * \param from 起始点。
     * \param to 终止点。
     */
    void update_arrow(std::size_t index, const Eigen::Vector3f& from, const Eigen::Vector3f& to);
    /*!
     * \~chinese
     * 按每个箭头由 5 条线段组成计，统计当前的箭头总数。
     * \returns 箭头总数。
     */
    std::size_t n_arrows() const noexcept;
};

/*!
 * \~chinese
 * \brief 用于渲染轴对齐包围盒 (AABB) 的特殊线条集。
 *
 * 类似 `ArrowSet` ，`AABBSet` 也是一个不提供强制数据检查的 `LineSet` 。
 * 使用约定、注意事项与 `ArrowSet` 相似。
 */
struct AABBSet : public LineSet
{
    /*!
     * \~chinese
     * 添加一个 AABB 。
     * \param p_min 三轴坐标最小的点。
     * \param p_max 三轴坐标最大的点。
     */
    void add_AABB(const Eigen::Vector3f& p_min, const Eigen::Vector3f& p_max);
};
