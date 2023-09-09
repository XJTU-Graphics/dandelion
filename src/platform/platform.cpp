#include "platform.h"

#include <cmath>
#include <exception>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <stb/stb_image.h>

#include "../ui/settings.h"
#include "../ui/controller.h"
#include "../utils/math.hpp"
#include "../utils/rendering.hpp"
#include "../utils/logger.h"

using std::make_unique;
using std::sqrt;

float scale_factor = 1.0f;

constexpr double DPI_EPS     = 1e-3;
constexpr double MM_PER_INCH = 25.4;

Platform::Platform()
{
    logger = get_logger("Platform");
    // Try to initialize GLFW and OpenGL context, the minimum requirement is
    // OpenGL 3.3 to support the core profile.
    glfwInit();
    bool context_created = create_context(4, 6);
    if (!context_created) {
        logger->info("Failed to create OpenGL context 4.6, drop back to 4.3");
        context_created = create_context(4, 3);
    }
    if (!context_created) {
        logger->warn("Failed to create OpenGL context 4.3, drop back to 3.3");
        context_created = create_context(3, 3);
    }
    if (!context_created) {
        logger->critical("Failed to create OpenGL context 3.3, exit");
        glfwTerminate();
        std::abort();
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        logger->critical("Could not load OpenGL functions");
        std::abort();
    }
    logger->info("runtime OpenGL context: {}", (const char*)glGetString(GL_VERSION));
    // Set the window icon.
    GLFWimage icons[3];
    icons[0].pixels = stbi_load("resources/icons/dandelion_32.png", &icons[0].width,
                                &icons[0].height, nullptr, 4);
    icons[1].pixels = stbi_load("resources/icons/dandelion_64.png", &icons[1].width,
                                &icons[1].height, nullptr, 4);
    icons[2].pixels = stbi_load("resources/icons/dandelion_512.png", &icons[2].width,
                                &icons[2].height, nullptr, 4);
    glfwSetWindowIcon(window, 3, icons);
    // 100% size for DPI < 120, 150% for 120 <= DPI < 192, 200% for DPI > 192
    dpi          = get_dpi();
    scale_factor = 1.0;
    if (dpi > 192.0 - DPI_EPS) {
        scale_factor = 2.0;
    } else if (dpi > 120.0 - DPI_EPS) {
        scale_factor = 1.5;
    }
    // Here we initialize ImGui before set the resizing callback, because
    // the function on_framebuffer_resized will call controller's method, which
    // is only available after ImGui's initialization.
    init_ui();
    glfwSetFramebufferSizeCallback(window, on_framebuffer_resized);
    // Perform DPI-aware scaling.
    resize_window();
    // Default properties for OpenGL, such as depth test or line width.
    set_opengl_properties();
    // Compile and use shader program for scene previewing.
    shader = make_unique<Shader>(logger);
    shader->load_vertex_shader("resources/shaders/vertex.glsl");
    shader->load_fragment_shader("resources/shaders/fragment.glsl");
    if (!shader->compile()) {
        logger->critical("Failed to compile shader program");
    }
    shader->use();
}

Platform::~Platform()
{
    logger->info("ImGui shutdown");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    logger->info("GLFW shutdown");
    glfwDestroyWindow(window);
    glfwTerminate();
    spdlog::shutdown();
}

void Platform::eventloop()
{
    Controller& controller = Controller::controller();
    controller.on_framebuffer_resized(static_cast<float>(window_width),
                                      static_cast<float>(window_height));
    while (!glfwWindowShouldClose(window)) {
        glClearColor(RGB(54, 54, 54), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwPollEvents();
        controller.process_input();

        controller.render(*shader);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

double Platform::get_dpi() noexcept
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    // Get logical screen size measured in screen coordinates
    // See https://www.glfw.org/docs/latest/intro_guide.html#coordinate_systems
    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    int logical_width             = video_mode->width;
    int logical_height            = video_mode->height;
    // Get physical screen size measured in millimeters
    int physical_width, physical_height;
    glfwGetMonitorPhysicalSize(monitor, &physical_width, &physical_height);
    double diagonal = sqrt(squ(physical_width) + squ(physical_height)) / MM_PER_INCH;
    logger->info("Physical screen size: {}x{} mm, diagonal: {:.2f} in", physical_width,
                 physical_height, diagonal);

    double dpi = sqrt(squ(logical_width) + squ(logical_height)) / diagonal;

    return dpi;
}

void Platform::resize_window() noexcept
{
    // Default window size is 800x600.
    window_width  = static_cast<int>(800.0f * scale_factor);
    window_height = static_cast<int>(600.0f * scale_factor);
    // Get the primary monitor and its origin position in the virtual
    // screen coordinate system.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    int screen_left, screen_top;
    glfwGetMonitorPos(monitor, &screen_left, &screen_top);
    const GLFWvidmode* video_mode = glfwGetVideoMode(monitor);
    const int screen_width        = video_mode->width;
    const int screen_height       = video_mode->height;
    const int x                   = screen_width / 2 - window_width / 2;
    const int y                   = screen_height / 2 - window_height / 2;
    // Resize the window according to the monitor's DPI.
    logger->info("screen DPI: {:.2f}, scale factor: {:.1f}", dpi, scale_factor);
    glfwSetWindowSize(window, window_width, window_height);
    // Align the center of the window to the center of the screen.
    glfwSetWindowPos(window, screen_left + x, screen_top + y);
    glfwShowWindow(window);
    // Update OpenGL viewport
    glViewport(0, 0, window_width, window_height);
}

bool Platform::create_context(int major, int minor) noexcept
{
    logger->debug("Try to create OpenGL context {}.{}", major, minor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Set the window invisible because we want to set its initial position after
    // calculating its size according to screen DPI. See GLFW's document of
    // glfwSetWindowPos for more details.
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    window = glfwCreateWindow(800, 600, "Dandelion 3D", nullptr, nullptr);

    return window != nullptr;
}

void Platform::set_opengl_properties() noexcept
{
    glEnable(GL_DEPTH_TEST);
    glPointSize(px(point_size));
    glLineWidth(px(line_width));
}

bool Platform::init_ui()
{
    // Initialize Dear ImGui context
    if (!IMGUI_CHECKVERSION())
        return false;
    if (ImGui::CreateContext() == nullptr)
        return false;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Load the Source Han Sans SC font and scale all UI elements by `scale_factor'
    io.Fonts->Clear();
    ImFont* source_han_sans =
        io.Fonts->AddFontFromFileTTF("resources/SourceHanSansSC-Regular.otf", 18.0f * scale_factor,
                                     nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    if (source_han_sans == nullptr) {
        logger->warn("Source Han Sans not found, drop back to the default font");
    }
    // Device-dependent initial configurations for Dear ImGui.
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale_factor);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void Platform::on_framebuffer_resized([[maybe_unused]] GLFWwindow* window, GLsizei width,
                                      GLsizei height)
{
    glViewport(0, 0, width, height);
    Controller& controller = Controller::controller();
    controller.on_framebuffer_resized(static_cast<float>(width), static_cast<float>(height));
}
