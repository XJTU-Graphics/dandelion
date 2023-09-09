#ifndef DANDELION_GEOMETRY_HALFEDGE_H
#define DANDELION_GEOMETRY_HALFEDGE_H

#include <cstddef>
#include <set>
#include <memory>
#include <optional>
#include <variant>
#include <tuple>
#include <unordered_map>

#include <Eigen/Core>
#include <spdlog/spdlog.h>

#include "../platform/gl.hpp"
#include "../platform/shader.hpp"
#include "../utils/linked_list.hpp"
#include "../scene/object.h"

/*!
 * \file geometry/halfedge.h
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格所需各种类型的公共头文件。
 *
 * Dandelion 的几何处理算法都是基于半边网格的，所有几何处理需要的类型都在这个头文件中声明，
 * 但在不同的源文件中定义。`Halfedge` / `Vertex` / `Edge` / `Face` 各自有一个源文件，
 * 而 `HalfedgeMesh` 则被分为两部分：创建、同步和检查等功能在 *halfedge_mesh.cpp*
 * 中实现，各类几何操作在 *meshedit.cpp* 中实现。
 */

struct Halfedge;
struct Vertex;
struct Edge;
struct Face;

/*!
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格中最关键的几何元素。
 *
 * 半边网格中，所有几何元素都通过半边相互连接。Halfedge 类维护了每条半边的起点、所属的边和面片、
 * 在整个半边网格上的下一条（前一条、反向）半边，从而将所有的几何基本元素联系在一起。
 */
struct Halfedge : LinkedListNode<Halfedge>
{
    /*! \~chinese 仅供 `HalfedgeMesh::new_halfedge` 调用，其他任何情况下都不应该直接使用。 */
    Halfedge(std::size_t halfedge_id);
    /*! \~chinese 一次性设置半边的所有属性，各参数含义与同名属性一致。 */
    void set_neighbors(Halfedge* next, Halfedge* prev, Halfedge* inv, Vertex* from, Edge* edge,
                       Face* face);
    /*!
     * \~chinese
     * \brief 这条半边是否属于一个虚拟的边界面。
     *
     * 这个函数的返回值是 `face->on_boundary`，因此返回真时表示这条半边属于一个
     * “虚假的”面（表示边界而不是真正的面片）；而返回假时这条半边也有可能与 mesh
     * 边界相邻，所以函数名为 `is_boundary` 而不是 `on_boundary` 。
     */
    bool is_boundary() const;
    /*! \~chinese 半边的全局唯一 ID，不会与整个半边网格中任何其他元素重复。 */
    const std::size_t id;
    /*! \~chinese 下一条半边。 */
    Halfedge* next;
    /*! \~chinese 上一条半边。 */
    Halfedge* prev;
    /*! \~chinese 反方向的半边。 */
    Halfedge* inv;
    /*! \~chinese 半边的起点（发出点）。 */
    Vertex* from;
    /*! \~chinese 半边所在的边。 */
    Edge* edge;
    /*! \~chinese 半边所在的面片。 */
    Face* face;
};

/*!
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格中的顶点。
 *
 * 半边网格中，每个顶点只维护自身的坐标和某一条从自身发出的半边。
 */
struct Vertex : LinkedListNode<Vertex>
{
    /*! \~chinese 仅供 `HalfedgeMesh::new_vertex` 调用，其他任何情况下都不应该直接使用。 */
    Vertex(std::size_t vertex_id);
    /*! \~chinese 邻接的面片数量（不包括虚拟的边界面）。 */
    size_t degree() const;
    /*!
     * \~chinese
     * \brief \f$\mathcal{N}_1\f$ 邻域 (1-ring neighborhood) 中所有顶点坐标的算数平均值。
     */
    Eigen::Vector3f neighborhood_center() const;
    /*! \~chinese 以面积为权重对邻接面片法向量求平均给出的顶点法向估计值。 */
    Eigen::Vector3f normal() const;
    /*! \~chinese 顶点的全局唯一 ID，不会与整个半边网格中任何其他元素重复。 */
    const std::size_t id;
    /*! \~chinese 从这一顶点发出的某条半边。 */
    Halfedge* halfedge;
    /*! \~chinese 顶点坐标。 */
    Eigen::Vector3f pos;
    /*! \~chinese 新建顶点的标识，在一些全局操作中用到。 */
    bool is_new;
    /*! \~chinese 在迭代式调整顶点坐标时保存迭代后的坐标（例如 Loop 细分时调整后的坐标）。 */
    Eigen::Vector3f new_pos;
};

