export module Karm.App:application;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Sys;

import Karm.App.Base;
import Karm.Logger;
import :_embed;

namespace Karm::App {

export using WindowId = Distinct<usize, struct WindowIdTag>;
export constexpr WindowId GLOBAL = WindowId{Limits<usize>::MAX};

export enum struct Direction : u8 {
    EAST,
    NORTH,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH,
    SOUTH_EAST,
    SOUTH_WEST,
    WEST,
};

export struct WindowProps {
    String title = "Karm Application"s;
    Math::Vec2i size = {800, 600};
};

export struct Window : Meta::Pinned {
    virtual ~Window() = default;

    virtual WindowId id() = 0;

    virtual Math::Recti bound() = 0;

    virtual f64 scaleFactor() = 0;

    virtual Gfx::MutPixels acquireSurface() = 0;

    void releaseSurface(Math::Recti r) {
        Array dirty = {r};
        releaseSurface(dirty);
    }

    virtual void releaseSurface(Slice<Math::Recti> dirty) = 0;

    virtual void drag() {
        logWarn("Window::drag() not implemented");
    }

    virtual void resize(Direction) {
        logWarn("Window::resize() not implemented");
    }

    virtual void snap(Snap) {
        logWarn("Window::snap() not implemented");
    }

    virtual void minimize() {
        logWarn("Window::minimize() not implemented");
    }

    virtual void close() = 0;
};

export struct Handler {
    virtual ~Handler() = default;

    virtual void update() {}

    virtual void handle(WindowId windowId, Event& e) = 0;

    template <typename E, typename... Args>
    void handle(WindowId windowId, Args&&... args) {
        auto e = App::makeEvent<E>(std::forward<Args>(args)...);
        handle(windowId, *e);
    }
};

export struct ApplicationProps {
    static ApplicationProps simple() {
        return {};
    }
};

export struct Application : Meta::Pinned {
    virtual ~Application() = default;

    [[clang::coro_wrapper]]
    static Async::Task<Rc<Application>> createAsync(Sys::Context& ctx, ApplicationProps const& props, Async::CancellationToken ct) {
        return _Embed::createAppAsync(ctx, props, ct);
    }

    virtual Async::Task<Rc<Window>> createWindowAsync(WindowProps const& props, Async::CancellationToken ct) = 0;

    virtual Async::Task<> runAsync(Rc<Handler> handler, Async::CancellationToken ct) = 0;
};

} // namespace Karm::App
