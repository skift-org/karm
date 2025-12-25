module;

#include <SDL3/SDL.h>

export module Karm.App:sdl.keys;

import Karm.Core;
import Karm.App.Base;

namespace Karm::App::Sdl {

static App::Key fromSdlKeycode(SDL_Keycode sdl) {
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

static App::Key fromSdlScancode(SDL_Scancode sdl) {
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

static Flags<App::KeyMod> fromSdlMod(u16 sdl) {
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

App::KeyboardEvent fromSdlKeyboardEvent(SDL_KeyboardEvent const& sdl) {
    App::KeyboardEvent ev{};
    ev.key = fromSdlKeycode(sdl.key);
    ev.code = fromSdlScancode(sdl.scancode);
    ev.mods = fromSdlMod(sdl.mod);
    return ev;
}

} // namespace Karm::App::Sdl