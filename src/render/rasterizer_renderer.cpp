#include <algorithm>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "rasterizer.h"
#include "render_engine.h"
#include "shader.h"
#include "triangle.h"
#include "../scene/light.h"
#include "../utils/logger.h"

using std::chrono::steady_clock;
using duration   = std::chrono::duration<float>;
using time_point = std::chrono::time_point<steady_clock, duration>;
using Eigen::Vector3f;
using Eigen::Vector4f;

// 光栅化渲染器的构造函数
RasterizerRenderer::RasterizerRenderer(RenderEngine& engine)
    : width(engine.width), height(engine.height), n_threads(engine.n_threads),
      rendering_res(engine.rendering_res)
{
    logger = get_logger("Rasterizer Renderer");
}

// 光栅化渲染器的渲染调用接口
void RasterizerRenderer::render(const Scene& scene)
{
    // initialize rasterizer
    Rasterizer r(static_cast<int>(width), static_cast<int>(height));

    // choose the active shader for fragment shader
    std::function<Vector3f(FragmentShaderPayload, GL::Material, std::list<Light>, Camera)>
        active_shader = phong_fragment_shader;
    // set vertex shader
    r.vertex_shader = vertex_shader;
    // set fragment shader
    r.fragment_shader = active_shader;

    // clear Color Buffer & Depth Buffer & rendering_res
    r.clear(BufferType::Color | BufferType::Depth);
    this->rendering_res.clear();
    // run time statistics
    time_point begin_time = steady_clock::now();
    for (const auto& group : scene.groups) {

        Camera cam = scene.camera;
        // set r.view & r.projection
        r.view       = cam.view();
        r.projection = cam.projection();

        for (const auto& object : group->objects) {
            // set r.model
            r.model = object->model();
            // set Uniforms for vertex shader
            Uniforms::MVP         = r.projection * r.view * r.model;
            Uniforms::inv_trans_M = r.model.inverse().transpose();
            Uniforms::width       = r.width;
            Uniforms::height      = r.height;
            // input object->mesh's vertices & faces & normals data
            std::vector<Triangle> TriangleList;
            const std::vector<float>& vertices     = object->mesh.vertices.data;
            const std::vector<unsigned int>& faces = object->mesh.faces.data;
            const std::vector<float>& normals      = object->mesh.normals.data;

            for (unsigned int i = 0; i < faces.size(); i += 3) {
                // set triangle list(vertex & normal)
                TriangleList.emplace_back();
                Triangle& t = *TriangleList.rbegin();
                for (int j = 0; j < 3; j++) {
                    unsigned int idx = faces[i + j];
                    t.vertex[j]      = Vector4f(vertices[3 * idx], vertices[3 * idx + 1],
                                                vertices[3 * idx + 2], 1.0f);
                    t.normal[j] =
                        Vector3f(normals[3 * idx], normals[3 * idx + 1], normals[3 * idx + 2]);
                }
            }
            // call r.draw()
            r.draw(TriangleList, object->mesh.material, scene.lights, cam);
        }
    }
    time_point end_time         = steady_clock::now();
    duration rendering_duration = end_time - begin_time;

    this->logger->info("rendering (single thread) takes {:.6f} seconds",
                       rendering_duration.count());

    // OutImage can be saved at the working directory as .ppm
    std::ofstream output_image;
    output_image.open("rasterizer_res.ppm");
    int nx = static_cast<int>(width);
    int ny = static_cast<int>(height);
    output_image << "P3\n" << nx << ' ' << ny << "\n255\n";

    for (long unsigned int i = 0; i < r.depth_buf.size(); i++) {
        rendering_res.push_back(static_cast<unsigned char>(r.frame_buf[i].x()));
        rendering_res.push_back(static_cast<unsigned char>(r.frame_buf[i].y()));
        rendering_res.push_back(static_cast<unsigned char>(r.frame_buf[i].z()));

        output_image << int(r.frame_buf[i].x()) << ' ' << int(r.frame_buf[i].y()) << ' '
                     << int(r.frame_buf[i].z()) << '\n';
    }
}
