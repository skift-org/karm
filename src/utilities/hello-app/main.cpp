#include <karm/entry>

import Karm.App;
import Karm.Drm;
import Karm.Gfx.Pixels;

using namespace Karm;

namespace Example {

struct Handler : App::Handler {
    Rc<App::Window> win;
    Rc<App::SwapChain> swapChain;

    Handler(Rc<App::Window> win)
        : win(win), swapChain(win->createSwapChain().unwrap()) {}

    void update() override {
        auto [buffer, _] = swapChain->acquire();
        auto pixels = Gfx::MutPixels::from(buffer);
        pixels.clear(Gfx::BLUE500);
        swapChain->present(buffer);
    }

    void handle(App::WindowId, App::Event& e) override {
        if (e.is<App::ResizeEvent>())
            swapChain = win->createSwapChain().unwrap();
        e.accept();
    }
};

} // namespace Example

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(env, {}, ct));
    auto win = co_trya$(app->createWindowAsync({}, ct));
    auto handler = makeRc<Example::Handler>(win);
    co_return co_await app->runAsync(handler, ct);
}
