#ifndef DANDELION_UI_CONTROLLER_H
#define DANDELION_UI_CONTROLLER_H

/*!
 * \file ui/controller.h
 */

#include <memory>
#include <variant>

#include <spdlog/spdlog.h>

#include "menubar.h"
#include "toolbar.h"
#include "selection_helper.h"
#include "../platform/gl.hpp"
#include "../platform/shader.hpp"
#include "../scene/camera.h"
#include "../scene/scene.h"

/*!
 * \ingroup ui
 * \~chinese
 * \brief 控制器管理所有的界面组件，并处理和预览视角操作（例如旋转、缩放或者平移）相关的输入。
 *
 * 控制器的生命周期就是整个程序 GUI 的生命周期，因此这是一个单例的类。其唯一一个实例可以通过
 * `controller()` 这个静态方法访问，访问操作是线程安全的（但访问后的修改不是）。
 * 控制器创建并持有所有与图形界面相关的资源：
 *
 * - 界面元素（菜单栏和工具栏）
 * - 场景（包含所有物体、坐标轴等）
 *
 * 界面元素本身不存储场景，也不保存指向场景的引用等。控制器在每一帧将场景的引用传递给
 * `Toolbar::render` 或 `Menubar::render` 函数，从而允许这些界面元素访问场景。
 * 与 `Menubar` 或 `Toolbar` 保存引用相比，这种设计当前并没有额外有点，它只是参考 Scotty3D
 * 思路的结果，将来有可能为引入全局撤销 (Undo) 和重做 (Redo) 操作提供方便。
 */
