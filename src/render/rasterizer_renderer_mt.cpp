#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <mutex>
#include <thread>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "rasterizer.h"
#include "render_engine.h"
#include "shader.h"
#include "triangle.h"
#include "../scene/light.h"

using std::chrono::steady_clock;
using duration   = std::chrono::duration<float>;
using time_point = std::chrono::time_point<steady_clock, duration>;
using Eigen::Vector3f;
using Eigen::Vector4f;

void RasterizerRenderer::render_mt(const Scene& scene)
{
    Rasterizer r(static_cast<int>(width), static_cast<int>(height));

    // choose the active shader for fragment shader
    std::function<Vector3f(FragmentShaderPayload, GL::Material, const std::list<Light>&, Camera)>
        active_shader = phong_fragment_shader;

    r.vertex_shader   = vertex_shader;
    r.fragment_shader = active_shader;

    // clear Color Buffer & Depth Buffer & rendering_res
    r.clear(BufferType::Color | BufferType::Depth);
    this->rendering_res.clear();

    time_point begin_time = steady_clock::now();



    
    // this line below is just for compiling and can be deleted
    (void)scene;
    
    // multi-thread render_mt should be completed here

    
    
    
    time_point end_time         = steady_clock::now();
    duration rendering_duration = end_time - begin_time;

    this->logger->info("rendering ({} threads) takes {:.6f} seconds", n_threads,
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
