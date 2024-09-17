#ifndef DANDELION_RENDER_RASTERIZER_RENDERER_H
#define DANDELION_RENDER_RASTERIZER_RENDERER_H

#include <vector>
#include <list>
#include <queue>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "../platform/gl.hpp"
#include "../scene/light.h"
#include "../scene/camera.h"
#include "graphics_interface.h"

using Eigen::Vector3f;

/*!
 * \~chinese
 * \brief 作用于每个顶点，通常是处理从世界空间到屏幕空间的坐标变化，后面紧接着光栅化。
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

class FragmentProcessor
{
public:
    void worker_thread();

    Eigen::Vector3f (*fragment_shader_ptr)(const FragmentShaderPayload& payload,
                                           const GL::Material& material, const std::list<Light>& lights,
                                           const Camera& camera);

private:
};

#endif // DANDELION_RENDER_RASTERIZER_RENDERER_H