/*!
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格中的边。
 *
 * 半边网格中，每条边只维护属于自身的某一条半边。
 */
struct Edge : LinkedListNode<Edge>
{
    /*! \~chinese 仅供 `HalfedgeMesh::new_edge` 调用，其他任何情况下都不应该直接使用。 */
    Edge(std::size_t edge_id);
    /*! \~chinese 这条边是否在 mesh 边界上。 */
    bool on_boundary() const;
    /*! \~chinese 边的中点坐标。 */
    Eigen::Vector3f center() const;
    /*! \~chinese 边的长度。 */
    float length() const;
    /*! \~chinese 边的全局唯一 ID，不会与整个半边网格中任何其他元素重复。 */
    const std::size_t id;
    /*! \~chinese 沿着这条边的某一条半边。 */
    Halfedge* halfedge;
    /*! \~chinese 新建边的标识，在一些全局操作中用到。 */
    bool is_new;
    /*!
     * \~chinese
     * \brief 准备分裂这条边时分裂后新增顶点的坐标。
     *
     * `HalfedgeMesh::split_edge` 函数并不会使用这个坐标，而是直接用边中点作为新增顶点的位置。
     * 这个属性仅供需要指定分裂后顶点位置的算法使用，例如 Loop 细分。
     */
    Eigen::Vector3f new_pos;
};

/*!
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格中的面片。
 *
 * 半边网格中，每个面片只维护属于自身的某一条半边。
 */
struct Face : LinkedListNode<Face>
{
    /*! \~chinese 仅供 `HalfedgeMesh::new_face` 调用，其他任何情况下都不应该直接使用。 */
    Face(std::size_t face_id, bool is_boundary = false);
    /*! \~chinese 返回面片法向量与面片面积的数量积。 */
    Eigen::Vector3f area_weighted_normal();
    /*! \~chinese 返回面片法向量（已经单位化）。 */
    Eigen::Vector3f normal();
    /*! \~chinese 返回面片中心（也是重心，顶点坐标的算数平均值）。 */
    Eigen::Vector3f center() const;
    /*! \~chinese 面片的全局唯一 ID，不会与整个半边网格中任何其他元素重复。 */
    const std::size_t id;
    /*! \~chinese 属于这个面片的某一条半边。 */
    Halfedge* halfedge;
    /*!
     * \~chinese
     * \brief 边界面标识。
     *
     * 如果不加处理地从一个不封闭 mesh 创建半边，位于 mesh 边界的边上将只会有一条半边，
     * 从而让某些半边的反向指针 (`inv`) 为空。为了让半边网格在形式上统一，
     * 创建半边网格时会将每条边界视作一个“环路”，并为这个“环路”创建一个虚拟的面。
     */
    const bool is_boundary;
};

