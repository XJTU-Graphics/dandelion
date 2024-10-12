#ifndef DANDELION_RENDER_RASTERIZER_RENDERER_H
#define DANDELION_RENDER_RASTERIZER_RENDERER_H

#include <list>
#include <queue>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "../platform/gl.hpp"
#include "../scene/light.h"
#include "../scene/camera.h"
#include "graphics_interface.h"

/*!
 * \file render/rasterizer_renderer.h
 * \ingroup rendering
 * \~chinese
 * \brief 光栅化渲染器中顶点处理、片元处理两个阶段的实现。
 */
class VertexProcessor
{
public:
    void input_vertices(const Eigen::Vector4f& positions, const Eigen::Vector3f& normals);
    VertexShaderPayload (*vertex_shader_ptr)(const VertexShaderPayload& payload);
    void worker_thread();

private:
    std::queue<VertexShaderPayload> vertex_queue;
    std::mutex queue_mutex;
};

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 负责执行片元着色器的工作线程。
 */
class FragmentProcessor
{
public:
    void worker_thread();

    Eigen::Vector3f (*fragment_shader_ptr)(const FragmentShaderPayload& payload,
                                           const GL::Material& material,
                                           const std::list<Light>& lights, const Camera& camera);

private:
};

#endif // DANDELION_RENDER_RASTERIZER_RENDERER_H
