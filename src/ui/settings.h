#ifndef DANDELION_UI_SETTINGS_H
#define DANDELION_UI_SETTINGS_H

/*!
 * \ingroup ui
 * \file ui/settings.h
 * \~english
 * \brief setting header is shared among all GUI-related **source files** (.cpp).
 *
 * In other words, this header is not related with definitions of any UI component, it should not
 * be included by any other header file through `#include`.
 *
 * \~chinese
 * \brief 在这个头文件中，定义了有关 GUI 的一些通用配置，
 * 它被所有与 GUI 相关的 **源文件** (.cpp) 共享。
 *
 * 换言之，这个文件与各种 UI 组件的定义无关，所以不应该被任何头文件使用 `#include` 包含。
 */

/*!
 * \~english
 * The global UI scale factor, defined in platform/platform.cpp
 * \~chinese
 * 全局 UI 缩放系数，定义于 platform/platform.cpp 中。
 */
extern float scale_factor;

/*!
 * \~english
 * Convert logical pixel (device independent pixel) to physical pixel.
 * For example, call px(200.0f) to get 200 px logically.
 * \return Multiplication of logical pixels and the scale factor.
 * \~chinese
 * 将逻辑像素（与设备无关的尺寸）转换为物理像素，例如用 `px(200.0f)` 表示逻辑上的 200 像素。
 * \return 物理像素乘以缩放系数。
 */
inline float px(float logical_pixel)
{
    return logical_pixel * scale_factor;
}

#endif // DANDELION_UI_SETTINGS_H
