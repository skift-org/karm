module;

#include <karm-core/macros.h>

export module Karm.App.Base:inputs;

import Karm.Core;
import Karm.Math;

import :event;

namespace Karm::App {

// MARK: Keyboard --------------------------------------------------------------

export enum struct KeyMod : u16 {
    LSHIFT = 1 << 0,
    RSHIFT = 1 << 1,
    LCTRL = 1 << 2,
    RCTRL = 1 << 3,
    LALT = 1 << 4,
    RALT = 1 << 5,
    LSUPER = 1 << 6,
    RSUPER = 1 << 7,

    NUM = 1 << 8,
    CAPS = 1 << 9,
    MODE = 1 << 10,
    SCROLL = 1 << 11,

    // Either left or right modifier
    SHIFT = 1 << 12,
    CTRL = 1 << 13,
    ALT = 1 << 14,
    SUPER = 1 << 15,
};

export bool match(Flags<KeyMod> in, Flags<KeyMod> mods) {
    Flags<KeyMod> either = {};
    Flags<KeyMod> mask = {};

    // Removing the non-combinable modifiers from the matching
    // Because eg: NumLock + A should be considered exactly like A
    if (static_cast<bool>(in & KeyMod::NUM)) {
        mask |= KeyMod::NUM;
    }
    if (static_cast<bool>(in & KeyMod::CAPS)) {
        mask |= KeyMod::CAPS;
    }
    if (static_cast<bool>(in & KeyMod::MODE)) {
        mask |= KeyMod::MODE;
    }
    if (static_cast<bool>(in & KeyMod::SCROLL)) {
        mask |= KeyMod::SCROLL;
    }

    if (static_cast<bool>(in & KeyMod::LSHIFT) or
        static_cast<bool>(in & KeyMod::RSHIFT)) {
        either |= KeyMod::SHIFT;
        mask |= Flags{KeyMod::LSHIFT, KeyMod::RSHIFT};
    }

    if (static_cast<bool>(in & KeyMod::LCTRL) or
        static_cast<bool>(in & KeyMod::RCTRL)) {
        either |= KeyMod::CTRL;
        mask |= Flags{KeyMod::LCTRL, KeyMod::RCTRL};
    }

    if (static_cast<bool>(in & KeyMod::LALT) or
        static_cast<bool>(in & KeyMod::RALT)) {
        either |= KeyMod::ALT;
        mask |= Flags{KeyMod::LALT, KeyMod::RALT};
    }

    if (static_cast<bool>(in & KeyMod::LSUPER) or
        static_cast<bool>(in & KeyMod::RSUPER)) {
        either |= KeyMod::SUPER;
        mask |= Flags{KeyMod::LSUPER, KeyMod::RSUPER};
    }

    return (((in | either) & mods) == mods) and
           ((in & (mods | mask)) == in);
}

export enum struct KeyMotion {
    RELEASED,
    PRESSED,
};

export struct Key {
    enum struct Code {
#define KEY(name, code) name = code,
#include "../defs/keys.inc"

#undef KEY
        _LEN,
    };

    using enum Code;

    Code _code;

    Key(Code code = Code::INVALID)
        : _code(code) {}

    Str name() const {
        switch (_code) {
#define KEY(name, code) \
    case Code::name:    \
        return #name;
#include "../defs/keys.inc"

        default:
            unreachable();
#undef KEY
        }
        return "INVALID";
    }

    Code code() {
        return _code;
    }

    bool operator==(Key const& other) const = default;

    auto operator<=>(Key const& other) const = default;

    auto operator<=>(Code const& other) const {
        return _code <=> other;
    }

    Opt<KeyMod> toMod() const {
        if (_code == LSHIFT)
            return KeyMod::LSHIFT;
        if (_code == RSHIFT)
            return KeyMod::RSHIFT;
        if (_code == LCTRL)
            return KeyMod::LCTRL;
        if (_code == RCTRL)
            return KeyMod::RCTRL;
        if (_code == LALT)
            return KeyMod::LALT;
        if (_code == RALT)
            return KeyMod::RALT;
        if (_code == LSUPER)
            return KeyMod::LSUPER;
        if (_code == RSUPER)
            return KeyMod::RSUPER;
        return NONE;
    }
};

export struct KeyboardEvent {
    enum {
        PRESS,
        RELEASE,
        REPEATE,

        _LEN,
    } type;

    /// The code of the key that was pressed or released
    /// This value takes the current keyboard layout into account
    Key key;

    /// The scancode of the key that was pressed or released
    /// This value is independent of the current keyboard layout
    Key code;

    Flags<KeyMod> mods = {};
};

export struct TypeEvent {
    Rune rune;
    Flags<KeyMod> mods = {};
};

// MARK: Mouse -----------------------------------------------------------------

export enum struct MouseButton : u8 {
    NONE = 0,

    LEFT = 1 << 0,
    MIDDLE = 1 << 1,
    RIGHT = 1 << 2,
    X1 = 1 << 3,
    X2 = 1 << 4,
};

export struct MouseEvent {
    enum {
        PRESS,
        RELEASE,
        SCROLL,
        MOVE,

        _LEN,
    } type;

    Math::Vec2i pos{};
    Math::Vec2f scroll{};
    Math::Vec2i delta{};
    Flags<MouseButton> buttons{};
    KeyMod mods{};
    MouseButton button{};

    bool pressed(Flags<MouseButton> button) const {
        return buttons & button;
    }

    bool released(MouseButton button) const {
        return not pressed(button);
    }
};

export struct MouseLeaveEvent {
};

export struct MouseEnterEvent {
};

} // namespace Karm::App

template <>
struct Karm::Serde::Serde<Karm::App::Key> {
    static Res<> serialize(Serializer& ser, App::Key const& v) {
        return ser.serialize(v._code);
    }

    static Res<App::Key> deserialize(Deserializer& de) {
        App::Key key{try$(de.deserialize<App::Key::Code>())};
        return Ok(key);
    }
};

template <>
struct Karm::Serde::Serde<Karm::App::KeyMod> {
    static Res<> serialize(Serializer& ser, App::KeyMod const& v) {
        return ser.serialize(toUnderlyingType(v));
    }

    static Res<App::KeyMod> deserialize(Deserializer& de) {
        auto v = try$(de.deserialize<Meta::UnderlyingType<App::KeyMod>>());
        return Ok(static_cast<App::KeyMod>(v));
    }
};

template <>
struct Karm::Serde::Serde<Karm::App::MouseButton> {
    static Res<> serialize(Serializer& ser, App::MouseButton const& v) {
        return ser.serialize(toUnderlyingType(v));
    }

    static Res<App::MouseButton> deserialize(Deserializer& de) {
        auto v = try$(de.deserialize<Meta::UnderlyingType<App::MouseButton>>());
        return Ok(static_cast<App::MouseButton>(v));
    }
};
