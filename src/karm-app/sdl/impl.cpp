module;

#include <SDL3/SDL.h>
#include <karm/macros>

module Karm.App;

import Karm.Core;
import Karm.Sys;
import :sdl.bridge;

namespace Karm::App::_Embed {

struct SdlApplication;

struct SdlBuffer : Drm::Buffer {
    u8* _pixel;

    SdlBuffer(u8* pixels, Drm::Format format, Math::Vec2u size, usize stride)
        : Buffer(format, size, stride), _pixel(pixels) {}

    Bytes bytes() const override {
        return {
            static_cast<u8 const*>(_pixel),
            stride * size.height,
        };
    }

    MutBytes mutBytes() const override {
        return {
            static_cast<u8*>(_pixel),
            stride * size.height,
        };
    }
};

struct SdlSwapChain : SwapChain {
    SDL_Window* _window;
    SDL_Surface* _surface = nullptr;

    SdlSwapChain(SDL_Window* window)
        : _window(window) {}

    AcquiredBuffer acquire() override {
        _surface = SDL_GetWindowSurface(_window);
        if (not _surface)
            panic("Failed to get window surface");
        SDL_LockSurface(_surface);
        return {
            makeRc<SdlBuffer>(
                static_cast<u8*>(_surface->pixels),
                Sdl::bridge(_surface->format),
                Math::Vec2i{_surface->w, _surface->h}.cast<usize>(),
                static_cast<usize>(_surface->pitch)
            ),
            0
        };
    }

    void present(Rc<Drm::Buffer>, Slice<Math::Recti>) override {
        SDL_UpdateWindowSurface(_window);
        SDL_UnlockSurface(_surface);
        _surface = nullptr;
    }
};

struct SdlWindow : Window {
    SdlApplication& _application;
    SDL_Window* _sdlWindow;
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

    Res<Rc<SwapChain>> createSwapChain(SwapChainProps const&) override {
        return Ok(makeRc<SdlSwapChain>(_sdlWindow));
    }

    void drag() override {
        // NOTE: This is a no-op because SDL lacks a  programmatic way of initiating a window drag.
    }

    void resize(Direction) override {
        // NOTE: This is a no-op because SDL lacks a programmatic way of initiating a window resize,
        //       the host window manager already provides this affordance.
    }

    void cursor(CursorStyle style) override {
        static Array<SDL_Cursor*, (usize)CursorStyle::_LEN> cursors = {};
        auto& cursor = cursors[(usize)style];
        if (not cursor)
            cursor = SDL_CreateSystemCursor(Sdl::bridge(style));
        SDL_SetCursor(cursor);
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
    Opt<Rc<Handler>> _handler = NONE;
    Map<SDL_WindowID, SdlWindow*> _windows;
    ApplicationProps _props;
    bool _exited = false;
    Math::Vec2i _lastScreenMousePos;

    explicit SdlApplication(ApplicationProps const& props)
        : _props(props) {}

    static SDL_HitTestResult _windowHitTest(SDL_Window* win, SDL_Point const* area, void* data) {
        int w, h;
        SDL_GetWindowSize(win, &w, &h);

        int const margin = 6;
        bool isTop = area->y < margin;
        bool isBottom = area->y >= (h - margin);
        bool isLeft = area->x < margin;
        bool isRight = area->x >= (w - margin);

        if (isTop && isLeft)
            return SDL_HITTEST_RESIZE_TOPLEFT;
        if (isTop && isRight)
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        if (isBottom && isLeft)
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        if (isBottom && isRight)
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

        if (isTop)
            return SDL_HITTEST_RESIZE_TOP;
        if (isBottom)
            return SDL_HITTEST_RESIZE_BOTTOM;
        if (isLeft)
            return SDL_HITTEST_RESIZE_LEFT;
        if (isRight)
            return SDL_HITTEST_RESIZE_RIGHT;

        auto app = (SdlApplication*)data;
        if (auto& [handler] = app->_handler) {
            return Sdl::bridge(handler->hitTest(
                WindowId{SDL_GetWindowID(win)},
                {area->x, area->y}
            ));
        }
        return SDL_HITTEST_NORMAL;
    }

    Async::Task<Rc<Window>> createWindowAsync(WindowProps const& props, Async::CancellationToken) override {
        SDL_Window* sdlWindow = SDL_CreateWindow(
            props.title.buf(),
            props.size.x,
            props.size.y,
            SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS
        );

        SDL_SetWindowHitTest(sdlWindow, _windowHitTest, this);
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
                    .mods = Sdl::bridgeSdlKeymod(SDL_GetModState()),
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
                    .mods = Sdl::bridgeSdlKeymod(SDL_GetModState()),
                    .button = button,
                    .clicks = sdlEvent.button.clicks,
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
                    .mods = Sdl::bridgeSdlKeymod(SDL_GetModState()),
                    .button = button,
                    .clicks = sdlEvent.button.clicks,
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
                    .mods = Sdl::bridgeSdlKeymod(SDL_GetModState()),
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
        _handler = handler;
        Defer _ = [&] {
            _handler = NONE;
        };

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

Async::Task<Rc<Application>> createAppAsync(Sys::Env&, ApplicationProps const& props, Async::CancellationToken) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    co_return Ok(makeRc<SdlApplication>(props));
}

} // namespace Karm::App::_Embed
