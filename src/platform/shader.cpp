#include "shader.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>

#include <Eigen/Core>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>

namespace fs = std::filesystem;
using Eigen::Matrix4f;
using Eigen::Vector3f;
using std::fclose;
using std::fopen;
using std::fread;
using std::make_unique;
using std::shared_ptr;
using std::size_t;
using std::unique_ptr;

Shader::Shader(const shared_ptr<spdlog::logger>& logger) : logger(logger)
{
}

Shader::~Shader()
{
    glDeleteProgram(this->id);
}

bool Shader::load_vertex_shader(const char* file_path)
{
    fs::path vertex_shader_path(file_path);
    if (!fs::exists(vertex_shader_path)) {
        logger->error("The specified vertex shader file does not exist.");
        return false;
    }
    fs::file_status status = fs::status(vertex_shader_path);
    if (!fs::is_regular_file(status)) {
        logger->error("The specified vertex shader path is not a regular file.");
        return false;
    }
    size_t size                   = static_cast<size_t>(fs::file_size(vertex_shader_path));
    vertex_shader_source          = make_unique<char[]>(size + 1);
    std::FILE* vertex_shader_file = fopen(vertex_shader_path.string().c_str(), "r");
    size_t bytes_read             = fread(vertex_shader_source.get(), size, 1, vertex_shader_file);
    fclose(vertex_shader_file);
    if (bytes_read == (size_t)0) {
        logger->warn("The vertex shader file is empty!");
    }
    logger->info("The loaded vertex shader: {}", file_path);

    return true;
}

bool Shader::load_fragment_shader(const char* file_path)
{
    fs::path fragment_shader_path(file_path);
    if (!fs::exists(fragment_shader_path)) {
        logger->error("The specified fragment shader file does not exist.");
        return false;
    }
    fs::file_status status = fs::status(fragment_shader_path);
    if (!fs::is_regular_file(status)) {
        logger->error("The specified fragment shader path is not a regular file.");
        return false;
    }
    size_t size                     = static_cast<size_t>(fs::file_size(fragment_shader_path));
    fragment_shader_source          = make_unique<char[]>(size + 1);
    std::FILE* fragment_shader_file = fopen(fragment_shader_path.string().c_str(), "r");
    size_t bytes_read = fread(fragment_shader_source.get(), size, 1, fragment_shader_file);
    fclose(fragment_shader_file);
    if (bytes_read == (size_t)0) {
        logger->warn("The fragment shader file is empty!");
    }
    logger->info("The loaded fragment shader: {}", file_path);

    return true;
}

bool Shader::compile()
{
    static constexpr unsigned int no_shader = static_cast<unsigned int>(-1);
    unsigned int vertex_shader = no_shader, fragment_shader = no_shader;
    int success;
    unique_ptr<char[]> compile_info = make_unique<char[]>(512);
    const char* shader_code         = vertex_shader_source.get();

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    shader_code   = vertex_shader_source.get();
    glShaderSource(vertex_shader, 1, &shader_code, nullptr);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, compile_info.get());
        logger->warn("Vertex shader {} compilation failed: {}", vertex_shader, compile_info.get());
        goto end_of_compilation;
    }
    logger->debug("Vertex shader {} compiled successfully.", vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    shader_code     = fragment_shader_source.get();
    glShaderSource(fragment_shader, 1, &shader_code, nullptr);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, compile_info.get());
        logger->warn("Fragment shader {} compilation failed: {}", fragment_shader,
                     compile_info.get());
        goto end_of_compilation;
    }
    logger->debug("Fragment shader {} compiled successfully.", fragment_shader);

    id = glCreateProgram();
    glAttachShader(this->id, vertex_shader);
    glAttachShader(this->id, fragment_shader);
    glLinkProgram(this->id);
    glGetProgramiv(this->id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->id, 512, nullptr, compile_info.get());
        logger->warn("Shader Program {} link failed: {}", this->id, compile_info.get());
        glDeleteProgram(this->id);
        goto end_of_compilation;
    }
    logger->info("Shader program {} link succeeded", this->id);

end_of_compilation:
    if (vertex_shader != no_shader)
        glDeleteShader(vertex_shader);
    if (fragment_shader != no_shader)
        glDeleteShader(fragment_shader);

    return static_cast<bool>(success);
}

void Shader::use() const
{
    glUseProgram(this->id);
}

template<>
bool Shader::set_uniform(const char* name, const bool& value) const
{
    int location = glGetUniformLocation(this->id, name);
    if (location == -1)
        return false;
    glUniform1i(location, value ? GL_TRUE : GL_FALSE);
    return true;
}

template<>
bool Shader::set_uniform(const char* name, const float& value) const
{
    int location = glGetUniformLocation(this->id, name);
    if (location == -1)
        return false;
    glUniform1f(location, value);
    return true;
}

template<>
bool Shader::set_uniform(const char* name, const Vector3f& value) const
{
    int location = glGetUniformLocation(this->id, name);
    if (location == -1)
        return false;
    glUniform3f(location, value.x(), value.y(), value.z());
    return true;
}

template<>
bool Shader::set_uniform(const char* name, const Matrix4f& value) const
{
    int location = glGetUniformLocation(this->id, name);
    if (location == -1)
        return false;
    glUniformMatrix4fv(location, 1, GL_FALSE, value.data());
    return true;
}
