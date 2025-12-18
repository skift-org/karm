export module Karm.App:application;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

import :_embed;
import :event;

namespace Karm::App {

export using WindowId = Distinct<usize, struct WindowIdTag>;

export struct WindowProps {
    String title;
    Math::Vec2i size;

    static WindowProps simple() {
        return {
            .title = "Karm Application"s,
            .size = {800, 600},
        };
    }
};

export struct Window : Meta::Pinned {
    virtual ~Window() = default;

    virtual WindowId id() = 0;

    virtual Math::Recti bound() = 0;

    virtual f64 scaleFactor() = 0;

    virtual Gfx::MutPixels acquireSurface() = 0;

    virtual void releaseSurface() = 0;
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

    static Res<Rc<Application>> create(ApplicationProps const& props = ApplicationProps::simple()) {
        return _Embed::createApp(props);
    }

    virtual Res<Rc<Window>> createWindow(WindowProps const& props = WindowProps::simple()) = 0;

    virtual Async::Task<> runAsync(Rc<Handler> handler, Async::CancellationToken ct) = 0;
};

} // namespace Karm::App
