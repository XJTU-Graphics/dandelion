#include <algorithm>
#include <chrono>
#include <format>
#include <limits>
#include <thread>

#include <catch2/catch_amalgamated.hpp>
#include <Eigen/Core>
#include <spdlog/spdlog.h>

#include "../src/scene/scene.h"
#include "../src/render/render_engine.h"

using Eigen::Vector3f;
using std::chrono::duration;
using std::chrono::steady_clock;

namespace {

// 加速比阈值：同一场景下 3 线程（纯流水）与 8 线程（流水 + 阶段内并行）的渲染耗时之比，
// 低于该值说明阶段内并行化没有起到预期的加速效果。
constexpr double speedup_threshold = 2.0;
// 每个线程配置的测量次数，取最小值以降低系统噪声的影响。
constexpr int measurement_repeats = 3;
// 初始加载的 cow 模型数量：cow.dae 单个约 6000 面，10 个约 6 万面。
constexpr int initial_cow_count = 10;
// (1,1,1) 配置下单次渲染时间超过该值（秒）时，将模型数量减半后重新估计，
// 避免多线程冷启动之外的冗余开销拖慢测试；同时应保证该时间不小于 1 秒。
constexpr double max_single_render_seconds = 4.0;

// 渲染分辨率：比正确性测试的 480 宽更大，使三个阶段都有足够负载。
constexpr float render_width  = 1920.0f;
constexpr float render_height = 1440.0f;

// 加载 cow_count 个 cow.dae、施加不同平移，构造大场景；相机和光源参数固定。
void build_scene(Scene& scene, int cow_count)
{
    scene.clear();
    for (int i = 0; i < cow_count; ++i) {
        if (!scene.import_model("../input/geometry/cow.dae")) {
            throw std::runtime_error("failed to import cow.dae");
        }
        // cow.dae 的坐标范围大致不超过正负 0.5，按 1.5 的间距排列即可避免重叠
        const float x = (static_cast<float>(i % 4) - 1.5f) * 1.5f;
        const float z = (static_cast<float>(i / 4) - 1.0f) * 1.5f;
        for (const std::unique_ptr<Object>& object: scene.groups.back()->objects) {
            object->center = Vector3f(x, 0.0f, z);
        }
    }
    scene.camera = Camera(
        Vector3f(0.0f, 5.0f, 10.0f), Vector3f(0.0f, 0.5f, 0.0f), 0.1f, 100.0f, 45.0f,
        render_width / render_height
    );
    scene.lights.clear();
    scene.lights.emplace_back(Vector3f(5.0f, 8.0f, 8.0f), 40.0f);
    scene.lights.emplace_back(Vector3f(-5.0f, 6.0f, 4.0f), 15.0f);
}

// 以 (n_vertex, n_rasterizer, n_fragment) 的线程配置渲染 scene repeats 次，返回最小耗时（秒）。
double measure_min_render_time(
    Scene& scene, int n_vertex_threads, int n_rasterizer_threads, int n_fragment_threads,
    int repeats
)
{
    RenderEngine engine;
    engine.width  = render_width;
    engine.height = render_height;
    RasterizerRenderer renderer(engine, n_vertex_threads, n_rasterizer_threads, n_fragment_threads);

    double min_time = std::numeric_limits<double>::infinity();
    for (int i = 0; i < repeats; ++i) {
        const steady_clock::time_point begin = steady_clock::now();
        renderer.render(scene);
        const double elapsed = duration<double>(steady_clock::now() - begin).count();
        min_time             = std::min(min_time, elapsed);
    }
    return min_time;
}

} // namespace

TEST_CASE("Rasterizer Renderer Parallel Speedup", "[.][benchmark]")
{
    if (std::thread::hardware_concurrency() < 8) {
        SKIP("hardware concurrency is less than 8, parallel speedup cannot be measured");
    }

    // 先用 (1,1,1) 配置预热并估计单次渲染时间，超过上限就将模型数量减半重新估计。
    // 这次渲染同时消除了多线程和缓存的冷启动开销。
    Scene  scene;
    int    cow_count = initial_cow_count;
    double pipeline_time;
    while (true) {
        build_scene(scene, cow_count);
        pipeline_time = measure_min_render_time(scene, 1, 1, 1, 1);
        if (pipeline_time <= max_single_render_seconds || cow_count == 1) {
            break;
        }
        cow_count = std::max(1, cow_count / 2);
        spdlog::info(
            "single render takes {:.2f}s, halving cow count to {}", pipeline_time, cow_count
        );
    }
    if (pipeline_time < 1.0) {
        spdlog::warn(
            "single render takes only {:.2f}s, speedup measurement may be noisy", pipeline_time
        );
    }

    // 正式测量：3 线程（1,1,1）只有流水优化；8 线程（2,2,4）每个阶段内部也并行。
    pipeline_time              = measure_min_render_time(scene, 1, 1, 1, measurement_repeats);
    const double parallel_time = measure_min_render_time(scene, 2, 2, 4, measurement_repeats);
    const double speedup       = pipeline_time / parallel_time;
    INFO(
        std::format(
            "cows: {}, 3-thread: {:.2f}s, 8-thread: {:.2f}s, speedup: {:.2f}", cow_count,
            pipeline_time, parallel_time, speedup
        )
    );
    REQUIRE(speedup > speedup_threshold);
}
