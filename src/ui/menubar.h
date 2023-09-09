#ifndef DANDELION_UI_MENUBAR_H
#define DANDELION_UI_MENUBAR_H

#include <memory>

#include "../scene/scene.h"

/*!
 * \file ui/menubar.h
 * \ingroup ui
 */

namespace UI {

/*!
 * \ingroup ui
 * \~chinese
 * \brief 辅助调试的 GUI 选项。
 */
struct DebugOptions
{
    /*! \~chinese 默认关闭所有的调试选项。 */
    DebugOptions();
    /*! \~chinese 显示进行拾取时生成的虚拟光线。 */
    bool show_picking_ray;
    /*! \~chinese 显示所有物体的 BVH 结构。 */
    bool show_BVH;
};

/*!
 * \ingroup ui
 * \~chinese
 * \brief 菜单栏提供加载文件操作、调整日志级别等调试选项和一些帮助页面。
 */
class Menubar
{
public:
    /*! \~chinese 菜单栏持有对调试选项的引用，构造时需要传递。 */
    Menubar(DebugOptions& debug_options);
    ~Menubar();
    /*! \~chinese 显示菜单栏。 */
    void render(Scene& scene);
    /*! \~chinese 当前菜单栏高度。 */
    float height() const;

private:
    /*! \~chinese 调整全局日志输出级别的菜单。 */
    void logging_levels_menu();
    /*! \~chinese 显示 GUI 操作帮助的弹出窗口。 */
    void usage();
    /*! \~chinese 显示开发者信息的弹出窗口。 */
    void about();
    /*! \~chinese 控制调试选项的面板。 */
    void debug_options_panel();
    /*! \~chinese 当前菜单栏高度，每一帧调用 `render` 时更新。 */
    float menubar_height;
    /*! \~chinese 对调试选项的引用。 */
    DebugOptions& debug_options;
    /*! \~chinese 用于在开发者信息页面显示 Logo 的 OpenGL 纹理描述符。 */
    unsigned int gl_icon_texture;
};

} // namespace UI

#endif // DANDELION_UI_MENUBAR_H
