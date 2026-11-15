export module Karm.App:inputs;

import Karm.Core;
import Karm.Math;
import Karm.App.Base;

namespace Karm::App {

export struct Mouse {
    Math::Vec2i position;
    Math::Vec2f scroll;
    Flags<MouseButton> buttons;

    void update(Event& event) {
        if (auto const& [e] = event.is<MouseEvent>()) {
            position = e.pos;
            scroll = e.scroll;
            buttons = e.buttons;
        }
    }
};

export Mouse& globalMouse() {
    static Mouse _mouse;
    return _mouse;
}

export struct Keyboard {
    Array<bool, toUnderlyingType(Key::Code::_LEN)> logicalKeys;
    Array<bool, toUnderlyingType(Key::Code::_LEN)> physicalKeys;

    bool pressed(Key key, KeyboardKey mode = KeyboardKey::LOGICAL) const {
        if (mode == KeyboardKey::LOGICAL)
            return logicalKeys[toUnderlyingType(key.code())];
        return physicalKeys[toUnderlyingType(key.code())];
    }

    bool release(Key key, KeyboardKey mode = KeyboardKey::LOGICAL) const {
        return not pressed(key, mode);
    }

    Flags<KeyMod> modifiers() const {
        Flags<KeyMod> mods;

        return mods;
    }

    void update(Event& event) {
        if (auto const& [e] = event.is<KeyboardEvent>()) {
            if (e.type == KeyboardEvent::PRESS) {
                logicalKeys[toUnderlyingType(e.logicalKey.code())] = true;
                physicalKeys[toUnderlyingType(e.physicalKey.code())] = true;
            } else if (e.type == KeyboardEvent::RELEASE) {
                logicalKeys[toUnderlyingType(e.logicalKey.code())] = false;
                physicalKeys[toUnderlyingType(e.physicalKey.code())] = false;
            }
        }
    }
};

export Keyboard& globalKeyboard() {
    static Keyboard _keyboard;
    return _keyboard;
}

} // namespace Karm::App