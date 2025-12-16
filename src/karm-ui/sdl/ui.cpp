module;

#include <SDL3/SDL.h>
#include <karm-core/macros.h>

module Karm.Ui;

import Karm.Image;
import Karm.App;
import Karm.Sys;
import Karm.Ref;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

import :host;
import :node;
import :drag;

namespace Karm::Ui::_Embed {

static SDL_HitTestResult _hitTestCallback(SDL_Window* window, SDL_Point const* area, void* data);

struct SdlHost : Host {
    SDL_Window* _window{};

    Math::Vec2i _lastMousePos{};
    Math::Vec2i _lastScreenMousePos{};

    SdlHost(Child root, SDL_Window* window)
        : Host(root), _window(window) {
    }

    ~SdlHost() {
        SDL_DestroyWindow(_window);
    }

    Math::Recti bound() override {
        i32 w, h;
        SDL_GetWindowSize(_window, &w, &h);
        return {w, h};
    }

    f64 dpi() {
        return pixels().width() / (f64)bound().width;
    }

    Gfx::MutPixels mutPixels() override {
        SDL_Surface* s = SDL_GetWindowSurface(_window);
        if (not s)
            panic("Failed to get window surface");
        SDL_LockSurface(s);

        return {
            s->pixels,
            {s->w, s->h},
            (usize)s->pitch,
            Gfx::BGRA8888,
        };
    }

    void flip(Slice<Math::Recti>) override {
        SDL_Surface* s = SDL_GetWindowSurface(_window);
        if (not s)
            panic("Failed to get window surface");
        SDL_UnlockSurface(s);

        SDL_UpdateWindowSurface(_window);
    }

