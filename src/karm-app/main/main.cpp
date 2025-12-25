#include <karm-sys/entry.h>

import Karm.App;
import Karm.Gfx;

using namespace Karm;

namespace Example {

struct Handler : App::Handler {
    Rc<App::Window> win;

    Handler(Rc<App::Window> win)
        : win(win) {}

    void update() override {
        auto s = win->acquireSurface();
        s.clear(Gfx::BLUE500);
        win->releaseSurface();
    }

    void handle(App::WindowId, App::Event& e) override {
        e.accept();
    }
};

} // namespace Example

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(ctx, {}, ct));
    auto win = co_trya$(app->createWindowAsync({}, ct));
    auto handler = makeRc<Example::Handler>(win);
    co_return co_await app->runAsync(handler, ct);
}
