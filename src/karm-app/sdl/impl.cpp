module;

#include <SDL3/SDL.h>
#include <karm/macros>

module Karm.App;

import Karm.Core;
import Karm.Gfx;
import Karm.Sys;

import :sdl.keys;

namespace Karm::App::_Embed {

static Flags<KeyMod> currentMods() {
    auto sdl = SDL_GetModState();
    Flags<KeyMod> mods;

    if (sdl & SDL_KMOD_LSHIFT)
        mods |= KeyMod::LSHIFT;
    if (sdl & SDL_KMOD_RSHIFT)
        mods |= KeyMod::RSHIFT;
    if (sdl & SDL_KMOD_LCTRL)
        mods |= KeyMod::LCTRL;
    if (sdl & SDL_KMOD_RCTRL)
        mods |= KeyMod::RCTRL;
    if (sdl & SDL_KMOD_LALT)
        mods |= KeyMod::LALT;
    if (sdl & SDL_KMOD_RALT)
        mods |= KeyMod::RALT;
    if (sdl & SDL_KMOD_LGUI)
        mods |= KeyMod::LSUPER;
    if (sdl & SDL_KMOD_RGUI)
        mods |= KeyMod::RSUPER;
    if (sdl & SDL_KMOD_NUM)
        mods |= KeyMod::NUM;
    if (sdl & SDL_KMOD_CAPS)
        mods |= KeyMod::CAPS;
    if (sdl & SDL_KMOD_MODE)
        mods |= KeyMod::MODE;
    if (sdl & SDL_KMOD_SCROLL)
        mods |= KeyMod::SCROLL;

    return mods;
}

struct SdlApplication;

struct SdlWindow : Window {
    SdlApplication& _application;
    SDL_Window* _sdlWindow;
    SDL_Surface* _sdlSurface = nullptr;
    Math::Vec2i _lastMousePos = {};
    bool _closed = false;

    explicit SdlWindow(SdlApplication& application, SDL_Window* sdlWindow)
        : _application(application), _sdlWindow(sdlWindow) {
    }

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

    void releaseSurface(Slice<Math::Recti>) override {
        SDL_UpdateWindowSurface(_sdlWindow);
        SDL_UnlockSurface(_sdlSurface);
        _sdlSurface = nullptr;
    }

    void drag() override {
        // FIXME
    }

    void snap(Snap snap) override {
        if (snap == Snap::NONE) {
            SDL_RestoreWindow(_sdlWindow);
        } else {
            SDL_MaximizeWindow(_sdlWindow);
        }
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
            SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS
        );

        SDL_ShowWindow(sdlWindow);
        SDL_UpdateWindowSurface(sdlWindow);
        SDL_StartTextInput(sdlWindow);
        auto window = makeRc<SdlWindow>(*this, sdlWindow);
        _windows.put(SDL_GetWindowID(sdlWindow), &*window);
        co_return Ok(window);
    }

    void detachWindow(SDL_WindowID id) {
        _windows.remove(id).unwrap("detaching invalid window id");
    }

    void _translateEvent(Rc<Handler> handler, SDL_Event const& sdlEvent) {
        switch (sdlEvent.type) {

        case SDL_EVENT_WINDOW_RESIZED: {
            handler->handle<ResizeEvent>(
                WindowId{sdlEvent.window.windowID},
                ResizeEvent{
                    .size = {
                        sdlEvent.window.data1,
                        sdlEvent.window.data2,
                    },
                }
            );
            break;
        }

        case SDL_EVENT_KEY_DOWN: {
            auto ev = Sdl::fromSdlKeyboardEvent(sdlEvent.key);
            ev.type = sdlEvent.key.repeat ? KeyboardEvent::REPEATE : KeyboardEvent::PRESS;
            handler->handle<KeyboardEvent>(WindowId{sdlEvent.key.windowID}, ev);
            break;
        }

        case SDL_EVENT_KEY_UP: {
            auto ev = Sdl::fromSdlKeyboardEvent(sdlEvent.key);
            ev.type = KeyboardEvent::RELEASE;
            handler->handle<KeyboardEvent>(WindowId{sdlEvent.key.windowID}, ev);
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            KeyboardEvent ev;
            ev.type = KeyboardEvent::PRESS;
            Str text = sdlEvent.text.text;
            for (Rune r : iterRunes(text)) {
                ev.rune = r;
                handler->handle<KeyboardEvent>(WindowId{sdlEvent.key.windowID}, ev);
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;
            auto window = _windows.lookup(sdlEvent.motion.windowID).unwrapOr(nullptr);

            Math::Vec2<f32> screenPos = {};
            SDL_GetGlobalMouseState(&screenPos.x, &screenPos.y);

            Flags<MouseButton> buttons;
            buttons.set(MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            window->_lastMousePos = {
                static_cast<isize>(sdlEvent.motion.x),
                static_cast<isize>(sdlEvent.motion.y),
            };

            handler->handle<MouseEvent>(
                WindowId{sdlEvent.motion.windowID},
                MouseEvent{
                    .type = MouseEvent::MOVE,
                    .pos = window->_lastMousePos,
                    .delta = screenPos.cast<isize>() - _lastScreenMousePos,
                    .buttons = buttons,
                    .mods = currentMods(),
                }
            );

            _lastScreenMousePos = screenPos.cast<isize>();
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.lookup(sdlEvent.motion.windowID).unwrapOr(nullptr);

            Flags<MouseButton> buttons;
            buttons.set(MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            MouseButton button = MouseButton::NONE;
            if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
                button = MouseButton::LEFT;
            } else if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
                button = MouseButton::RIGHT;
            } else if (sdlEvent.button.button == SDL_BUTTON_MIDDLE) {
                button = MouseButton::MIDDLE;
            }

            handler->handle<MouseEvent>(
                WindowId{sdlEvent.button.windowID},
                MouseEvent{
                    .type = MouseEvent::RELEASE,
                    .pos = window->_lastMousePos,
                    .buttons = buttons,
                    .mods = currentMods(),
                    .button = button,
                }
            );
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.lookup(sdlEvent.motion.windowID).unwrapOr(nullptr);
            if (not window)
                return;

            Flags<MouseButton> buttons;
            buttons.set(MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            MouseButton button = MouseButton::NONE;
            if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
                button = MouseButton::LEFT;
            } else if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
                button = MouseButton::RIGHT;
            } else if (sdlEvent.button.button == SDL_BUTTON_MIDDLE) {
                button = MouseButton::MIDDLE;
            }

            handler->handle<MouseEvent>(
                WindowId{sdlEvent.button.windowID},

                MouseEvent{
                    .type = MouseEvent::PRESS,
                    .pos = window->_lastMousePos,
                    .buttons = buttons,
                    .mods = currentMods(),
                    .button = button,
                }
            );
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            if (sdlEvent.wheel.which == SDL_TOUCH_MOUSEID)
                return;
            auto* window = _windows.lookup(sdlEvent.motion.windowID).unwrapOr(nullptr);
            if (not window)
                return;

            handler->handle<MouseEvent>(
                WindowId{sdlEvent.wheel.windowID},

                MouseEvent{
                    .type = MouseEvent::SCROLL,
                    .pos = window->_lastMousePos,
                    .scroll = {
                        -sdlEvent.wheel.x,
                        sdlEvent.wheel.y,
                    },
                    .mods = currentMods(),
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
        for (auto const& [id, window] : _windows.iterItems())
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