/*!
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格的不合法情况。
 *
 * 在创建半边网格失败或验证其合法性失败时，用该枚举类代表失败原因。
 * - NO_SELECTED_MESH 表示没有选中任何 mesh，半边网格没有数据源。
 * - MULTIPLE_ORIENTED_EDGES 表示有多条重叠的半边（端点和方向都相同），该 mesh 不可定向。
 * - NON_MANIFOLD_VERTEX 表示有非流形顶点。
 * - INIFINITE_POSITION_VALUE 表示有的顶点坐标是无穷大。
 * - INVALID_HALFEDGE_PERMUTATION 表示某条半边的指针空缺或指向了不正确的元素。
 * - INVALID_VERTEX_CONNECTIVITY 表示某个顶点与半边的连接关系不正确
 * - INVALID_EDGE_CONNECTIVITY 表示某条边与半边的连接关系不正确。
 * = INVALID_FACE_CONNECTIVITY 表示某个面与半边的连接关系不正确。
 * - ILL_FORMED_HALFEDGE_INVERSION 表示某条半边的反向关系不正确。
 * - POOR_HALFEDGE_ACCESSIBILITY 表示某条半边不能通过其连接到的元素反向访问到。
 *
 * 更详细的说明见运行时输出的日志内容。
 */
enum class HalfedgeMeshFailure
{
    NO_SELECTED_MESH,
    MULTIPLE_ORIENTED_EDGES,
    NON_MANIFOLD_VERTEX,
    INIFINITE_POSITION_VALUE,
    INVALID_HALFEDGE_PERMUTATION,
    INVALID_VERTEX_CONNECTIVITY,
    INVALID_EDGE_CONNECTIVITY,
    INVALID_FACE_CONNECTIVITY,
    ILL_FORMED_HALFEDGE_INVERSION,
    POOR_HALFEDGE_ACCESSIBILITY
};

/*!
 * \ingroup geometry
 * \~chinese
 * \brief 半边网格整体。
 *
 * 当 Dandelion 进入建模模式时，`Scene` 对象将用选中的 mesh 构造一个半边网格，
 * 从而支持各种基于半边网格的几何算法。当几何操作完成后，需要将半边网格的几何信息
 * （坐标、连接关系等）同步到原先的 mesh，这样才能显示操作带来的变化。
 *
 * 各种全局操作往往需要频繁增删几何元素，为了保证 \f$O(1)\f$ 的增删效率，
 * 所有的几何元素都存储于双向链表 (`LinkedList`) 中。
 */
