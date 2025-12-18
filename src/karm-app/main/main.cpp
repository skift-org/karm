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

Async::Task<> entryPointAsync(Sys::Context&, Async::CancellationToken ct) {
    auto app = co_try$(App::Application::create());
    auto win = co_try$(app->createWindow());
    auto handler = makeRc<Example::Handler>(win);
    co_return co_await app->runAsync(handler, ct);
}
