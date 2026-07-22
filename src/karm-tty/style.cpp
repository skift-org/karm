module;

#include <karm/macros>

export module Karm.Tty:style;

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Tty {

export enum Color {
    _COLOR_UNDEF = -1,

    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    MAGENTA = 5,
    CYAN = 6,
    GRAY_LIGHT = 7,

    GRAY_DARK = 60,
    RED_LIGHT = 61,
    GREEN_LIGHT = 62,
    YELLOW_LIGHT = 63,
    BLUE_LIGHT = 64,
    MAGENTA_LIGHT = 65,
    CYAN_LIGHT = 66,
    WHITE = 67,
};

export Color random(usize seed) {
    if (seed & 1)
        return static_cast<Color>((seed >> 1) % 7 + 1);
    return static_cast<Color>((seed >> 1) % 7 + 1 + 60);
}

// clang-format off

export constexpr Array LIGHT_COLORS = {
    GRAY_DARK,RED_LIGHT,GREEN_LIGHT,YELLOW_LIGHT,BLUE_LIGHT,MAGENTA_LIGHT,CYAN_LIGHT,WHITE,
};

export constexpr Array DARK_COLORS = {
    BLACK,RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,GRAY_LIGHT,
};

export constexpr Array COLORS = {
    BLACK,RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,GRAY_LIGHT,
    GRAY_DARK,RED_LIGHT,GREEN_LIGHT,YELLOW_LIGHT,BLUE_LIGHT,MAGENTA_LIGHT,CYAN_LIGHT,WHITE,
};

// clang-format on

export struct Style {
    bool reset{};
    Color foreground{_COLOR_UNDEF};
    Color background{_COLOR_UNDEF};
    bool bold{};
    bool underline{};
    bool blink{};
    bool reverse{};
    bool invisible{};

    void repr([[maybe_unused]] Io::Emit& e) const {
#ifdef __ck_sys_terminal_ansi__
        if (reset) {
            e("\x1b[0m"s);
        }

        if (foreground != Karm::Tty::_COLOR_UNDEF) {
            e("\x1b[{}m", foreground + 30);
        }

        if (background != Karm::Tty::_COLOR_UNDEF) {
            e("\x1b[{}m", background + 40);
        }

        if (bold) {
            e("\x1b[1m"s);
        }

        if (underline) {
            e("\x1b[4m"s);
        }

        if (blink) {
            e("\x1b[5m"s);
        }

        if (reverse) {
            e("\x1b[7m"s);
        }

        if (invisible) {
            e("\x1b[8m"s);
        }
#endif
    }
};

export constexpr Style RESET = {.reset = true};

export template <typename T>
struct Styled {
    T _inner;
    Style _color;
};

export template <typename T>
Styled<T> operator|(T inner, Style style) {
    return Styled<T>{inner, style};
}

export template <typename T>
Styled<T> operator|(T inner, Color color) {
    return Styled<T>{inner, color};
}

} // namespace Karm::Tty

export template <typename T>
struct Karm::Io::Formatter<Karm::Tty::Styled<T>> {
    Formatter<Tty::Style> _styleFmt{};
    Formatter<T> _innerFmt{};

    void parse(SScan& scan) {
        if constexpr (requires() {
                          _innerFmt.parse(scan);
                      }) {
            _innerFmt.parse(scan);
        }
    }

    Res<> format(TextWriter& writer, Tty::Styled<T> const& val) {
#ifdef __ck_sys_terminal_ansi__
        try$(_styleFmt.format(writer, val._color));
        try$(_innerFmt.format(writer, val._inner));
        try$(writer.writeStr("\x1b[0m"s));
        return Ok();
#else
        return _innerFmt.format(writer, val._inner);
#endif
    }
};