    static App::Key _fromSdlKeycode(SDL_Keycode sdl) {
        // clang-format off
        switch (sdl) {
            case SDLK_A: return App::Key::A;
            case SDLK_B: return App::Key::B;
            case SDLK_C: return App::Key::C;
            case SDLK_D: return App::Key::D;
            case SDLK_E: return App::Key::E;
            case SDLK_F: return App::Key::F;
            case SDLK_G: return App::Key::G;
            case SDLK_H: return App::Key::H;
            case SDLK_I: return App::Key::I;
            case SDLK_J: return App::Key::J;
            case SDLK_K: return App::Key::K;
            case SDLK_L: return App::Key::L;
            case SDLK_M: return App::Key::M;
            case SDLK_N: return App::Key::N;
            case SDLK_O: return App::Key::O;
            case SDLK_P: return App::Key::P;
            case SDLK_Q: return App::Key::Q;
            case SDLK_R: return App::Key::R;
            case SDLK_S: return App::Key::S;
            case SDLK_T: return App::Key::T;
            case SDLK_U: return App::Key::U;
            case SDLK_V: return App::Key::V;
            case SDLK_W: return App::Key::W;
            case SDLK_X: return App::Key::X;
            case SDLK_Y: return App::Key::Y;
            case SDLK_Z: return App::Key::Z;

            case SDLK_1: return App::Key::NUM1;
            case SDLK_2: return App::Key::NUM2;
            case SDLK_3: return App::Key::NUM3;
            case SDLK_4: return App::Key::NUM4;
            case SDLK_5: return App::Key::NUM5;
            case SDLK_6: return App::Key::NUM6;
            case SDLK_7: return App::Key::NUM7;
            case SDLK_8: return App::Key::NUM8;
            case SDLK_9: return App::Key::NUM9;
            case SDLK_0: return App::Key::NUM0;

            case SDLK_RETURN: return App::Key::ENTER;
            case SDLK_ESCAPE: return App::Key::ESC;
            case SDLK_BACKSPACE: return App::Key::BKSPC;
            case SDLK_TAB: return App::Key::TAB;
            case SDLK_SPACE: return App::Key::SPACE;
            case SDLK_CAPSLOCK: return App::Key::CAPSLOCK;

            case SDLK_F1: return App::Key::F1;
            case SDLK_F2: return App::Key::F2;
            case SDLK_F3: return App::Key::F3;
            case SDLK_F4: return App::Key::F4;
            case SDLK_F5: return App::Key::F5;
            case SDLK_F6: return App::Key::F6;
            case SDLK_F7: return App::Key::F7;
            case SDLK_F8: return App::Key::F8;
            case SDLK_F9: return App::Key::F9;
            case SDLK_F10: return App::Key::F10;
            case SDLK_F11: return App::Key::F11;
            case SDLK_F12: return App::Key::F12;

            case SDLK_INSERT: return App::Key::INSERT;
            case SDLK_HOME: return App::Key::HOME;
            case SDLK_PAGEUP: return App::Key::PGUP;
            case SDLK_DELETE: return App::Key::DELETE;
            case SDLK_END: return App::Key::END;
            case SDLK_PAGEDOWN: return App::Key::PGDOWN;
            case SDLK_RIGHT: return App::Key::RIGHT;
            case SDLK_LEFT: return App::Key::LEFT;
            case SDLK_DOWN: return App::Key::DOWN;
            case SDLK_UP: return App::Key::UP;

            case SDLK_KP_ENTER: return App::Key::KPADENTER;
            case SDLK_KP_1: return App::Key::KPAD1;
            case SDLK_KP_2: return App::Key::KPAD2;
            case SDLK_KP_3: return App::Key::KPAD3;
            case SDLK_KP_4: return App::Key::KPAD4;
            case SDLK_KP_5: return App::Key::KPAD5;
            case SDLK_KP_6: return App::Key::KPAD6;
            case SDLK_KP_7: return App::Key::KPAD7;
            case SDLK_KP_8: return App::Key::KPAD8;
            case SDLK_KP_9: return App::Key::KPAD9;
            case SDLK_KP_0: return App::Key::KPAD0;

            case SDLK_MENU: return App::Key::MENU;

            case SDLK_LCTRL: return App::Key::LCTRL;
            case SDLK_LSHIFT: return App::Key::LSHIFT;
            case SDLK_LALT: return App::Key::LALT;
            case SDLK_LGUI: return App::Key::LSUPER;

            case SDLK_RCTRL: return App::Key::RCTRL;
            case SDLK_RSHIFT: return App::Key::RSHIFT;
            case SDLK_RALT: return App::Key::RALT;
            case SDLK_RGUI: return App::Key::RSUPER;

            default: return App::Key::INVALID;
        }
        // clang-format on
    }