class HalfedgeMesh
{
public:
    /*! \~chinese 指定一个 `GL::Mesh` 作为参照，构造半边网格。 */
    HalfedgeMesh(Object& object);
    /*! \~chinese 全局只有一个半边网格实例，因此不允许复制构造。 */
    HalfedgeMesh(HalfedgeMesh& other) = delete;
    /*! \~chinese 析构时会调用 `clear_erasure_records` 释放所有已删除元素占据的内存。 */
    ~HalfedgeMesh();
    /*! \~chinese 将当前半边网格的几何结构同步到数据源 mesh。 */
    void sync();
    /*! \~chinese 渲染所有的半边（不负责渲染顶点、边和面片）。 */
    void render(const Shader& shader);
    /*! \~chinese 返回绘制半边时的起点和终点坐标。 */
    static std::tuple<Eigen::Vector3f, Eigen::Vector3f> halfedge_arrow_endpoints(const Halfedge* h);
    /*!
     * \~chinese
     * \brief 翻转一条边。
     *
     * 由于翻转边的过程中所有几何基本元素数量不变，可以在不增删元素的前提下实现这个函数。
     * 在此情况下，返回的就是被传入的边。如果实现过程中删除了原有的边，
     * 返回的指针可能与传入的不同。
     *
     * 但无论采用哪种实现方法，都不应该翻转 mesh 边界上的边。
     * \returns 如果翻转成功，返回被翻转的边；反之返回 `std::nullopt` 。
     */
    std::optional<Edge*> flip_edge(Edge* e);
    /*!
     * \~chinese
     * \brief 分裂一条边。
     *
     * 将传入的边分裂成两条边，分裂处新增一个顶点，并将此顶点与相对位置的两个顶点连接。
     * 分裂操作要求指定边的两个邻接面都是三角形面。
     * \returns 如果分裂成功，返回新增顶点；反之返回 `std::nullopt` 。
     */
    std::optional<Vertex*> split_edge(Edge* e);
    /*!
     * \~chinese
     * \brief 坍缩一条边。
     *
     * 将传入的边坍缩成一个顶点，原先连接到两个端点上的其他边全部连接到这个顶点；
     * 如果这条边的某个邻接面是三角形面，这个邻接面会被坍缩成一条边。
     *
     * 不加检查地坍缩一条边有可能破坏 mesh 的流形性质。假设边端点分别为 \f$v_1,v_2\f$，
     * 此函数仅当
     * \f[
     *     \mathcal{N}_1(v_1)\cap\mathcal{N}_1(v_2) = 2
     * \f]
     * 时才执行坍缩，以免破坏流行性质。
     * \returns 如果坍缩成功，返回坍缩后的顶点；反之返回 `std::nullopt` 。
     */
    std::optional<Vertex*> collapse_edge(Edge* e);
    /*!
     * \~chinese
     * \brief 执行一次 Loop 曲面细分。
     *
     * 该函数使用 `flip_edge` 和 `split_edge` 完成一次 Loop 曲面细分。注意，Loop
     * 曲面细分只能细分三角网格。
     */
    void loop_subdivide();
    /*!
     * \~chinese
     * \brief 执行一次曲面简化。
     *
     * 该函数根据二次误差度量 (Quadric Error Metric, QEM) 确定损失最小的边，
     * 再用 `collapse_edge` 坍缩它从而减少面数，直至面数减为简化前的 1/4
     * 或找不到可以坍缩的边为止。
     */
    void simplify();
    /*!
     * \~chinese
     * \brief 执行一次重网格化。
     *
     * 该函数采用各向同性重网格化策略调整 mesh 的边，尽可能让每个顶点的度都接近 6、
     * 每个面片都接近正三角形。调整时它重复以下过程：
     * 1. 分裂过长的边
     * 2. 坍缩过短的边
     * 3. 通过翻转边让顶点的度数更平均
     * 4. 将顶点位置向它的 \f$\mathcal{N}_1\f$ 邻域平均值移动
     *
     * 各向同性重网格化只能应用于三角形网格。
     */
    void isotropic_remesh();
    /*! \~chinese 所有半边。 */
    LinkedList<Halfedge> halfedges;
    /*! \~chinese 所有顶点。 */
    LinkedList<Vertex> vertices;
    /*! \~chinese 所有边。 */
    LinkedList<Edge> edges;
    /*! \~chinese 所有面片。 */
    LinkedList<Face> faces;
    /*! \~chinese 将 `GL::Mesh` 使用的顶点索引映射为半边网格中的顶点指针。 */
    std::vector<Vertex*> v_pointers;
    /*!
     * \~chinese
     * \brief 当前处于不一致状态的几何基本元素。
     *
     * 在 GUI 上选中了半边网格中的某个元素后，控制器将设置该属性，`sync`
     * 函数根据该属性的值在每一帧更新数据源 mesh 中的顶点坐标，让建模模式下可以实时预览形变效果。
     */
    std::variant<std::monostate, Vertex*, Edge*, Face*> inconsistent_element;
    /*! \~chinese 全局一致性。成功完成一次全局操作后，此变量将置为真，表示需要同步到参照 mesh。 */
    bool global_inconsistent;
    /*! \~chinese 在创建半边网格时设置，如果创建正常则为 `std::nullopt`。 */
    std::optional<HalfedgeMeshFailure> error_info;

private:
    /*!
     * \~chinese
     * \brief 在曲面简化算法中用到的工具类。
     *
     * 基于 QEM 的简化算法需要实时获取损失最小的边，这个结构体用于记录一条边的坍缩代价、
     * 坍缩后顶点的最佳位置，并放入优先队列中进行排序。
     */
    struct EdgeRecord
    {
        EdgeRecord() = default;
        /*! \~chinese 根据两个端点的二次误差矩阵构造边的二次误差矩阵，并计算最佳坍缩位置。 */
        EdgeRecord(std::unordered_map<Vertex*, Eigen::Matrix4f>& vertex_quadrics, Edge* e);
        /*! \~chinese 这个记录对应的边。 */
        Edge* edge;
        /*! \~chinese 执行曲面简化算法时的最佳坍缩位置。 */
        Eigen::Vector3f optimal_pos;
        /*! \~chinese 执行曲面简化算法时坍缩这条边的代价（带来的误差）。 */
        float cost;
    };
    /*! \~chinese 排序 `EdgeRecord` 所需的比较运算符重载。 */
    friend bool operator<(const EdgeRecord& a, const EdgeRecord& b);
    /*! \~chinese 创建一条半边。 */
    Halfedge* new_halfedge();
    /*! \~chinese 创建一个顶点。 */
    Vertex* new_vertex();
    /*! \~chinese 创建一条边。 */
    Edge* new_edge();
    /*! \~chinese 创建一个面片，可以是真实存在的面片也可以是代表边界的虚拟面片。 */
    Face* new_face(bool is_boundary = false);
    /*! \~chinese 重新生成所有半边对应的箭头，在更新绘制数据时使用。 */
    void regenerate_halfedge_arrows();
    /*! \~chinese 删除一条半边。 */
    void erase(Halfedge* h);
    /*! \~chinese 删除一个顶点。 */
    void erase(Vertex* v);
    /*! \~chinese 删除一条边。 */
    void erase(Edge* e);
    /*! \~chinese 删除一个面。 */
    void erase(Face* f);
    /*!
     * \~chinese
     * \brief 清楚已删除元素的记录。
     *
     * 这个函数清空 `erased_[elements]` 中存储的删除记录。
     */
    void clear_erasure_records();
    /*!
     * \~chinese
     * \brief 检查半边网格的状态。
     *
     * 这个函数可以检查半边网格中的连接关系是否正确、指针是否悬垂，有助于及时发现错误。
     * 错误信息会被输出到日志，并返回一个错误枚举值（参考 `HalfedgeMeshFailure` 类的说明）。
     * \returns 如果发现错误，返回相应的错误枚举值；反之为 `std::nullopt`
     */
    std::optional<HalfedgeMeshFailure> validate();

