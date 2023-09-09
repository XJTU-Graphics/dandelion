#ifndef DANDELION_UTILS_RENDERING_HPP
#define DANDELION_UTILS_RENDERING_HPP

/*!
 * \ingroup utils
 * \file utils/rendering.hpp
 * \~chinese
 * \brief 这个文件定义了一些和渲染（离线渲染或场景预览）相关的常量、枚举等。
 */

// MSVC defines the macro RGB, undefine it.
#ifdef _WIN32
#undef RGB
#endif
/*!
 * \ingroup utils
 * \~chinese
 * 将整数形式的颜色转换成浮点数，便于传入 Eigen::Vector3f 的构造函数。
 */
#define RGB(r, g, b) (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f

///@{
/*!
 * \ingroup utils
 * \~chinese
 * 预览场景时，GLSL shader 中各输入变量的位置。相应的变量定义参考 resources/shaders
 * 中的 shader 代码。
 */
constexpr unsigned int vertex_position_location = 0;
constexpr unsigned int vertex_color_location    = 1;
constexpr unsigned int vertex_normal_location   = 2;
///@}

/*!
 * \~chinese
 * 预览场景时用 OpenGL 绘制的点大小。
 */
constexpr float point_size = 8.0f;
/*!
 * \~chinese
 * 预览场景时用 OpenGL 绘制的线宽。
 */
constexpr float line_width = 2.0f;

/*!
 * \~chinese
 * 预览场景时使用的渲染模式，与 Dandelion 的工作模式一一对应。
 */
enum class WorkingMode
{
    LAYOUT,
    MODEL,
    RENDER,
    SIMULATE
};

/*!
 * \~chinese
 * \brief 允许拾取物体的模式。
 *
 * 这个常量数组指定允许拾取物体（直接在屏幕上点击选取物体）的工作模式，
 * 当前为布局模式（用于操纵物体）和物理模拟模式（用于修改物理属性）。
 */
constexpr WorkingMode picking_enabled_modes[] = {WorkingMode::LAYOUT, WorkingMode::MODEL,
                                                 WorkingMode::SIMULATE};

/*!
 * \~chinese
 * \brief 判断指定模式是否允许拾取物体。
 *
 * \param mode 表示当前的工作模式。
 */
inline bool check_picking_enabled(WorkingMode mode)
{
    for (auto enabled_mode : picking_enabled_modes) {
        if (mode == enabled_mode) {
            return true;
        }
    }
    return false;
}

#endif // DANDELION_UTILS_RENDERING_HPP