    static App::Key _fromSdlScancode(SDL_Scancode sdl) {
        // clang-format off
        switch (sdl) {
            case SDL_SCANCODE_A: return App::Key::A;
            case SDL_SCANCODE_B: return App::Key::B;
            case SDL_SCANCODE_C: return App::Key::C;
            case SDL_SCANCODE_D: return App::Key::D;
            case SDL_SCANCODE_E: return App::Key::E;
            case SDL_SCANCODE_F: return App::Key::F;
            case SDL_SCANCODE_G: return App::Key::G;
            case SDL_SCANCODE_H: return App::Key::H;
            case SDL_SCANCODE_I: return App::Key::I;
            case SDL_SCANCODE_J: return App::Key::J;
            case SDL_SCANCODE_K: return App::Key::K;
            case SDL_SCANCODE_L: return App::Key::L;
            case SDL_SCANCODE_M: return App::Key::M;
            case SDL_SCANCODE_N: return App::Key::N;
            case SDL_SCANCODE_O: return App::Key::O;
            case SDL_SCANCODE_P: return App::Key::P;
            case SDL_SCANCODE_Q: return App::Key::Q;
            case SDL_SCANCODE_R: return App::Key::R;
            case SDL_SCANCODE_S: return App::Key::S;
            case SDL_SCANCODE_T: return App::Key::T;
            case SDL_SCANCODE_U: return App::Key::U;
            case SDL_SCANCODE_V: return App::Key::V;
            case SDL_SCANCODE_W: return App::Key::W;
            case SDL_SCANCODE_X: return App::Key::X;
            case SDL_SCANCODE_Y: return App::Key::Y;
            case SDL_SCANCODE_Z: return App::Key::Z;

            case SDL_SCANCODE_1: return App::Key::NUM1;
            case SDL_SCANCODE_2: return App::Key::NUM2;
            case SDL_SCANCODE_3: return App::Key::NUM3;
            case SDL_SCANCODE_4: return App::Key::NUM4;
            case SDL_SCANCODE_5: return App::Key::NUM5;
            case SDL_SCANCODE_6: return App::Key::NUM6;
            case SDL_SCANCODE_7: return App::Key::NUM7;
            case SDL_SCANCODE_8: return App::Key::NUM8;
            case SDL_SCANCODE_9: return App::Key::NUM9;
            case SDL_SCANCODE_0: return App::Key::NUM0;

            case SDL_SCANCODE_RETURN: return App::Key::ENTER;
            case SDL_SCANCODE_ESCAPE: return App::Key::ESC;
            case SDL_SCANCODE_BACKSPACE: return App::Key::BKSPC;
            case SDL_SCANCODE_TAB: return App::Key::TAB;
            case SDL_SCANCODE_SPACE: return App::Key::SPACE;
            case SDL_SCANCODE_CAPSLOCK: return App::Key::CAPSLOCK;

            case SDL_SCANCODE_F1: return App::Key::F1;
            case SDL_SCANCODE_F2: return App::Key::F2;
            case SDL_SCANCODE_F3: return App::Key::F3;
            case SDL_SCANCODE_F4: return App::Key::F4;
            case SDL_SCANCODE_F5: return App::Key::F5;
            case SDL_SCANCODE_F6: return App::Key::F6;
            case SDL_SCANCODE_F7: return App::Key::F7;
            case SDL_SCANCODE_F8: return App::Key::F8;
            case SDL_SCANCODE_F9: return App::Key::F9;
            case SDL_SCANCODE_F10: return App::Key::F10;
            case SDL_SCANCODE_F11: return App::Key::F11;
            case SDL_SCANCODE_F12: return App::Key::F12;

            case SDL_SCANCODE_INSERT: return App::Key::INSERT;
            case SDL_SCANCODE_HOME: return App::Key::HOME;
            case SDL_SCANCODE_PAGEUP: return App::Key::PGUP;
            case SDL_SCANCODE_DELETE: return App::Key::DELETE;
            case SDL_SCANCODE_END: return App::Key::END;
            case SDL_SCANCODE_PAGEDOWN: return App::Key::PGDOWN;
            case SDL_SCANCODE_RIGHT: return App::Key::RIGHT;
            case SDL_SCANCODE_LEFT: return App::Key::LEFT;
            case SDL_SCANCODE_DOWN: return App::Key::DOWN;
            case SDL_SCANCODE_UP: return App::Key::UP;

            case SDL_SCANCODE_KP_ENTER: return App::Key::KPADENTER;
            case SDL_SCANCODE_KP_1: return App::Key::KPAD1;
            case SDL_SCANCODE_KP_2: return App::Key::KPAD2;
            case SDL_SCANCODE_KP_3: return App::Key::KPAD3;
            case SDL_SCANCODE_KP_4: return App::Key::KPAD4;
            case SDL_SCANCODE_KP_5: return App::Key::KPAD5;
            case SDL_SCANCODE_KP_6: return App::Key::KPAD6;
            case SDL_SCANCODE_KP_7: return App::Key::KPAD7;
            case SDL_SCANCODE_KP_8: return App::Key::KPAD8;
            case SDL_SCANCODE_KP_9: return App::Key::KPAD9;
            case SDL_SCANCODE_KP_0: return App::Key::KPAD0;

            case SDL_SCANCODE_MENU: return App::Key::MENU;

            case SDL_SCANCODE_LCTRL: return App::Key::LCTRL;
            case SDL_SCANCODE_LSHIFT: return App::Key::LSHIFT;
            case SDL_SCANCODE_LALT: return App::Key::LALT;
            case SDL_SCANCODE_LGUI: return App::Key::LSUPER;

            case SDL_SCANCODE_RCTRL: return App::Key::RCTRL;
            case SDL_SCANCODE_RSHIFT: return App::Key::RSHIFT;
            case SDL_SCANCODE_RALT: return App::Key::RALT;
            case SDL_SCANCODE_RGUI: return App::Key::RSUPER;

            default: return App::Key::INVALID;
        }
        // clang-format on
    }