    /*! \~chinese 用于构造半边网格几何元素时分配新的唯一 ID。 */
    static std::size_t next_available_id;
    /*! \~chinese 数据源 mesh 对应的物体。 */
    Object& object;
    /*! \~chinese 数据源 mesh，用于构造半边网格，需要同步修改。 */
    GL::Mesh& mesh;
    /*! \~chinese 已删除的半边。 */
    std::unordered_map<size_t, Halfedge*> erased_halfedges;
    /*! \~chinese 已删除的顶点。 */
    std::unordered_map<size_t, Vertex*> erased_vertices;
    /*! \~chinese 已删除的边。 */
    std::unordered_map<size_t, Edge*> erased_edges;
    /*! \~chinese 已删除的面。 */
    std::unordered_map<size_t, Face*> erased_faces;

    /*! \~chinese 将半边网格中的顶点指针映射为 `GL::Mesh` 使用的顶点索引。 */
    std::unordered_map<const Vertex*, size_t> v_indices;
    /*! \~chinese 将半边网格的半边指针映射为 `GL::LineSet` 中的箭头索引。 */
    std::unordered_map<const Halfedge*, size_t> h_indices;
    /*! \~chinese 用于渲染半边的 `LineSet` 对象。 */
    GL::LineSet halfedge_arrows;
    /*! \~chinese 日志记录器。 */
    std::shared_ptr<spdlog::logger> logger;
};

#endif // DANDELION_GEOMETRY_HALFEDGE_H
