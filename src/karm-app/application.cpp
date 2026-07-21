export module Karm.App:application;

import Karm.Core;
import Karm.Drm;
import Karm.Math;
import Karm.Sys;

import Karm.App.Base;
import Karm.Logger;
import :_embed;

using namespace Karm::Literals;

namespace Karm::App {

export using WindowId = Distinct<usize, struct WindowIdTag>;
export constexpr WindowId GLOBAL = WindowId{Limits<usize>::MAX};

export constexpr HitResult resizeHit(Direction dir) {
    switch (dir) {
    case Direction::EAST:
        return HitResult::RESIZE_EAST;
    case Direction::NORTH:
        return HitResult::RESIZE_NORTH;
    case Direction::NORTH_EAST:
        return HitResult::RESIZE_NORTH_EAST;
    case Direction::NORTH_WEST:
        return HitResult::RESIZE_NORTH_WEST;
    case Direction::SOUTH:
        return HitResult::RESIZE_SOUTH;
    case Direction::SOUTH_EAST:
        return HitResult::RESIZE_SOUTH_EAST;
    case Direction::SOUTH_WEST:
        return HitResult::RESIZE_SOUTH_WEST;
    case Direction::WEST:
        return HitResult::RESIZE_WEST;
    default:
        unreachable();
    }
}

export enum struct PresentMode : u8 {
    PACED,
    IMMEDIATE,

    _LEN,
};

export struct SwapChainProps {
    Drm::Format format;
    u8 size = 2;
    PresentMode mode = PresentMode::PACED;
};

export struct AcquiredBuffer {
    Rc<Drm::Buffer> buffer;
    u8 age;
};

export struct SwapChain {
    Drm::Format format;
    Math::Vec2u size;

    virtual ~SwapChain() = default;

    virtual AcquiredBuffer acquire() = 0;

    void present(Rc<Drm::Buffer> buf) {
        Math::Recti rect{buf->size.cast<isize>()};
        present(buf, rect);
    }

    virtual void present(Rc<Drm::Buffer>, Slice<Math::Recti> dirty) = 0;
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

    virtual Res<Rc<SwapChain>> createSwapChain(SwapChainProps const& props = {}) = 0;

    virtual void drag() {
        logWarn("Window::drag() not implemented");
    }

    virtual void resize(Direction) {
        logWarn("Window::resize() not implemented");
    }

    virtual void cursor(CursorStyle) {
        logWarn("Window::cursor() not implemented");
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

    virtual HitResult hitTest([[maybe_unused]] WindowId windowId, [[maybe_unused]] Math::Vec2i pos) {
        return HitResult::NORMAL;
    }

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
    static Async::Task<Rc<Application>> createAsync(Sys::Env& env, ApplicationProps const& props, Async::CancellationToken ct) {
        return _Embed::createAppAsync(env, props, ct);
    }

    virtual Async::Task<Rc<Window>> createWindowAsync(WindowProps const& props, Async::CancellationToken ct) = 0;

    virtual Async::Task<> runAsync(Rc<Handler> handler, Async::CancellationToken ct) = 0;
};

} // namespace Karm::App
