#include "rasterizer_renderer.h"
#include "../utils/math.hpp"
#include <cstdio>

#ifdef _WIN32
#undef min
#undef max
#endif

using Eigen::Vector3f;
using Eigen::Vector4f;

void VertexProcessor::input_vertices(const Eigen::Vector4f& positions,
                                     const Eigen::Vector3f& normals)
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    VertexShaderPayload payload;
    payload.world_position = positions;
    payload.normal         = normals;
    vertex_queue.push(payload);
}

void VertexProcessor::worker_thread()
{
    while (true) {
        VertexShaderPayload payload;
        {
            if (vertex_queue.empty()) {
                continue;
            }
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (vertex_queue.empty()) {
                continue;
            }
            payload = vertex_queue.front();
            vertex_queue.pop();
        }
        if (payload.world_position.w() == -1.0f) {
            Context::vertex_finish = true;
            return;
        }
        VertexShaderPayload output_payload = vertex_shader_ptr(payload);
        {
            std::unique_lock<std::mutex> lock(Context::vertex_queue_mutex);
            Context::vertex_shader_output_queue.push(output_payload);
        }
    }
}

// vertex shader
VertexShaderPayload vertex_shader(const VertexShaderPayload& payload)
{
    VertexShaderPayload output_payload = payload;

    // Vertex position transformation

    // Viewport transformation

    // Vertex normal transformation

    return output_payload;
}

void FragmentProcessor::worker_thread()
{
    while (true) {
        FragmentShaderPayload fragment;
        {
            if (Context::rasterizer_finish && Context::rasterizer_output_queue.empty()) {
                Context::fragment_finish = true;
                return;
            }
            if (Context::rasterizer_output_queue.empty()) {
                continue;
            }
            std::unique_lock<std::mutex> lock(Context::rasterizer_queue_mutex);
            if (Context::rasterizer_output_queue.empty()) {
                continue;
            }
            fragment = Context::rasterizer_output_queue.front();
            Context::rasterizer_output_queue.pop();
        }
        int index = (Uniforms::height - 1 - fragment.y) * Uniforms::width + fragment.x;
        if (fragment.depth > Context::frame_buffer.depth_buffer[index]) {
            continue;
        }
        fragment.color =
            fragment_shader_ptr(fragment, Uniforms::material, Uniforms::lights, Uniforms::camera);
        Context::frame_buffer.set_pixel(index, fragment.depth, fragment.color);
    }
}

Vector3f phong_fragment_shader(const FragmentShaderPayload& payload, const GL::Material& material,
                               const std::list<Light>& lights, const Camera& camera)
{
    // these lines below are just for compiling and can be deleted
    (void)payload;
    (void)material;
    (void)lights;
    (void)camera;
    // these lines above are just for compiling and can be deleted

    Vector3f result = {0, 0, 0};

    // ka,kd,ks can be got from material.ambient,material.diffuse,material.specular

    // set ambient light intensity

    // Light Direction

    // View Direction

    // Half Vector

    // Light Attenuation

    // Ambient

    // Diffuse

    // Specular

    // set rendering result max threshold to 255
    return result * 255.f;
}
