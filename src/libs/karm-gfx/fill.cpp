module;

#include <karm-math/trans.h>

export module Karm.Gfx:fill;

import Karm.Core;

import :buffer;
import :color;
import :colors;

namespace Karm::Gfx {

export enum struct FillRule {
    NONZERO,
    EVENODD,
};

export struct Gradient {
    enum Type {
        LINEAR,
        RADIAL,
        CONICAL,
        DIAMOND,
    };

    using Buf = Array<Color, 256>;
    using Stop = Pair<Color, f64>;

    Type _type = LINEAR;
    Math::Vec2f _start = {0.5, 0.5};
    Math::Vec2f _end = {1, 1};
    Rc<Buf> _buf;

    struct Builder {
        static constexpr isize LIMIT = 16;

        Type _type = LINEAR;
        Math::Vec2f _start = {0.5, 0.5};
        Math::Vec2f _end = {1, 1};
        InlineVec<Stop, LIMIT> _stops;

        Builder(Type type) : _type(type) {}

        Builder(Type type, Math::Vec2f start, Math::Vec2f end)
            : _type(type), _start(start), _end(end) {}

        Builder& withStop(Color color, f64 pos) {
            _stops.pushBack({color, pos});
            return *this;
        }

        Builder& withStart(Math::Vec2f start) {
            _start = start;
            return *this;
        }

        Builder& withEnd(Math::Vec2f end) {
            _end = end;
            return *this;
        }

        Builder& withHsv() {
            for (f64 i = 0; i <= 360; i += 30)
                withStop(hsvToRgb({i, 1, 1}), i / 360.0);
            return *this;
        }

        Builder& withColors(Meta::Same<Color> auto... args) {
            Array colors = {args...};

            if (colors.len() == 1)
                return withStop(colors[0], 0.5);

            for (usize i = 0; i < colors.len(); i++)
                withStop(colors[i], (f64)i / (colors.len() - 1));

            return *this;
        }

        static Color _lerp(Gradient::Stop lhs, Gradient::Stop rhs, f64 pos) {
            f64 t = (pos - lhs.v1) / (rhs.v1 - lhs.v1);
            return lhs.v0.lerpWith(rhs.v0, t);
        }

        static void _bakeStops(Slice<Gradient::Stop> stops, Gradient::Buf& buf, bool wraparound) {
            fill(mutSub(buf), Gfx::BLACK);

            if (stops.len() == 0)
                return;

            if (stops.len() == 1) {
                fill(mutSub(buf), stops[0].v0);
                return;
            }

            for (float j = 0; j < stops[0].v1 * 256; j++) {
                if (wraparound) {
                    auto lhs = stops[stops.len() - 1];
                    lhs.v1 -= 1;
                    buf[j] = _lerp(lhs, stops[0], j / 256.0);
                } else {
                    buf[j] = stops[0].v0;
                }
            }

            for (usize i = 0; i < stops.len() - 1; i++) {
                auto iPos = stops[i].v1;
                auto jPos = stops[i + 1].v1;

                for (f64 j = iPos * 256; j < jPos * 256; j++)
                    buf[j] = _lerp(stops[i], stops[i + 1], j / 256.0);
            }

            for (f64 j = last(stops).v1 * 256; j < 256; j++) {
                if (wraparound) {
                    auto rhs = stops[0];
                    rhs.v1 += 1;
                    buf[j] = _lerp(last(stops), rhs, j / 256.0);
                } else {
                    buf[j] = last(stops).v0;
                }
            }
        }

        Gradient bake() {
            auto buf = makeRc<Buf>();
            _bakeStops(_stops, *buf, _type == CONICAL);
            return {_type, _start, _end, buf};
        }
    };

    static Builder linear() {
        return Builder{LINEAR, {0, 0}, {1, 1}};
    }

    static Builder hsv() {
        return hlinear()
            .withHsv();
    }

    static Builder vlinear() {
        return Builder{LINEAR, {0.5, 0}, {0.5, 1}};
    }

    static Builder hlinear() {
        return Builder{LINEAR, {0, 0.5}, {1, 0.5}};
    }

    static Builder radial() {
        return Builder{RADIAL, {0.5, 0.5}, {1, 0.5}};
    }

    static Builder conical() {
        return Builder{CONICAL, {0.5, 0.5}, {1, 0.5}};
    }

    static Builder diamond() {
        return Builder{DIAMOND, {0.5, 0.5}, {1, 0.5}};
    }

    Gradient(Type type, Math::Vec2f start, Math::Vec2f end, Rc<Buf> buf)
        : _type(type), _start(start), _end(end), _buf(buf) {}

    Gradient& withType(Type type) {
        _type = type;
        return *this;
    }

    Gradient& withStart(Math::Vec2f start) {
        _start = start;
        return *this;
    }

    Gradient& withEnd(Math::Vec2f end) {
        _end = end;
        return *this;
    }

    Gradient& withPoints(Math::Vec2f start, Math::Vec2f end) {
        _start = start;
        _end = end;
        return *this;
    }

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
        return (*_buf)[clamp(usize(p * 255), 0uz, 255uz)];
    }
};

using _Fills = Union<
    Color,
    Gradient,
    Pixels>;

export struct Fill : _Fills {
    using _Fills::_Fills;

    Fill(Color color = ALPHA) : _Fills(color) {}

    always_inline Color sample(Math::Vec2f pos) const {
        return visit(
            [&](auto const& p) {
                return p.sample(pos);
            }
        );
    }

    void repr(Io::Emit& e) const {
        visit(Visitor{
            [&](Color const& c) {
                c.repr(e);
            },
            [&](Gradient const& grad) {
                e("(gradient {} {})", grad._start, grad._end);
            },
            [&](Pixels const& pixels) {
                e("(pixels {} {})", pixels.width(), pixels.height());
            },
        });
    }
};

} // namespace Karm::Gfx
