#ifndef DANDELION_UI_TOOLBAR_H
#define DANDELION_UI_TOOLBAR_H

#include <cstddef>
#include <optional>
#include <functional>

#include "selection_helper.h"
#include "../scene/scene.h"
#include "../utils/rendering.hpp"

/*!
 * \file ui/toolbar.h
 * \ingroup ui
 */

namespace UI {

/*!
 * \ingroup ui
 * \~chinese
 * \brief 场景中最大可容纳的节点数（组数量与物体数量的总和）。
 */
constexpr std::size_t MAX_SCENE_NODES = 100;

/*!
 * \ingroup ui
 * \~chinese
 * \brief 工具栏提供大部分对场景、物体、相机和光源的操作。
 */
class Toolbar
{
public:
    /*!
     * \~chinese
     * \brief 工具栏持有对工作模式的引用和对选中元素的引用，构造时必须传递。
     *
     * 工具栏还有 `on_element_selected` 和 `on_selection_canceled`
     * 两个与选择和拾取有关的回调函数，如果没有设置这两个回调函数，工具栏将不能正常工作。
     * 但构造函数并不会设置它们，而是由控制器在创建工具栏时设置。目前，
     *
     * - `on_element_selected` 回调对应 `Controller::select`
     * - `on_selection_canceled` 回调对应 `Controller::unselect`
     */
    Toolbar(WorkingMode& mode, const SelectableType& selected_element);
    ~Toolbar();
    /*! \~chinese 显示工具栏。 */
    void render(Scene& scene);

    /*! \~chinese 选中元素时的回调函数。 */
    std::function<void(SelectableType)> on_element_selected;
    /*! \~chinese 取消选中时的回调函数。 */
    std::function<void()> on_selection_canceled;

private:
    /*! \~chinese 将场景层次结构展示为一个树形列表。 */
    void scene_hierarchies(Scene& scene);
    /*! \~chinese 显示标签分别为 x, y, z 的三个 `ImGui::DragFloat` 控件。 */
    void xyz_drag(float* x, float* y, float* z, float v_speed, const char* format = "%.2f");
    /*! \~chinese 显示并编辑单个物体的材质属性。 */
    void material_editor(GL::Material& material);
    /*! \~chinese 布局模式对应的标签页。 */
    void layout_mode(Scene& scene);
    /*! \~chinese 建模模式对应的标签页。 */
    void model_mode(Scene& scene);
    /*! \~chinese 渲染模式对应的标签页。 */
    void render_mode(Scene& scene);
    /*! \~chinese 物理模拟模式对应的标签页。 */
    void simulate_mode(Scene& scene);

    /*! \~chinese 与 Dandelion 的工作模式一一对应，`render` 方法据此调整渲染行为。 */
    WorkingMode& mode;
    /*! \~chinese 工具栏需要了解当前被选中的元素才能显示相应的操作。 */
    const SelectableType& selected_element;
    /*! \~chinese 用于在渲染模式下展示渲染结果的 OpenGL 纹理描述符。 */
    unsigned int gl_rendered_texture;
};

} // namespace UI

#endif // DANDELION_UI_TOOLBAR_H