    static Flags<App::KeyMod> _fromSdlMod(u16 sdl) {
        Flags<App::KeyMod> mods;
        ;

        if (sdl & SDL_KMOD_LSHIFT)
            mods |= App::KeyMod::LSHIFT;

        if (sdl & SDL_KMOD_RSHIFT)
            mods |= App::KeyMod::RSHIFT;

        if (sdl & SDL_KMOD_LCTRL)
            mods |= App::KeyMod::LCTRL;

        if (sdl & SDL_KMOD_RCTRL)
            mods |= App::KeyMod::RCTRL;

        if (sdl & SDL_KMOD_LALT)
            mods |= App::KeyMod::LALT;

        if (sdl & SDL_KMOD_RALT)
            mods |= App::KeyMod::RALT;

        if (sdl & SDL_KMOD_LGUI)
            mods |= App::KeyMod::LSUPER;

        if (sdl & SDL_KMOD_RGUI)
            mods |= App::KeyMod::RSUPER;

        if (sdl & SDL_KMOD_NUM)
            mods |= App::KeyMod::NUM;

        if (sdl & SDL_KMOD_CAPS)
            mods |= App::KeyMod::CAPS;

        if (sdl & SDL_KMOD_MODE)
            mods |= App::KeyMod::MODE;

        if (sdl & SDL_KMOD_SCROLL)
            mods |= App::KeyMod::SCROLL;

        return mods;
    }

    static App::KeyboardEvent _fromSdlKeyboardEvent(SDL_KeyboardEvent const& sdl) {
        App::KeyboardEvent ev{};
        ev.key = _fromSdlKeycode(sdl.key);
        ev.code = _fromSdlScancode(sdl.scancode);
        ev.mods = _fromSdlMod(sdl.mod);
        return ev;
    }

    SDL_Cursor* _cursor{};
    SDL_SystemCursor _systemCursor{};

