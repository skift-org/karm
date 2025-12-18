module;

#include <karm-core/macros.h>

export module Karm.Ui:app;

import Karm.Sys;
import Karm.App;
import Karm.Math;
import Karm.Logger;

import :node;
import :atoms;

namespace Karm::Ui {

export auto FRAME_RATE = 60;
export auto FRAME_TIME = 1.0 / FRAME_RATE;

struct Handler : App::Handler {
    Rc<App::Window> _window;
    Child _root;

    Handler(Rc<App::Window> window, Child root)
        : _window(window), _root(root) {}

    void update() override {
        auto pixels = _window->acquireSurface();

        auto e = App::makeEvent<Node::AnimateEvent>(FRAME_TIME);
        _root->event(*e);

        _root->layout(_window->bound().size());

        Gfx::CpuCanvas g;
        g.begin(pixels);
        g.push();

        g.clip(pixels.bound().cast<f64>());
        g.clear(pixels.bound(), GRAY950);
        g.fillStyle(GRAY50);

        // NOTE: Since we are applying the scale factor,
        // now we need to operate in the window logical space
        g.scale(_window->scaleFactor());
        _root->paint(g, _window->bound().size());

        g.pop();

        _window->releaseSurface();
    }

    void handle(App::WindowId, App::Event& e) override {
        _root->event(e);
    }
};

export Async::Task<> runAsync(Sys::Context&, Child root, Async::CancellationToken ct) {
    auto app = co_try$(App::Application::create());
    auto size = root->size({1024, 720}, Hint::MIN);
    auto win = co_try$(app->createWindow({
        .title = "Karm UI Application"s,
        .size = size,
    }));
    auto handler = makeRc<Handler>(win, root);
    co_return co_await app->runAsync(handler, ct);
}

} // namespace Karm::Ui