class Controller
{
public:
    /*!
     * \~chinese
     * \brief 获取 `Controller` 类的唯一实例。
     *
     * 唯一的实例被定义成函数内的静态变量，在 C++ 11 及之后的标准中，这种定义方式是线程安全的。
     */
    static Controller& controller();
    ///@{
    /*! \~chinese 禁止复制构造。*/
    Controller(Controller& other)            = delete;
    Controller& operator=(Controller& other) = delete;
    ///@}
    ~Controller();
    /*!
     * \~chinese
     * \brief 将鼠标拖动转换为旋转视角或平移视角操作。
     *
     * 当按下鼠标中键（或 Alt+鼠标左键）拖动时旋转视角，按下 Ctrl+鼠标左键拖动时平移视角。
     * 同时按下 Alt 和 Ctrl 的效果与只按下 Alt 相同（只旋转不平移）。
     *
     * \param initial 为真表示开始拖动时调用，为假表示拖动过程中调用。
     */
    void on_mouse_dragged(bool initial);
    /*!
     * \~chinese
     * \brief 根据鼠标点击选取场景中的物体。
     *
     * 这个函数将鼠标点击的屏幕坐标 (screen space) 转换为观察坐标 (view space)，
     * 然后将观察坐标转换回世界坐标 (world space)，从而构造一条从观察相机（主相机）
     * 发出的射线，按这条射线与场景中物体的相交结果拾取（选中）场景中的可选中对象。
     * 工作于布局模式和物理模拟模式下时，它调用 `pick_object` 函数，工作于建模模式下时，
     * 它调用 `pick_element` 函数。
     */
    void on_picking();
    /*!
     * \~chinese
     * \brief 将拨动鼠标滚轮的操作转换为视角拉进或远离。
     *
     * 缩放视角的操作按指数规律执行：鼠标滚轮每滚动一个单位，观察相机到目标点的距离就乘上一个固定值
     * `wheel_scroll_factor`，即
     *
     * \f[
     *   \mathbf{d}_\text{new}=\text{wheel_scroll_factor}^\text{wheel_delta}\cdot\mathbf{d}
     * \f]
     */
    void on_wheel_scrolled();
    /*!
     * \~chinese
     * \brief 在缩放窗口时调整轨迹球半径、预览视角（相机）Y 方向的 FoV，并更新自身记录的窗口尺寸。
     *
     * 虽然名字相同，但这个函数并非 GLFW 的窗口缩放回调函数。在缩放时它会被真正的回调函数
     * `Platform::on_framebuffer_resized` 调用。
     * \param width 缩放后的窗口宽度
     * \param height 缩放后的窗口高度
     */
    void on_framebuffer_resized(float width, float height);
    /*!
     * \~chinese
     * \brief 处理关于场景操作的输入。
     *
     * 这个函数判别鼠标操作是否对应于某种场景操作（调整视角、拾取物体等），并调用相应的处理函数
     * （例如 `on_mouse_clicked` 或 `on_picking`）。与场景操作相对的是控件操作（例如点击按钮），
     * 控件操作由相应的 ImGui 控件直接处理。
     */
    void process_input();
    /*!
     * \~chinese
     * 渲染场景和各个 UI 组件。控制器本身不直接渲染任何内容，而是调用相应类的 `render()` 方法。
     * \param shader 当前渲染使用的 shader，用于设置 shader 中的全局变量。
     */
    void render(const Shader& shader);
    /*!
     * \~english All variables used for layout are float because ImVec2 expects float.
     * \~chinese 由于 ImGui 使用浮点数表示尺寸，所有和布局相关的变量都是浮点型。
     */
    ///@{
    float window_width, window_height;
    float toolbar_width;
    ///@}

private:
    /*!
     * \~chinese
     * \brief 选择一个元素。
     *
     * 这个函数修改控制器记录的选择状态。它包括两步动作：
     * 1. 调用 `unselect` 清除原先的选择状态及其副作用
     * 2. 修改被选中元素 `selected_element`，调用 `select_[type]` 函数执行具体操作
     *
     * \param element 要选择的新元素
     */
    void select(SelectableType element);
    /*!
     * \~chinese
     * \brief 取消选择。
     *
     * 这个函数清楚当前的选择状态，将`selected_element` 重置为 `std::monostate`
     * 并清空 `highlighted_element` 或 `highlighted_halfedge` 这些高亮元素。
     * 如果之前的被选中元素设置过 `Scene` 等对象的属性，它也会一并将其重置。
     */
    void unselect();
    /*!
     * \~chinese
     * \brief 渲染当前选中的元素。
     *
     * 渲染选中元素时会禁用深度检测 `GL_DEPTH_TEST` ，直接在原先的绘制结果上叠加。
     * 因此，即使将视角旋转到背面也会看到高亮出来的被选中元素。
     */
    void render_selected_element(const Shader& shader);
    /*!
     * \~chinese
     * \brief 渲染帮助调试的元素。
     *
     * 根据 `debug_options` 中的各项设置，渲染帮助调试的结构，如 BVH
     * 的所有包围盒等。
     */
    void render_debug_helpers(const Shader& shader);
    /*!
     * \~chinese
     * \brief 拾取物体。
     *
     * 在布局或物理模拟模式下，根据射线求交的结果选择物体。
     * \param ray 根据点击位置生成的射线（世界坐标系下）
     */
    void pick_object(Ray& ray);
    /*!
     * \~chinese
     * \brief 拾取半边、顶点、边或面片。
     *
     * 在建模模式下，根据射线求交的结果选择半边网格上的基本元素。
     * \param ray 根据点击位置生成的射线（世界坐标系下）
     */
    void pick_element(Ray& ray);
    /*! \~chinese 选择物体并更新 `Scene::selected_object` 。 */
    void select_object(Object* object);
    /*! \~chinese 选择半边并更新 `highlighted_halfedge` 。 */
    void select_halfedge(const Halfedge* halfedge);
    /*! \~chinese 选择顶点并更新 `highlighted_element` 。 */
    void select_vertex(Vertex* vertex);
    /*! \~chinese 选择边并更新 `highlighted_element` 。 */
    void select_edge(Edge* edge);
    /*! \~chinese 选择面片并更新 `highlighted_element` 。 */
    void select_face(Face* face);
    /*! \~chinese 选择光源并更新 `highlighted_element` 。 */
    void select_light(Light* light);
    /*!
     * \~chinese
     * \brief 将鼠标拖动转换为轨迹球 (Trackball) 操作，用于旋转预览视角。
     *
     * 轨迹球曲面的推导参考 [Object Mouse
     * Trackball](https://www.khronos.org/opengl/wiki/Object_Mouse_Trackball)。
     * \param initial 为真表示开始拖动时调用，为假表示拖动过程中调用。
     */
    void on_rotating(bool initial);
    /*!
     * \~chinese
     * \brief 将鼠标拖动转换为视角平移。
     *
     * 将鼠标拖动时屏幕坐标的变化量换算为观察空间 (view space) 中的坐标变化，
     * 光标向右移动对应相机向左移动，光标向上移动对应相机向下移动。
     * 相应的变化量会累加到主相机的 `position` 和 `target` 属性上，因此平移过程中视线方向不会改变。
     *
     * 视点到观察点的距离增大（减小）时，相同屏幕坐标变化量对应的观察坐标变化量也增大（减小），
     * 平移量正比于视角远近，比例系数为 `mouse_translation_factor` 。
     * \param initial 为真表示开始拖动时调用，为假表示拖动过程中调用。
     */
    void on_translating(bool initial);
    /*! \~chinese 缩放视角的控制系数，详见 `on_wheel_scrolled` 方法。 */
    static constexpr float wheel_scroll_factor = 0.8f;
    /*! \~chinese 平移视角时屏幕坐标到观察坐标换算系数的固定部分。 */
    static constexpr float mouse_translation_factor = 0.001f;
    /*! \~chinese 构造函数是私有的。 */
    Controller();
    /*!
     * \~chinese
     * \brief 全局工作模式。
     *
     * 控制器维护着当前的工作模式，决定各种功能可用或禁用。其他的 GUI 组件持有对该变量的引用。
     */
    WorkingMode mode;
    /*! \~chinese 一些帮助调试的选项，详见 `UI::DebugOptions` 类型说明。 */
    UI::DebugOptions debug_options;
    /*! \~chinese 菜单栏。 */
    std::unique_ptr<UI::Menubar> menubar;
    /*! \~chinese 工具栏。 */
    std::unique_ptr<UI::Toolbar> toolbar;
    /*! \~chinese 包含所有三维数据的场景实例。 */
    std::unique_ptr<Scene> scene;
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
    std::unique_ptr<Camera> main_camera;
    /*! \~chinese 日志记录器。 */
    std::shared_ptr<spdlog::logger> logger;
    /*! \~chinese 当前的轨迹球半径，决定轨迹球控制曲面上球面和双曲面部分的相切位置。 */
    float trackball_radius;
    /*! \~chinese 被选中元素类型为顶点、边、面片或光源时使用的绘制对象。 */
    GL::Mesh highlighted_element;
    /*! \~chinese 被选中元素类型为半边时使用的绘制对象。 */
    GL::LineSet highlighted_halfedge;
    /*! \~chinese 显示拾取射线用的绘制对象，对应 `UI::DebugOptions::show_picking_ray` 。 */
    GL::LineSet picking_ray;
};

#endif // DANDELION_UI_CONTROLLER_H
