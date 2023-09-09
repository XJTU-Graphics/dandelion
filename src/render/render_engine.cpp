#include "render_engine.h"

Eigen::Vector3f RenderEngine::background_color(RGB(100, 100, 100));

RenderEngine::RenderEngine()
{
    // unique pointer to Rasterizer Renderer
    rasterizer_render = std::make_unique<RasterizerRenderer>(*this);
    // unique pointer to Whitted Style Renderer
    whitted_render = std::make_unique<WhittedRenderer>(*this);
    // default setting of number of threads(if use multi-threads edition)
    n_threads = 4;
}

// choose render type
void RenderEngine::render(Scene& scene, RendererType type)
{
    switch (type) {
    case RendererType::RASTERIZER: rasterizer_render->render(scene); break;
    case RendererType::RASTERIZER_MT: rasterizer_render->render_mt(scene); break;
    case RendererType::WHITTED_STYLE: whitted_render->render(scene); break;
    default: break;
    }
}
