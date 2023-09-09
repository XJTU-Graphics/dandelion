#ifndef DANDELION_PLATFORM_SHADER_HPP
#define DANDELION_PLATFORM_SHADER_HPP

/*!
 * \file platform/shader.hpp
 */

#include <memory>

#include <spdlog/spdlog.h>

/*!
 * \ingroup platform
 * \~chinese
 * \brief 对 GLSL Shader 的简单封装。
 *
 * 一个 Shade 对象在构造时并不直接加载 GLSL 代码，正确的使用顺序是：
 * 1. 调用 `load_vertex_shader`
 * 2. 调用 `load_fragment_shader`
 * 3. 调用 `compile` 编译 Shader
 * 4. 前三个步骤都返回 `true` 的前提下，调用 `use` 使用 Shader
 */
class Shader
{
public:
    Shader(const std::shared_ptr<spdlog::logger>& logger);
    ~Shader();
    bool load_vertex_shader(const char* file_path);
    bool load_fragment_shader(const char* file_path);
    bool compile();
    void use() const;
    /*! \~chinese
     * 设置 shader 中指定的 uniform。这个函数只在 cpp 文件中特化，传入不支持的类型会在编译期报错。
     * \tparam T CPU 端的变量类型
     * \param name uniform 的名称
     * \param value CPU 端准备好的值
     * \returns 是否设置成功
     */
    template<typename T>
    bool set_uniform(const char* name, const T& value) const;
    unsigned int id;

private:
    std::shared_ptr<spdlog::logger> logger;
    std::unique_ptr<char[]> vertex_shader_source;
    std::unique_ptr<char[]> fragment_shader_source;
};

#endif // DANDELION_PLATFORM_SHADER_HPP
