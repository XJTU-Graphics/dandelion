#ifndef DANDELION_SCENE_SCENE_H
#define DANDELION_SCENE_SCENE_H

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <chrono>
#include <string_view>

#include <spdlog/spdlog.h>

#include "group.h"
#include "camera.h"
#include "light.h"
#include "selection_helper.h"
#include "../geometry/mesh.hpp"
#include "../geometry/halfedge.h"

/*!
 * \file scene/scene.h
 * \ingroup rendering
 * \ingroup simulation
 * \~chinese
 * \brief 包含场景的类。
 */

/*!
 * \ingroup rendering
 * \ingroup simulation
 * \~chinese
 * \brief 表示一个包含相机、光源、物体等对象的完整场景。
 *
 * `Scene` 是一个对象语义的类，它既包含完成图形设计和渲染所需的全部数据，
 * 也具有渲染自身预览效果的功能。当前的设计是：控制器 (Controller) 在 OpenGL
 * 上下文被创建后即创建场景实例，该实例在程序的整个生命周期中都是唯一的。因此，
 * 复制或移动 `Scene` 实例都是错误的行为，因为这有可能导致场景数据不一致。
 * 未来加入修改历史记录时，此设计思路有可能被改变。
 *
 * 场景可以是空的，如果它非空，则可以包含若干个物体组 (Group)；
 * 每个组也可以为空或包含若干物体 (Object)，物体是场景中的最小单元，不能再分了。
 * 各种数据遵循“尽可能存储到更小单元中”的原则，因此 `Scene` 类中只存储相机、
 * 光源这些与任何物体都无关的数据，而材质、mesh 等数据则存储到物体中。
 * 所有物体都由 `Scene` 对象创建并持有，当 `Scene` 对象析构时，所有的组、
 * 物体都会自行析构。
 */
class Scene
{
public:

    Scene();
    ///@{
    /*! \~chinese 禁止复制场景。 */
    Scene(Scene& other)       = delete;
    Scene(const Scene& other) = delete;
    ///@}
    /*! \~chinese 禁止移动场景。 */
    Scene(Scene&& other) = delete;
    ~Scene()             = default;
    /*!
     * \~chinese
     * \brief 从指定路径导入模型文件到这个场景中。
     *
     * 这个函数只会根据文件名创建一个物体组，然后调用物体组的 `load_models` 方法加载文件。
     *
     * \param file_path 要导入的模型文件路径
     * \returns 加载是否成功
     */
    bool import_model(const std::string& file_path);
    /*!
     * \~chinese
     * \brief 保存所有场景数据到指定的目录中。
     *
     * 保存场景时涉及两类数据：
     * - 模型数据（包括几何数据和材质数据），会被保存为通用模型文件格式
     * - 元数据，Dandelion 特有的组、物体信息等，会被保存为 metadata.json 文件
     *
     * \param directory 场景文件夹路径
     * \returns 是否保存成功
     */
    bool save(const std::string_view directory);
    /*!
     * \~chinese
     * \brief 从指定目录中加载保存的场景数据。
     *
     * 与 `save` 方法相对应，`load` 会从保存的数据中恢复模型、物体、组、相机、
     * 光源等全部信息。
     * \param directory 要加载的场景文件夹路径
     * \returns 是否加载成功
     */
    bool load(const std::string_view directory);
    /*!
     * \~chinese
     * \brief 清除场景中所有元素
     */
    void clear();
    /*! \~chinese 备份物体当前状态并开始模拟，此后每一帧都会调用所有物体的 `update` 方法。 */
    void start_simulation();
    /*! \~chinese 停止模拟，此后不再调用物体的 `update` 方法。 */
    void stop_simulation();
    /*! \~chinese 恢复物体在动画开始前的状态。 */
    void reset_simulation();
    /*! \~chinese 查询当前是否正在进行物理模拟。 */
    bool check_during_simulation();
    /*!
     * \~chinese
     * \brief 计算场景中所有物体下一帧要渲染的运动状态。
     *
     * 这个函数按照固定的时间步长模拟物体运动。
     *
     * 每一帧的渲染过程可以概括为更新数据（几何、运动等等）和渲染图像两步。
     * 约定当前屏幕上显示的是上一帧的图像，正在计算的是当前帧的数据。
     * 更新运动状态时首先用当前时间减去 `last_update` 得到上一帧和之前帧剩余时长之和；
     * 再循环模拟物体运动。每循环一次走过一个长度为 `time_step` 的时间步、
     * 剩余时长减去 `time_step` ；当剩余时长不足一个 `time_step` 时停止模拟，
     * 并将这一帧模拟走过的总时长累加到 `last_update` 上。
     */
    void simulation_update();

