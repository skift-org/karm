module;

#include <karm/macros>

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
    bool _shouldAnimate = true;
    bool _shouldLayout = true;
    Vec<Math::Recti> _dirty;

    RootNode(Child child, Rc<App::Window> window)
        : ProxyNode(child), _window(window) {}

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        g.push();
        g.scale(_window->scaleFactor());
        g.clip(r);
        g.clear(GRAY900);
        g.fillStyle(GRAY50);
        child().paint(g, r);
        g.pop();
    }

    void update() {
        if (_shouldAnimate) {
            _shouldAnimate = false;

            auto e = App::makeEvent<AnimateEvent>(FRAME_TIME);
            child().event(*e);
        }

        if (_shouldLayout) {
            _shouldLayout = false;
            _shouldAnimate = true;

            child().layout(_window->bound().size());

            _dirty.clear();
            _dirty.pushBack(_window->bound().size());
        }

        auto pixels = _window->acquireSurface();
        if (_dirty.len()) {
            Gfx::CpuCanvas g;
            g.begin(pixels);
            for (auto& d : _dirty) {
                paint(g, d);
            }
            g.end();
        }

        _window->releaseSurface(_dirty);
        _dirty.clear();
    }

    void event(App::Event& event) override {
        if (auto e = event.is<App::ResizeEvent>()) {
            _shouldLayout = true;
            event.accept();
        }

        if (not event.accepted())
            ProxyNode::event(event);
    }

    void bubble(App::Event& event) override {
        if (auto e = event.is<PaintEvent>()) {
            _dirty.pushBack(e->bound);
            event.accept();
        } else if (auto e = event.is<LayoutEvent>()) {
            _shouldLayout = true;
            event.accept();
        } else if (auto e = event.is<AnimateEvent>()) {
            _shouldAnimate = true;
            event.accept();
        } else if (event.is<App::DragStartEvent>()) {
            _window->drag();
            event.accept();
        } else if (auto e = event.is<App::RequestCloseEvent>()) {
            _window->close();
            event.accept();
        } else if (auto e = event.is<App::RequestSnapeEvent>()) {
            _window->snap(e->snap);
            event.accept();
        } else if (auto e = event.is<App::RequestMinimizeEvent>()) {
            _window->minimize();
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
