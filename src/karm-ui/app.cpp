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

struct RootNode : ProxyNode<RootNode> {
    Rc<App::Window> _window;

    RootNode(Child child, Rc<App::Window> window)
        : ProxyNode(child), _window(window) {}

    void update() {
        auto pixels = _window->acquireSurface();

        auto e = App::makeEvent<Node::AnimateEvent>(FRAME_TIME);
        child().event(*e);

        child().layout(_window->bound().size());

        Gfx::CpuCanvas g;
        g.begin(pixels);
        g.push();

        g.clip(pixels.bound().cast<f64>());
        g.clear(pixels.bound(), GRAY950);
        g.fillStyle(GRAY50);

        // NOTE: Since we are applying the scale factor,
        // now we need to operate in the window logical space
        g.scale(_window->scaleFactor());
        child().paint(g, _window->bound().size());

        g.pop();

        _window->releaseSurface();
    }

    void bubble(App::Event& event) override {
        if (auto e = event.is<Node::PaintEvent>()) {
            event.accept();
        } else if (auto e = event.is<Node::LayoutEvent>()) {
            event.accept();
        } else if (auto e = event.is<Node::AnimateEvent>()) {
            event.accept();
        } else if (auto e = event.is<App::DragEvent>()) {
            _window->drag(*e);
            event.accept();
        }

        if (not event.accepted()) {
            logWarn("unhandled event, bouncing down");
            this->event(event);
        }
    }
};

struct Handler : App::Handler {
    Rc<RootNode> _root;

    Handler(Rc<RootNode> root) : _root(root) {}

    void update() override {
        _root->update();
    }

    void handle(App::WindowId, App::Event& e) override {
        _root->event(e);
    }
};

export Async::Task<> runAsync(Sys::Context& ctx, Child child, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(ctx, {}, ct));
    auto size = child->size({1024, 720}, Hint::MIN);
    auto win = co_trya$(app->createWindowAsync(
        {
            .size = size,
        },
        ct
    ));
    auto root = makeRc<RootNode>(child, win);
    auto handler = makeRc<Handler>(root);
    co_return co_await app->runAsync(handler, ct);
}

} // namespace Karm::Ui