    void translate(SDL_Event const& sdlEvent) {
        switch (sdlEvent.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            _shouldLayout = true;
            break;
        case SDL_EVENT_WINDOW_EXPOSED:
            _dirty.pushBack(pixels().bound());
            break;

        case SDL_EVENT_KEY_DOWN: {
            auto ev = _fromSdlKeyboardEvent(sdlEvent.key);
            ev.type = sdlEvent.key.repeat ? App::KeyboardEvent::REPEATE : App::KeyboardEvent::PRESS;
            event<App::KeyboardEvent>(*this, ev);
            break;
        }

        case SDL_EVENT_KEY_UP: {
            auto ev = _fromSdlKeyboardEvent(sdlEvent.key);
            ev.type = App::KeyboardEvent::RELEASE;
            event<App::KeyboardEvent>(*this, ev);
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            Str text = sdlEvent.text.text;
            for (Rune r : iterRunes(text)) {
                event<App::TypeEvent>(*this, r);
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID)
                return;

            Math::Vec2<f32> screenPos = {};
            SDL_GetGlobalMouseState(&screenPos.x, &screenPos.y);

            Flags<App::MouseButton> buttons;
            buttons.set(App::MouseButton::LEFT, sdlEvent.motion.state & SDL_BUTTON_LMASK);
            buttons.set(App::MouseButton::MIDDLE, sdlEvent.motion.state & SDL_BUTTON_MMASK);
            buttons.set(App::MouseButton::RIGHT, sdlEvent.motion.state & SDL_BUTTON_RMASK);

            // do the hit test and update the cursor

            SDL_Point p = {static_cast<int>(sdlEvent.motion.x), static_cast<int>(sdlEvent.motion.y)};
            SDL_HitTestResult result = _hitTestCallback(_window, &p, this);
            SDL_SystemCursor systemCursor = SDL_SYSTEM_CURSOR_DEFAULT;

            switch (result) {
            case SDL_HITTEST_RESIZE_TOPLEFT:
            case SDL_HITTEST_RESIZE_BOTTOMRIGHT:
                systemCursor = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
                break;
            case SDL_HITTEST_RESIZE_TOPRIGHT:
            case SDL_HITTEST_RESIZE_BOTTOMLEFT:
                systemCursor = SDL_SYSTEM_CURSOR_NESW_RESIZE;
                break;
            case SDL_HITTEST_RESIZE_TOP:
            case SDL_HITTEST_RESIZE_BOTTOM:
                systemCursor = SDL_SYSTEM_CURSOR_NS_RESIZE;
                break;
            case SDL_HITTEST_RESIZE_LEFT:
            case SDL_HITTEST_RESIZE_RIGHT:
                systemCursor = SDL_SYSTEM_CURSOR_EW_RESIZE;
                break;
            default:
                break;
            }

            if (_systemCursor != systemCursor) {
                if (_cursor) {
                    SDL_DestroyCursor(_cursor);
                }

                _cursor = SDL_CreateSystemCursor(systemCursor);
                SDL_SetCursor(_cursor);
                _systemCursor = systemCursor;
            }

            _lastMousePos = {
                static_cast<isize>(sdlEvent.motion.x),
                static_cast<isize>(sdlEvent.motion.y),
            };

            event<App::MouseEvent>(
                *this,
                App::MouseEvent{
                    .type = App::MouseEvent::MOVE,
                    .pos = _lastMousePos,
                    .delta = screenPos.cast<isize>() - _lastScreenMousePos,
                    .buttons = buttons,
                }
            );

            _lastScreenMousePos = screenPos.cast<isize>();
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID) {
                return;
            }

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

            event<App::MouseEvent>(
                *this,
                App::MouseEvent{
                    .type = App::MouseEvent::RELEASE,
                    .pos = _lastMousePos,
                    .buttons = buttons,
                    .button = button,
                }
            );
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (sdlEvent.motion.which == SDL_TOUCH_MOUSEID) {
                return;
            }

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

            event<App::MouseEvent>(
                *this,
                App::MouseEvent{
                    .type = App::MouseEvent::PRESS,
                    .pos = _lastMousePos,
                    .buttons = buttons,
                    .button = button,
                }
            );
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            if (sdlEvent.wheel.which == SDL_TOUCH_MOUSEID)
                return;

            event<App::MouseEvent>(
                *this,
                App::MouseEvent{
                    .type = App::MouseEvent::SCROLL,
                    .pos = _lastMousePos,
                    .scroll = {
                        -sdlEvent.wheel.x,
                        sdlEvent.wheel.y,
                    },
                }
            );

            break;
        }

        case SDL_EVENT_QUIT: {
            bubble<App::RequestExitEvent>(*this);
            break;
        }

        default:
            break;
        }
    }

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        g.push();
        g.scale(dpi());
        Host::paint(g, r);
        g.pop();
    }

    Async::Task<> waitAsync(Instant ts, Async::CancellationToken ct) override {
        // HACK: Since we don't have a lot of control onto how SDL wait for
        //       events we can't integrate it properly with our event loop
        //       To remedy this we will just cap how long we wait, this way
        //       we can poll for event.

        // NOTE: A better option would be to have SDL in a separated thread
        //       and do the communication over an inter-thread channel but
        //       but this would require to make the Framework thread safe
        auto delay = Duration::fromMSecs((usize)(FRAME_TIME * 1000));
        auto cappedWait = min(ts, Sys::instant() + delay);
        co_trya$(Sys::globalSched().sleepAsync(cappedWait, ct));

        SDL_Event e{};
        while (SDL_PollEvent(&e) != 0 and alive())
            translate(e);
        co_return Ok();
    }

    void bubble(App::Event& event) override {
        if (auto e = event.is<DragEvent>()) {
            if (e->type == DragEvent::START) {
                SDL_CaptureMouse(true);
            } else if (e->type == DragEvent::END) {
                SDL_CaptureMouse(false);
            } else if (e->type == DragEvent::DRAG) {
                Math::Vec2<i32> pos{};
                SDL_GetWindowPosition(_window, &pos.x, &pos.y);
                pos = pos + e->delta.cast<i32>();
                SDL_SetWindowPosition(_window, pos.x, pos.y);
            }
            event.accept();
        } else if (event.is<App::RequestMinimizeEvent>()) {
            SDL_MinimizeWindow(_window);
            event.accept();
        } else if (event.is<App::RequestMaximizeEvent>()) {
            if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MAXIMIZED)
                SDL_RestoreWindow(_window);
            else
                SDL_MaximizeWindow(_window);
            event.accept();
        }

        Host::bubble(event);
    }
};

