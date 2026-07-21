module;

#include <karm/macros>

export module Karm.Gfx.Pixels:formats;

import Karm.Math;
import Karm.Core;
import Karm.Drm;

import :color;

namespace Karm::Gfx {

export struct Rgba8888 {
    always_inline static Color load(void const* pixel) {
        u8 const* p = static_cast<u8 const*>(pixel);
        return Color::fromRgba(p[0], p[1], p[2], p[3]);
    }

    always_inline static void store(void* pixel, Color color) {
        u8* p = static_cast<u8*>(pixel);
        p[0] = color.red;
        p[1] = color.green;
        p[2] = color.blue;
        p[3] = color.alpha;
    }

    always_inline static constexpr usize bpp() {
        return 4;
    }
};

export Rgba8888 RGBA8888;

export struct Bgra8888 {
    always_inline static Color load(void const* pixel) {
        u8 const* p = static_cast<u8 const*>(pixel);
        return Color::fromRgba(p[2], p[1], p[0], p[3]);
    }

    always_inline static void store(void* pixel, Color color) {
        u8* p = static_cast<u8*>(pixel);
        p[0] = color.blue;
        p[1] = color.green;
        p[2] = color.red;
        p[3] = color.alpha;
    }

    always_inline static constexpr usize bpp() {
        return 4;
    }
};

export Bgra8888 BGRA8888;

export struct Greyscale8 {
    always_inline static Color load(void const* pixel) {
        u8 const* p = static_cast<u8 const*>(pixel);
        return Color::fromRgba(p[0], p[0], p[0], 255);
    }

    always_inline static void store(void* pixel, Color color) {
        u8* p = static_cast<u8*>(pixel);
        p[0] = color.red;
    }

    always_inline static constexpr usize bpp() {
        return 1;
    }
};

export Greyscale8 GREYSCALE8;

using _Fmts = Union<Rgba8888, Bgra8888, Greyscale8>;

export struct Format : _Fmts {
    using _Fmts::_Fmts;

    always_inline Color load(void const* pixel) const {
        return visit([&](auto f) {
            return f.load(pixel);
        });
    }

    always_inline void store(void* pixel, Color color) const {
        visit([&](auto f) {
            f.store(pixel, color);
        });
    }

    always_inline constexpr usize bpp() const {
        return visit([&](auto f) {
            return f.bpp();
        });
    }
};

export Format bridge(Drm::Format format) {
    switch (format) {
    case Drm::Format::ARGB8888:
        return BGRA8888;
    case Drm::Format::XRGB8888:
        return BGRA8888;
    case Drm::Format::ABGR8888:
        return RGBA8888;
    case Drm::Format::XBGR8888:
        return RGBA8888;
    default:
        notImplemented();
    }
}

} // namespace Karm::Gfx
