module;

#include <SDL3/SDL.h>
#include <karm-core/macros.h>

module Karm.App;

import Karm.Core;
import Karm.Gfx;
import Karm.Sys;

import :sdl.keys;

namespace Karm::App::_Embed {

struct SdlApplication;

struct SdlWindow : Window {
    SdlApplication& _application;
    SDL_Window* _sdlWindow;
    SDL_Surface* _sdlSurface = nullptr;
    Math::Vec2i _lastMousePos = {};
    bool _closed = false;

    explicit SdlWindow(SdlApplication& application, SDL_Window* sdlWindow)
        : _application(application), _sdlWindow(sdlWindow) {}

    ~SdlWindow();

    WindowId id() override {
        return WindowId{SDL_GetWindowID(_sdlWindow)};
    }

    Math::Recti bound() override {
        int x, y, w, h;
        SDL_GetWindowPosition(_sdlWindow, &x, &y);
        SDL_GetWindowSize(_sdlWindow, &w, &h);
        return {x, y, w, h};
    }

    f64 scaleFactor() override {
        return SDL_GetWindowPixelDensity(_sdlWindow);
    }

    Gfx::MutPixels acquireSurface() override {
        _sdlSurface = SDL_GetWindowSurface(_sdlWindow);
        if (not _sdlSurface)
            panic("Failed to get window surface");
        SDL_LockSurface(_sdlSurface);
        return {
            _sdlSurface->pixels,
            {_sdlSurface->w, _sdlSurface->h},
            (usize)_sdlSurface->pitch,
            Gfx::BGRA8888,
        };
    }

    void releaseSurface() override {
        SDL_UnlockSurface(_sdlSurface);
        SDL_UpdateWindowSurface(_sdlWindow);
    }

    void drag(DragEvent e) override {
        if (e.type == DragEvent::START) {
            SDL_CaptureMouse(true);
        } else if (e.type == DragEvent::END) {
            SDL_CaptureMouse(false);
        } else if (e.type == DragEvent::DRAG) {
            Math::Vec2<i32> pos{};
            SDL_GetWindowPosition(_sdlWindow, &pos.x, &pos.y);
            pos = pos + e.delta.cast<i32>();
            SDL_SetWindowPosition(_sdlWindow, pos.x, pos.y);
        }
    }

    void resize(Direction) override {
    }

    void maximize() override {
        SDL_MaximizeWindow(_sdlWindow);
    }

    void minimize() override {
        SDL_MinimizeWindow(_sdlWindow);
    }

    void close() override {
        _closed = true;
    }
};

struct SdlApplication : Application {
    Map<SDL_WindowID, SdlWindow*> _windows;
    ApplicationProps _props;
    bool _exited = false;
    Math::Vec2i _lastScreenMousePos;

    explicit SdlApplication(ApplicationProps const& props)
        : _props(props) {}

    Async::Task<Rc<Window>> createWindowAsync(WindowProps const& props, Async::CancellationToken) override {
        SDL_Window* sdlWindow = SDL_CreateWindow(
            props.title.buf(),
            props.size.x,
            props.size.y,
            SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE
        );

        SDL_ShowWindow(sdlWindow);
        SDL_UpdateWindowSurface(sdlWindow);
        SDL_StartTextInput(sdlWindow);
        auto window = makeRc<SdlWindow>(*this, sdlWindow);
        _windows.put(SDL_GetWindowID(sdlWindow), &*window);
        co_return Ok(window);
    }

    void detachWindow(SDL_WindowID id) {
        _windows.del(id);
    }