static SDL_HitTestResult _hitTestCallback(SDL_Window* window, SDL_Point const* area, void* data) {
    SdlHost* host = (SdlHost*)data;
    isize grabPadding = 6 * host->dpi();
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    if (area->y < grabPadding) {
        if (area->x < grabPadding)
            return SDL_HITTEST_RESIZE_TOPLEFT;
        else if (area->x > width - grabPadding)
            return SDL_HITTEST_RESIZE_TOPRIGHT;

        return SDL_HITTEST_RESIZE_TOP;
    } else if (area->y > height - grabPadding) {
        if (area->x < grabPadding)
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        else if (area->x > width - grabPadding)
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

        return SDL_HITTEST_RESIZE_BOTTOM;
    } else if (area->x < grabPadding) {
        return SDL_HITTEST_RESIZE_LEFT;
    } else if (area->x > width - grabPadding) {
        return SDL_HITTEST_RESIZE_RIGHT;
    }

    return SDL_HITTEST_NORMAL;
}

static Res<> _setWindowIcon(SDL_Window* window) {
    auto url = try$(Sys::Bundle::current()).url() / "images/icon.qoi"_path;
    auto defaultUrl = "bundle://karm-ui/images/icon.qoi"_url;
    auto image = Karm::Image::load(url).unwrapOrElse([&] {
        return Karm::Image::loadOrFallback(defaultUrl).take();
    });

    auto* surface = SDL_CreateSurface(image->width(), image->height(), SDL_PIXELFORMAT_BGRA32);
    if (not surface)
        return Error::other(SDL_GetError());
    Defer _{[&] {
        SDL_DestroySurface(surface);
    }};

    Gfx::MutPixels pixels{
        surface->pixels,
        {surface->w, surface->h},
        (usize)surface->pitch,
        Gfx::BGRA8888,
    };
    blitUnsafe(pixels, image->pixels());

    SDL_SetWindowIcon(window, surface);

    return Ok();
}

static Res<Rc<Host>> makeHost(Child root) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    auto size = root->size({1024, 720}, Hint::MIN);

    SDL_Window* window = SDL_CreateWindow(
        "Application",
        size.width,
        size.height,
        SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS
    );

    if (not window)
        return Error::other(SDL_GetError());

    SDL_StartTextInput(window);

    try$(_setWindowIcon(window));
    auto host = makeRc<SdlHost>(root, window);

    SDL_SetWindowHitTest(window, _hitTestCallback, (void*)&host.unwrap());

    return Ok(host);
}

Async::Task<> runAsync(Sys::Context&, Child root, Async::CancellationToken ct) {
    auto host = co_try$(makeHost(std::move(root)));
    co_return co_await host->runAsync(ct);
}

} // namespace Karm::Ui::_Embed
