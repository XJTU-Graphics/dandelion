#include "render_engine.h"

Eigen::Vector3f RenderEngine::background_color(RGB_COLOR(100, 100, 100));

RenderEngine::RenderEngine()
{
    // unique pointer to Rasterizer Renderer
    rasterizer_render = std::make_unique<RasterizerRenderer>(*this, 1, 2, 2);
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
    // case RendererType::RASTERIZER_MT: rasterizer_render->render_mt(scene); break;
    case RendererType::WHITTED_STYLE: whitted_render->render(scene); break;
    default: break;
    }
}

void SpinLock::lock()
{
    while (true) {
        if (!this->locked.test_and_set())
            return;
        do {
            for (std::atomic<int> i = 0; i < 150; ++i);
        } while (this->locked.test());
    }
}

void SpinLock::unlock()
{
    this->locked.clear();
}

void FrameBuffer::clear(BufferType buff)
{
    if ((buff & BufferType::Color) == BufferType::Color) {
        fill(color_buffer.begin(), color_buffer.end(), RenderEngine::background_color * 255.0f);
    }
    if ((buff & BufferType::Depth) == BufferType::Depth) {
        fill(depth_buffer.begin(), depth_buffer.end(), std::numeric_limits<float>::infinity());
    }
}

void FrameBuffer::set_pixel(int index, float depth, const Eigen::Vector3f& color)
{
    // spin locks lock
    spin_locks[index].lock();

    depth_buffer[index] = depth;
    color_buffer[index] = color;

    // spin locks unlock
    spin_locks[index].unlock();
}