    /*! \~chinese 场景中所有的物体组。 */
    std::vector<std::unique_ptr<Group>> groups;
    /*!
     * \~chinese
     * \brief 当前被选中的物体。
     *
     * 在布局模式和物理模拟模式下，它决定当前被高亮的物体；在建模模式下，仅有这个物体被绘制。
     */
    Object* selected_object;
    /*!
     * \~chinese
     * \brief 当前被选中的元素。
     *
     * 根据当前所处的模式，物体、各类几何基本元素、光源都可能被选中，详见 `SelectableType`
     * 的类型说明。当 `selected_element` 持有 `std::monostate` 类型时，
     * 当前的选择状态为空（没有任何元素被选中）。
     */
    SelectableType selected_element;
    /*! \~chinese 用于预览场景的观察相机（主相机）。 */
    Camera main_camera;
    /*! \~chinese 用于 **离线渲染** 的相机，和用于预览场景的观察相机（主相机）无关。 */
    Camera camera;
    /*! \~chinese 用于 **离线渲染** 的光源，和预览时照亮物体的光源无关。 */
    std::list<Light> lights;
    /*! \~chinese 用于建模模式的半边网格。 */
    std::unique_ptr<HalfedgeMesh> halfedge_mesh;
    /*! \~chinese 用于在物理模拟模式下显示速度向量。 */
    ArrowSet arrows;
    /*! \~chinese 用于显示地面网格。 */
    LineSet ground_grid;
    /*! \~chinese 用于显示 x 轴。 */
    LineSet x_axis;
    /*! \~chinese 用于显示 y 轴。 */
    LineSet y_axis;
    /*! \~chinese 用于显示 z 轴。 */
    LineSet z_axis;
    /*! \~chinese 用于显示离线渲染相机。 */
    LineSet camera_wireframe;
    /*! \~chinese 被选中元素类型为顶点、边、面片或光源时使用的绘制对象。 */
    Mesh highlighted_element;
    /*! \~chinese 被选中元素类型为半边时使用的绘制对象。 */
    ArrowSet highlighted_halfedge;
    /*! \~chinese 显示拾取射线用的绘制对象，对应 `UI::DebugOptions::show_picking_ray` 。 */
    LineSet picking_ray;

private:

    /*!
     * \~chinese
     * 主相机的初始位置。
     */
    static Eigen::Vector3f initial_camera_pos;
    /*!
     * \~chinese
     * 主相机的初始观察目标点（看向的位置）。
     */
    static Eigen::Vector3f initial_camera_target;
    /*!
     * \~chinese
     * 保存场景元数据的文件名。
     */
    static constexpr std::string_view metadata_filename = "metadata.json";
    /*! \~chinese 状态变量，表示当前是否正在进行物理模拟。 */
    bool during_animation;
    /*! \~chinese 上一次将模拟状态同步到渲染的时间点。 */
    std::chrono::time_point<std::chrono::steady_clock> last_update;
    /*! \~chinese 碰撞检测时记录所有物体，其他情况下无效。 */
    std::vector<Object*> all_objects;
    /*! \~chinese 日志记录器。 */
    std::shared_ptr<spdlog::logger> logger;
};

#endif // DANDELION_SCENE_SCENE_H
