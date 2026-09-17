#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <limits>
#include <string>
#include <vector>

#include <catch2/catch_amalgamated.hpp>
#include <Eigen/Core>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "../src/scene/scene.h"
#include "../src/render/render_engine.h"

using Eigen::Vector3f;
namespace fs = std::filesystem;

TEST_CASE("Rasterizer Renderer", "[rendering]")
{
    // 参考图由正确的渲染器生成，只有光栅化时的线程竞争和浮点误差会带来少量像素差异；
    // 30dB 足以容忍这些噪声，而渲染逻辑错误通常会使 PSNR 掉到 20dB 以下。
    constexpr double psnr_threshold = 30.0;
    // 保存的场景数据中没有背景色，约定所有测试场景都使用纯黑色背景渲染。
    RenderEngine::background_color = Vector3f(0.0f, 0.0f, 0.0f);

    const fs::path input_dir("../input/rendering");
    const fs::path ans_dir("../ans/rendering/rasterizer_renderer");

    std::vector<fs::path> scene_dirs;
    for (const fs::directory_entry& entry: fs::directory_iterator(input_dir)) {
        if (entry.is_directory()) {
            scene_dirs.push_back(entry.path());
        }
    }
    std::sort(scene_dirs.begin(), scene_dirs.end());
    REQUIRE(!scene_dirs.empty());

    for (const fs::path& scene_dir: scene_dirs) {
        const std::string scene_name = scene_dir.filename().string();
        spdlog::info("rendering test case: {}", scene_name);

        Scene scene;
        REQUIRE(scene.load(scene_dir.generic_string()));

        // 与 GUI 一致：渲染宽度固定为 480，高度按离线渲染相机的宽高比推导。
        RenderEngine engine;
        engine.width  = 480.0f;
        engine.height = std::floor(engine.width / scene.camera.aspect_ratio);
        engine.render(scene, RendererType::RASTERIZER);

        const fs::path ans_path  = ans_dir / (scene_name + ".png");
        int            ans_width = 0, ans_height = 0, ans_channels = 0;
        unsigned char* ans_data =
            stbi_load(ans_path.generic_string().c_str(), &ans_width, &ans_height, &ans_channels, 3);
        REQUIRE(ans_data != nullptr);
        REQUIRE(ans_width == static_cast<int>(engine.width));
        REQUIRE(ans_height == static_cast<int>(engine.height));

        double squared_error = 0.0;
        for (size_t i = 0; i < engine.rendering_res.size(); ++i) {
            const double diff =
                static_cast<double>(engine.rendering_res[i]) - static_cast<double>(ans_data[i]);
            squared_error += diff * diff;
        }
        stbi_image_free(ans_data);

        const double mse  = squared_error / static_cast<double>(engine.rendering_res.size());
        const double psnr = (mse == 0.0) ? std::numeric_limits<double>::infinity()
                                         : 10.0 * std::log10(255.0 * 255.0 / mse);
        INFO(std::format("scene: {}, PSNR: {:.2f} dB", scene_name, psnr));
        REQUIRE(psnr > psnr_threshold);
    }
}
