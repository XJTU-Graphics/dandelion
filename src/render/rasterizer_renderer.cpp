#include <cstddef>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "render_engine.h"
#include "../scene/light.h"
#include "../utils/logger.h"

using std::size_t;
using std::chrono::steady_clock;
using duration   = std::chrono::duration<float>;
using time_point = std::chrono::time_point<steady_clock, duration>;
using Eigen::Vector3f;
using Eigen::Vector4f;

// vertex processor & rasterizer & fragement processor can visit
// all the static variables below from Uniforms structure
Eigen::Matrix4f Uniforms::MVP;
Eigen::Matrix4f Uniforms::inv_trans_M;
int Uniforms::width  = 0;
int Uniforms::height = 0;

GL::Material ini_material   = GL::Material();
std::list<Light> ini_lights = {};
Camera ini_camera           = Camera(Vector3f::Ones(), Vector3f::Ones(), 0.1f, 10.0f, 45.0f, 1.33f);

GL::Material& Uniforms::material   = ini_material;
std::list<Light>& Uniforms::lights = ini_lights;
Camera& Uniforms::camera           = ini_camera;

std::mutex Context::vertex_queue_mutex;
std::mutex Context::rasterizer_queue_mutex;
std::queue<VertexShaderPayload> Context::vertex_shader_output_queue;
std::queue<FragmentShaderPayload> Context::rasterizer_output_queue;

bool Context::vertex_finish     = false;
bool Context::rasterizer_finish = false;
bool Context::fragment_finish   = false;

FrameBuffer Context::frame_buffer(Uniforms::width, Uniforms::height);

FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height), color_buffer(width * height, Eigen::Vector3f(0, 0, 0)),
      depth_buffer(width * height, std::numeric_limits<float>::infinity()),
      spin_locks(width * height)
{
    for (auto& lock : spin_locks) {
        lock.unlock();
    }
}

// 光栅化渲染器的构造函数
RasterizerRenderer::RasterizerRenderer(RenderEngine& engine, int num_vertex_threads,
                                       int num_rasterizer_threads, int num_fragment_threads)
    : width(engine.width), height(engine.height), n_vertex_threads(num_vertex_threads),
      n_rasterizer_threads(num_rasterizer_threads), n_fragment_threads(num_fragment_threads),
      vertex_processor(), rasterizer(), fragment_processor(), rendering_res(engine.rendering_res)
{
    logger = get_logger("Rasterizer Renderer");
}

// 光栅化渲染器的渲染调用接口
void RasterizerRenderer::render(const Scene& scene)
{
    Uniforms::width       = static_cast<int>(width);
    Uniforms::height      = static_cast<int>(height);
    Context::frame_buffer = FrameBuffer(Uniforms::width, Uniforms::height);
    // clear Color Buffer & Depth Buffer & rendering_res
    Context::frame_buffer.clear(BufferType::Color | BufferType::Depth);
    this->rendering_res.clear();
    // run time statistics
    time_point begin_time                  = steady_clock::now();
    Camera cam                             = scene.camera;
    vertex_processor.vertex_shader_ptr     = vertex_shader;
    fragment_processor.fragment_shader_ptr = phong_fragment_shader;
    for (const auto& group : scene.groups) {
        for (const auto& object : group->objects) {
            Context::vertex_finish     = false;
            Context::rasterizer_finish = false;
            Context::fragment_finish   = false;

            std::vector<std::thread> workers;
            for (int i = 0; i < n_vertex_threads; ++i) {
                workers.emplace_back(&VertexProcessor::worker_thread, &vertex_processor);
            }
            for (int i = 0; i < n_rasterizer_threads; ++i) {
                workers.emplace_back(&Rasterizer::worker_thread, &rasterizer);
            }
            for (int i = 0; i < n_fragment_threads; ++i) {
                workers.emplace_back(&FragmentProcessor::worker_thread, &fragment_processor);
            }

            // set Uniforms for vertex shader
            Uniforms::MVP         = cam.projection() * cam.view() * object->model();
            Uniforms::inv_trans_M = object->model().inverse().transpose();
            Uniforms::width       = static_cast<int>(this->width);
            Uniforms::height      = static_cast<int>(this->height);
            // To do: 同步
            Uniforms::material = object->mesh.material;
            Uniforms::lights   = scene.lights;
            Uniforms::camera   = scene.camera;

            // input object->mesh's vertices & faces & normals data
            const std::vector<float>& vertices     = object->mesh.vertices.data;
            const std::vector<unsigned int>& faces = object->mesh.faces.data;
            const std::vector<float>& normals      = object->mesh.normals.data;
            size_t num_faces                       = faces.size();

            // process vertices
            for (size_t i = 0; i < num_faces; i += 3) {
                for (size_t j = 0; j < 3; j++) {
                    size_t idx = faces[i + j];
                    vertex_processor.input_vertices(
                        Vector4f(vertices[3 * idx], vertices[3 * idx + 1], vertices[3 * idx + 2],
                                 1.0f),
                        Vector3f(normals[3 * idx], normals[3 * idx + 1], normals[3 * idx + 2]));
                }
            }
            vertex_processor.input_vertices(Eigen::Vector4f(0, 0, 0, -1.0f),
                                            Eigen::Vector3f::Zero());
            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }
    }

    time_point end_time         = steady_clock::now();
    duration rendering_duration = end_time - begin_time;

    this->logger->info("rendering (single thread) takes {:.6f} seconds",
                       rendering_duration.count());

    for (long unsigned int i = 0; i < Context::frame_buffer.depth_buffer.size(); i++) {
        rendering_res.push_back(
            static_cast<unsigned char>(Context::frame_buffer.color_buffer[i].x()));
        rendering_res.push_back(
            static_cast<unsigned char>(Context::frame_buffer.color_buffer[i].y()));
        rendering_res.push_back(
            static_cast<unsigned char>(Context::frame_buffer.color_buffer[i].z()));
    }
}

void VertexProcessor::input_vertices(const Vector4f& positions, const Vector3f& normals)
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
