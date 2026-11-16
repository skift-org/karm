module;

#include <karm/macros>

export module Karm.Gfx:fill;

import Karm.Core;
import Karm.Math;
import Karm.Gfx.Pixels;

namespace Karm::Gfx {

export enum struct FillRule : u8 {
    NONZERO,
    EVENODD,
};

export enum struct GradientType {
    LINEAR,
    RADIAL,
    CONICAL,
    DIAMOND,
};

export using BakedGradientStops = Array<Color, 256>;

export struct GradientStop {
    Color color;
    f64 pos;

    Color lerp(GradientStop rhs, f64 at) const {
        return color.lerpWith(rhs.color, (at - pos) / (rhs.pos - at));
    }

    static constexpr BakedGradientStops bake(Slice<GradientStop> stops, bool wrapAround) {
        BakedGradientStops baked;
        fill(mutSub(baked), BLACK);

        if (stops.len() == 0)
            return baked;

        if (stops.len() == 1) {
            fill(mutSub(baked), stops[0].color);
            return baked;
        }

        for (float j = 0; j < stops[0].pos * 256; j++) {
            if (wrapAround) {
                auto lhs = stops[stops.len() - 1];
                lhs.pos -= 1;
                baked[j] = lhs.lerp(stops[0], j / 256.0);
            } else {
                baked[j] = stops[0].color;
            }
        }

        for (usize i = 0; i < stops.len() - 1; i++) {
            auto iPos = stops[i].pos;
            auto jPos = stops[i + 1].pos;

            for (f64 j = iPos * 256; j < jPos * 256; j++)
                baked[j] = stops[i].lerp(stops[i + 1], j / 256.0);
        }

        for (f64 j = last(stops).pos * 256; j < 256; j++) {
            if (wrapAround) {
                auto rhs = stops[0];
                rhs.pos += 1;
                baked[j] = last(stops).lerp(rhs, j / 256.0);
            } else {
                baked[j] = last(stops).color;
            }
        }

        return baked;
    }

    void repr(Io::Emit& e) const {
        e("(gradient-stop {} {})", color, pos);
    }
};

export struct BakedGradient {
    using enum GradientType;

    GradientType _type = LINEAR;
    Math::Vec2f _start = {0.5, 0.5};
    Math::Vec2f _end = {1, 1};
    BakedGradientStops _stops;

    always_inline f64 transform(Math::Vec2f pos) const {
        pos = pos - _start;
        pos = pos.rotate(-(_end - _start).angle());
        f64 scale = (_end - _start).len();
        pos = pos / scale;

        switch (_type) {
        case LINEAR:
            return pos.x;

        case RADIAL:
            return pos.len();

        case CONICAL:
            return (pos.angle() + Math::PI) / Math::TAU;

        case DIAMOND:
            return Math::abs(pos.x) + Math::abs(pos.y);
        }
    }

    always_inline Color sample(Math::Vec2f pos) const {
        auto p = transform(pos);
        return _stops[clamp(static_cast<usize>(p * 255), 0uz, 255uz)];
    }
};

export struct Gradient {
    using enum GradientType;

    GradientType type = LINEAR;
    Math::Vec2f start = {0.5, 0.5};
    Math::Vec2f end = {1, 1};
    SmallVec<GradientStop, 4> stops = {};

    static Gradient linear() {
        return Gradient{LINEAR, {0, 0}, {1, 1}};
    }

    static Gradient hsv() {
        return hlinear().withHsv();
    }

    static Gradient vlinear() {
        return Gradient{LINEAR, {0.5, 0}, {0.5, 1}};
    }

    static Gradient hlinear() {
        return Gradient{LINEAR, {0, 0.5}, {1, 0.5}};
    }

    static Gradient radial() {
        return Gradient{RADIAL, {0.5, 0.5}, {1, 0.5}};
    }

    static Gradient conical() {
        return Gradient{CONICAL, {0.5, 0.5}, {1, 0.5}};
    }

    static Gradient diamond() {
        return Gradient{DIAMOND, {0.5, 0.5}, {1, 0.5}};
    }

    Gradient& withType(GradientType type) {
        this->type = type;
        return *this;
    }

    Gradient& withStop(Color color, f64 pos) {
        stops.pushBack({color, pos});
        return *this;
    }

    Gradient& withPoints(Math::Vec2f start, Math::Vec2f end) {
        this->start = start;
        this->end = end;
        return *this;
    }

    Gradient& withStart(Math::Vec2f start) {
        this->start = start;
        return *this;
    }

    Gradient& withEnd(Math::Vec2f end) {
        this->end = end;
        return *this;
    }

    Gradient& withHsv() {
        for (f64 i = 0; i <= 360; i += 30)
            withStop(hsvToRgb({i, 1, 1}), i / 360.0);
        return *this;
    }

    Gradient& withColors(Meta::Same<Color> auto... args) {
        Array colors = {args...};
        if (colors.len() == 1)
            return withStop(colors[0], 0.5);
        for (usize i = 0; i < colors.len(); i++)
            withStop(colors[i], static_cast<f64>(i) / (colors.len() - 1));
        return *this;
    }

    void repr(Io::Emit& e) const {
        e("(gradient {} {} {} {})", type, start, end, stops);
    }

    BakedGradient bake() const {
        return {
            type,
            start,
            end,
            GradientStop::bake(stops, type == CONICAL)
        };
    }
};

using _Fills = Union<
    Color,
    Gradient,
    Rc<Image>>;

export struct Fill : _Fills {
    using _Fills::_Fills;

    Fill(Color color = ALPHA) : _Fills(color) {}

    void repr(Io::Emit& e) const {
        visit(
            [&](auto const& i) {
                e("{}", i);
            }
        );
    }
};

} // namespace Karm::Gfx
