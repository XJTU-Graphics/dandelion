#include <ctime>

#include <catch2/catch_amalgamated.hpp>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void abort_on_error()
{
    spdlog::shutdown();
    glfwTerminate();
    std::abort();
}

int main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%n] [%^%l%$] %v");
    spdlog::set_default_logger(spdlog::stdout_color_mt("Test"));
    std::tm now = fmt::localtime(std::time(nullptr));
    spdlog::info("Dandelion 3D Unit Test, started at {:%Y-%m-%d %H:%M:%S%z}", now);

    GLFWwindow* window = nullptr;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_MENUBAR, GL_FALSE);
#endif
    window = glfwCreateWindow(800, 600, "Dandelion 3D Test", nullptr, nullptr);
    if (window == nullptr) {
        spdlog::critical("Cannot create an OpenGL 3.3 context, abort");
        abort_on_error();
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::critical("Cannot load OpenGL APIs, abort");
        glfwDestroyWindow(window);
        abort_on_error();
    }

    int result = Catch::Session().run(argc, argv);

    glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
