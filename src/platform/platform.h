#ifndef DANDELION_PLATFORM_PLATFORM_H
#define DANDELION_PLATFORM_PLATFORM_H

/*!
 * \file platform/platform.h
 */

#include <memory>

#ifdef _WIN32
    #include <Windows.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "shader.hpp"

/*!
 * \ingroup platform
 * \~english
 * \brief The Platform class manages the platform-dependent window and some platform-
 * dependent parameters.
 *
 * \~chinese
 * \brief 这个类管理平台相关的窗口、配置信息以及应用启动的入口和事件循环。
 */
class Platform
{
public:

    /*!
     * \~chinese
     * \brief 初始化 OpenGL Context、创建窗口、初始化 Dear ImGui、创建预览场景用的 shader。
     *
     * 构造函数按照 4.6 -> 4.3 -> 3.3 的版本顺序尝试创建 OpenGL Context，若全部失败，
     * 则程序终止直接退出（OpenGL 3.3 是使用 GLSL shader 的最低要求）。
     *
     * OpenGL Context 创建成功后，构造函数加载 OpenGL API 并创建窗口，再探测用户显示器的
     * DPI 来决定程序的全局缩放比例。Dear ImGui 的初始化过程依赖于全局缩放。
     * 最后，构造函数编译、链接渲染预览场景的 shader。
     */
    Platform();
    /*! \~chinese 执行 OpenGL 销毁窗口的操作。 */
    ~Platform();
    /*!
     * \~chinese
     * \brief 事件循环主体。
     *
     * 这是 GUI 的主循环，负责接收消息（输入）并转发给 Controller 对象。
     * 此循环内的过程基本上是与平台 (OpenGL / GLFW) 相关的，平台无关的处理则移交 Controller。
     */
    void eventloop();

private:

    static constexpr float mouse_wheel_threshold = 1e-2f;
    /*! \~chinese 检测显示器 DPI。 */
    double get_dpi() noexcept;
    /*! \~chinese 按 DPI 缩放窗口。 */
    void resize_window() noexcept;
    /*!
     * \~chinese
     * 创建 `major.minor` 版本的 OpenGL Context。
     * \param major OpenGL 主版本
     * \param minor OpenGL 次版本（小版本）
     * \returns 创建是否成功
     */
    bool create_context(int major, int minor) noexcept;
    /*! \~chinese
     * 设置一些全局通用的 OpenGL 属性，例如开启深度测试等。
     */
    void set_opengl_properties() noexcept;
    /*! \~chinese 初始化 Dear ImGui，加载字体、设置缩放和基础样式。 */
    bool init_ui();
    /*! \~chinese 处理窗口缩放的回调函数。 */
    static void on_framebuffer_resized(GLFWwindow* window, GLsizei width, GLsizei height);

    /*! \~chinese 持有的日志记录器。 */
    std::shared_ptr<spdlog::logger> logger;
    /*! \~chinese 窗口对象。 */
    GLFWwindow* window;
    ///@{
    /*! \~chinese 实时更新的窗口长宽。*/
    int window_width, window_height;
    ///@}
    /*! \~chinese 屏幕分辨率，实际是 PPI (Pixels Per Inch)，称为 DPI 只是出于习惯。 */
    double dpi;
    /*! \~chinese 用于实时渲染预览窗口的 OpenGL Shader 封装对象。 */
    std::unique_ptr<Shader> shader;
};

#endif