    void _translateEvent(Rc<Handler> handler, SDL_Event const& sdlEvent) {
        switch (sdlEvent.type) {

        case SDL_EVENT_KEY_DOWN: {
            auto ev = Sdl::fromSdlKeyboardEvent(sdlEvent.key);
            ev.type = sdlEvent.key.repeat ? App::KeyboardEvent::REPEATE : App::KeyboardEvent::PRESS;
            handler->handle<App::KeyboardEvent>(WindowId{sdlEvent.key.windowID}, ev);
            break;
        }

        case SDL_EVENT_KEY_UP: {
            auto ev = Sdl::fromSdlKeyboardEvent(sdlEvent.key);
            ev.type = App::KeyboardEvent::RELEASE;
            handler->handle<App::KeyboardEvent>(WindowId{sdlEvent.key.windowID}, ev);
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            Str text = sdlEvent.text.text;
            for (Rune r : iterRunes(text)) {
                handler->handle<App::TypeEvent>(WindowId{sdlEvent.key.windowID}, r);
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.get(sdlEvent.motion.windowID);

            Math::Vec2<f32> screenPos = {};
            SDL_GetGlobalMouseState(&screenPos.x, &screenPos.y);

            Flags<App::MouseButton> buttons;
            buttons.set(App::MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(App::MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(App::MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            window->_lastMousePos = {
                static_cast<isize>(sdlEvent.motion.x),
                static_cast<isize>(sdlEvent.motion.y),
            };

            handler->handle<App::MouseEvent>(
                WindowId{sdlEvent.motion.windowID},
                App::MouseEvent{
                    .type = App::MouseEvent::MOVE,
                    .pos = window->_lastMousePos,
                    .delta = screenPos.cast<isize>() - _lastScreenMousePos,
                    .buttons = buttons,
                }
            );

            _lastScreenMousePos = screenPos.cast<isize>();
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.get(sdlEvent.motion.windowID);

            Flags<App::MouseButton> buttons;
            buttons.set(App::MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(App::MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(App::MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            App::MouseButton button = App::MouseButton::NONE;
            if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
                button = App::MouseButton::LEFT;
            } else if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
                button = App::MouseButton::RIGHT;
            } else if (sdlEvent.button.button == SDL_BUTTON_MIDDLE) {
                button = App::MouseButton::MIDDLE;
            }

            handler->handle<App::MouseEvent>(
                WindowId{sdlEvent.button.windowID},
                App::MouseEvent{
                    .type = App::MouseEvent::RELEASE,
                    .pos = window->_lastMousePos,
                    .buttons = buttons,
                    .button = button,
                }
            );
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.get(sdlEvent.motion.windowID);

            Flags<App::MouseButton> buttons;
            buttons.set(App::MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(App::MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(App::MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            App::MouseButton button = App::MouseButton::NONE;
            if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
                button = App::MouseButton::LEFT;
            } else if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
                button = App::MouseButton::RIGHT;
            } else if (sdlEvent.button.button == SDL_BUTTON_MIDDLE) {
                button = App::MouseButton::MIDDLE;
            }

            handler->handle<App::MouseEvent>(
                WindowId{sdlEvent.button.windowID},

                App::MouseEvent{
                    .type = App::MouseEvent::PRESS,
                    .pos = window->_lastMousePos,
                    .buttons = buttons,
                    .button = button,
                }
            );
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            if (sdlEvent.wheel.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.get(sdlEvent.motion.windowID);

            handler->handle<App::MouseEvent>(
                WindowId{sdlEvent.wheel.windowID},

                App::MouseEvent{
                    .type = App::MouseEvent::SCROLL,
                    .pos = window->_lastMousePos,
                    .scroll = {
                        -sdlEvent.wheel.x,
                        sdlEvent.wheel.y,
                    },
                }
            );

            break;
        }

        case SDL_EVENT_QUIT: {
            _exited = true;
            break;
        }

        default:
            break;
        }
    }

    bool exited() {
        for (auto [id, window] : _windows.iterUnordered())
            if (not window->_closed)
                return _exited;
        return true;
    }

    Async::Task<> runAsync(Rc<Handler> handler, Async::CancellationToken ct) override {
        Duration const frameDuration = Duration::fromMSecs(16);

        Instant nextFrameTime = Sys::instant();

        while (not exited()) {
            nextFrameTime = nextFrameTime + frameDuration;

            SDL_Event sdlEvent;
            while (SDL_PollEvent(&sdlEvent))
                _translateEvent(handler, sdlEvent);

            handler->update();

            co_trya$(Sys::globalSched().sleepAsync(nextFrameTime, ct));

            Instant now = Sys::instant();
            if (now > nextFrameTime + frameDuration) {
                nextFrameTime = now;
            }
        }

        co_return Ok();
    }
};

SdlWindow::~SdlWindow() {
    _application.detachWindow(SDL_GetWindowID(_sdlWindow));
}

Async::Task<Rc<Application>> createAppAsync(Sys::Context&, ApplicationProps const& props, Async::CancellationToken) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    co_return Ok(makeRc<SdlApplication>(props));
}

} // namespace Karm::App::_Embed
