#include <karm/entry>

import Karm.Sys;
import Karm.Gpu;
import Karm.App;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

struct Handler : App::Handler {
    Rc<Gpu::Device> _device;
    Rc<Gpu::Pipeline> _pipeline;
    Rc<App::Window> _window;
    Rc<App::SwapChain> _swapChain;

    Handler(Rc<Gpu::Device> device, Rc<Gpu::Pipeline> pipeline, Rc<App::Window> window)
        : _device(device),
          _pipeline(pipeline),
          _window(window),
          _swapChain(window->createSwapChain().unwrap()) {}

    void handle(App::WindowId, App::Event& e) override {
        if (e.is<App::ResizeEvent>())
            _swapChain = _window->createSwapChain().unwrap();
    }

    void update() override {
        auto [drmBuffer, _] = _swapChain->acquire();
        auto windowTexture = _device->createTexture(
            drmBuffer,
            Gpu::UsageFlags::COLOR_ATTACHMENT
        );

        auto queue = _device->createQueue(Gpu::QueueType::DEFAULT);
        auto cmd = queue->startCommandRecording();

        cmd->beginRenderPass({
            .colorAttachments = {
                Gpu::RenderAttachment{
                    .texture = windowTexture,
                    .loadOp = Gpu::LoadOp::CLEAR,
                    .storeOp = Gpu::StoreOp::STORE,
                    .clearColor = {1.0, 0.0, 0.0, 1.0},
                },
            },
            .renderArea = {_swapChain->size.cast<isize>()},
        });

        cmd->pipeline(_pipeline);
        cmd->draw(NONE, NONE, 3, 1);
        cmd->endRenderPass();

        cmd->finalize();
        queue->submit({cmd});
        _device->waitForIdle();
        _swapChain->present(drmBuffer);
    }
};

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(env, {}, ct));
    auto window = co_trya$(app->createWindowAsync({}, ct));
    auto device = co_try$(Gpu::Device::create({}));
    auto shader = co_try$(Sys::readAll("bundle://hello-gpu/shader.spv"_url));
    auto pipeline = device->createGraphicPipeline(
        {
            .source = shader,
            .entryPoint = "vertex_main"s,
        },
        {
            .source = shader,
            .entryPoint = "fragment_main"s,
        },
        {
            .colorTargets = {
                Gpu::ColorTarget{
                    .format = Gpu::Format::BGRA8_UNORM,
                },
            },
        }
    );
    co_return co_await app->runAsync(
        makeRc<Handler>(device, pipeline, window),
        ct
    );
}
