#ifndef DANDELION_SCENE_OBJECT_H
#define DANDELION_SCENE_OBJECT_H

#include <cstddef>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "../platform/gl.hpp"
#include "../platform/shader.hpp"
#include "../utils/rendering.hpp"
#include "../utils/bvh.h"
#include "../utils/kinetic_state.h"

/*!
 * \file scene/object.h
 * \ingroup rendering
 * \ingroup simulation
 */

/*!
 * \ingroup rendering
 * \ingroup simulation
 * \~chinese
 * \brief 表示物体的类。
 *
 * “物体”由存储几何数据的 mesh、存储变换信息的位姿参数和存储颜色的材质组成，
 * 每个 `Object` 实例对应唯一的 `GL::Mesh`、`GL::Material` 和唯一的模型变换矩阵
 * (Model Transform Matrix)，是场景中的最小单元。
 *
 * 由于 `Object` 对象持有的 `GL::Mesh` 实例是不可复制的，当前的 `Object`
 * 也是不可复制的。但“复制一个物体”在逻辑上完全合理，因此未来会实现该类的复制构造函数。
 */
class Object
{
public:
    Object(const std::string& object_name);
    ///@{
    /*! \~chinese 禁止复制物体。 */
    Object(Object& other)       = delete;
    Object(const Object& other) = delete;
    ///@}
    ~Object() = default;
    /*! \~chinese 此物体的模型变换矩阵 (Model Transform Matrix)。 */
    Eigen::Matrix4f model();
    /*!
     * \~chinese
     * \brief 更新下一个时间步的运动状态。
     *
     * 首先使用 `Object::step` 计算下一个时间步自身的运动状态，尝试将自身移动到算出的位置，
     * 再检测在此位置是否会与其他物体碰撞。如果发生碰撞则自身位置回退，
     * 并根据动量定理修改碰撞双方的速度。
     *
     * \param all_objects 场景中所有的物体，用于碰撞检测和响应。
     */
    void update(std::vector<Object*>& all_objects);
    /*!
     * \~chinese
     * \brief 根据指定的渲染模式渲染物体。
     *
     * \param shader 对一个 `Shader` 对象的引用
     * \param mode 渲染模式。所有模式下都渲染面片，建模模式下额外渲染边和顶点。
     * \param selected 布局模式下该物体是否被选中，被选中的物体额外渲染边。其他模式下，
     * 该参数无意义。
     */
    void render(const Shader& shader, WorkingMode mode, bool selected);
    /*!
     * \~chinese
     * \brief 重新构建 BVH 。
     *
     * 原先没有构建过 BVH 的情况下调用这个函数也是安全的。
     */
    void rebuild_BVH();

    /*!
     * \~chinese
     * \brief 用于更新物体运动状态的函数。
     *
     * 让这个静态变量指向不同的函数，就可以更换不同的求解器，默认使用前向欧拉法求解。
     */
    static std::function<KineticState(const KineticState&, const KineticState&)> step;
    /*! \~chinese 是否启用 BVH 加速碰撞检测。 */
    static bool BVH_for_collision;
    /*! \~chinese 物体的 ID，不会与其他物体重复。 */
    std::size_t id;
    /*! \~chinese 物体的名称，来自加载文件时的 mesh 名称（文件中未定义会创建一个默认名称）。 */
    std::string name;
    /*! \~chinese 控制物体是否可见，当前恒为真，并无实际作用。 */
    bool visible;
    /*! \~chinese 表示上一帧过后是否被修改，在第一次加载后或与半边网格不一致时为真。 */
    bool modified;
    ///@{
    /*! \~chinese 物体的位姿属性，用于构造它的模型变换矩阵 (Model Transform Matrix)。 */
    Eigen::Vector3f center;
    Eigen::Vector3f scaling;
    Eigen::Quaternionf rotation;
    ///@}
    /*! \~chinese 物体的速度。 */
    Eigen::Vector3f velocity;
    /*! \~chinese 物体所受的合外力。 */
    Eigen::Vector3f force;
    /*! \~chinese 物体质量 */
    float mass;
    /*! \~chinese 上一帧的状态，用于高阶求解器求解。 */
    KineticState prev_state;
    /*! \~chinese 进入物理模拟模式后用于备份初始状态。 */
    KineticState backup;
    /*! \~chinese
     * 由于位姿参数每一帧都可能变化，mesh 中存储模型坐标系下的坐标以提高运行效率。
     * 如需获取世界坐标系下的坐标，请乘上模型变换矩阵。
     */
    GL::Mesh mesh;
    /*!
     * \~chinese
     * \brief 根据这个物体建立的 BVH 。
     *
     * 物体的 BVH 在它的 mesh 加载完成后构建。由于 mesh 数据的加载过程是在 `Group::load`
     * 中完成的，构建 BVH 的函数调用也只能在这个函数（而不是在 `Object` 的构造函数）中进行。
     *
     * 物体的 mesh 数据都在模型坐标系下，因此 BVH 也是建立在模型坐标系下的。这意味着物体的平移、
     * 旋转和缩放都不改变 BVH 的结构，只有物体发生形变时才需要更新 BVH 。目前的实现是：
     * 每次退出建模模式时，不加检查地删除原先的 BVH 并重新构建。
     */
    std::unique_ptr<BVH> bvh;
    /*! \~chinese 代表 BVH 所有包围盒的线框。 */
    GL::LineSet BVH_boxes;

private:
    void refresh_BVH_boxes(BVHNode* node);
    /*! \~chinese 下一个可用的物体 ID 。 */
    static std::size_t next_available_id;
    /*! \~chinese 日志记录器。 */
    std::shared_ptr<spdlog::logger> logger;
};

#endif // DANDELION_SCENE_OBJECT_H
