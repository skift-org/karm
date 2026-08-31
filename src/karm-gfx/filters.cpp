export module Karm.Gfx:filters;

import Karm.Core;
import Karm.Math;
import Karm.Gfx.Pixels;

namespace Karm::Gfx {

export struct Unfiltered {
    static constexpr auto NAME = "unfiltered";

    void apply(MutPixels) const {}
};

export struct BlurFilter {
    static constexpr auto NAME = "blur";
    static constexpr frange RANGE = frange::fromStartEnd(0, 32);
    static constexpr f64 DEFAULT = 16;

    f64 amount = DEFAULT;

    struct StackBlur {
        isize _radius;
        Ring<Math::Vec4u> _queue;
        Math::Vec4u _sum;

        StackBlur(isize radius)
            : _radius(radius), _queue(width()) { clear(); }

        Math::Vec4u outgoingSumCurrent() const {
            Math::Vec4u sum = {};
            for (isize i = 0; i <= _radius; i++)
                sum = sum + _queue.peekFront(i);
            return sum;
        }

        Math::Vec4u incomingSumWith(Math::Vec4u in) const {
            Math::Vec4u sum = in;
            for (isize i = 0; i < _radius; i++)
                sum = sum + _queue.peekFront(width() - 1 - i);
            return sum;
        }

        isize width() const {
            return _radius * 2 + 1;
        }

        isize denominator() const {
            return (_radius + 1) * (_radius + 1);
        }

        void enqueue(Math::Vec4u color) {
            _sum = _sum + incomingSumWith(color) - outgoingSumCurrent();
            _queue.popFront();
            _queue.pushBack(color);
        }

        Math::Vec4u dequeue() const {
            auto d = denominator();
            return (_sum + Math::Vec4u(d / 2)) / d;
        }

        void clear() {
            _sum = {};
            _queue.clear();
            for (isize i = 0; i < width(); i++)
                _queue.pushBack({});
        }
    };

    void apply(MutPixels p) const {
        if (amount == 0)
            return;

        StackBlur stack{(isize)amount};
        auto b = p.bound();

        for (isize y = b.top(); y < b.bottom(); y++) {
            for (isize i = 0; i < stack.width(); i++) {
                auto x = b.start() + i - (isize)amount;
                stack.dequeue();
                stack.enqueue(p.load({x, y}));
            }

            for (isize x = b.start(); x < b.end(); x++) {
                p.store({x, y}, stack.dequeue());
                stack.enqueue(p.load({x + (isize)amount + 1, y}));
            }

            stack.clear();
        }

        for (isize x = b.start(); x < b.end(); x++) {
            for (isize i = 0; i < stack.width(); i++) {
                isize const y = b.top() + i - (isize)amount;
                stack.dequeue();
                stack.enqueue(p.load({x, y}));
            }

            for (isize y = b.top(); y < b.bottom(); y++) {
                p.store({x, y}, stack.dequeue());
                stack.enqueue(p.load({x, y + (isize)amount + 1}));
            }

            stack.clear();
        }
    }
};

export struct ColorMatrix {
    Math::Mat4f matrix;
    Math::Vec4f bias;

    static constexpr ColorMatrix identity() {
        return ColorMatrix{
            .matrix = Math::Mat4f::identity(),
            .bias = Math::Vec4f{},
        };
    }

    // Rec. 709
    static constexpr Math::Vec3f LUMA = {0.2126, 0.7152, 0.0722};

    static ColorMatrix grayscale() {
        return {
            .matrix = {
                {LUMA.x, LUMA.x, LUMA.x, 0},
                {LUMA.y, LUMA.y, LUMA.y, 0},
                {LUMA.z, LUMA.z, LUMA.z, 0},
                {0, 0, 0, 1},
            },
            .bias = {},
        };
    }

    static ColorMatrix lerp(ColorMatrix const& a, ColorMatrix const& b, f64 t) {
        return {
            .matrix = a.matrix * (1 - t) + b.matrix * t,
            .bias = a.bias * (1 - t) + b.bias * t,
        };
    }

    static ColorMatrix saturation(f64 a) {
        return lerp(identity(), grayscale(), a);
    }

    static ColorMatrix brightness(f64 a) {
        return {.matrix = Math::Mat4f::scaling(a, a, a), .bias = {}};
    }

    static ColorMatrix tint(Math::Vec4f c) {
        return {
            .matrix = {
                {c.x, 0, 0, 0},
                {0, c.y, 0, 0},
                {0, 0, c.z, 0},
                {0, 0, 0, c.w},
            },
            .bias = {},
        };
    }

    static ColorMatrix contrast(f64 a) {
        f64 f = a + 1;
        f64 o = 0.5 * (1 - f);
        return {.matrix = Math::Mat4f::scaling(f, f, f), .bias = {o, o, o, 0}};
    }

    static ColorMatrix sepia(f64 a) {
        auto s = tint({1.0, 0.8905, 0.6936, 1}) * grayscale();
        return lerp(identity(), s, a);
    }

    static ColorMatrix invert() {
        return {
            .matrix = Math::Mat4f::scaling(-1, -1, -1),
            .bias = {1, 1, 1, 0},
        };
    }

    static ColorMatrix opacity(f64 a) {
        return {
            .matrix = {
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {0, 0, 0, a},
            },
            .bias = {},
        };
    }

    static ColorMatrix hueRotate(f64 rad) {
        f64 c = Math::cos(rad), s = Math::sin(rad);
        return {
            .matrix = {
                {0.213 + c * 0.787 - s * 0.213, 0.213 - c * 0.213 + s * 0.143, 0.213 - c * 0.213 - s * 0.787, 0},
                {0.715 - c * 0.715 - s * 0.715, 0.715 + c * 0.285 + s * 0.140, 0.715 - c * 0.715 + s * 0.715, 0},
                {0.072 - c * 0.072 + s * 0.928, 0.072 - c * 0.072 - s * 0.283, 0.072 + c * 0.928 + s * 0.072, 0},
                {0, 0, 0, 1},
            },
            .bias = {},
        };
    }

    ColorMatrix operator*(ColorMatrix const& other) const {
        return ColorMatrix{
            .matrix = matrix * other.matrix,
            .bias = matrix * other.bias + bias,
        };
    }

    ColorMatrix& operator*=(ColorMatrix const& other) {
        matrix *= other.matrix;
        bias = matrix * other.bias + bias;
        return *this;
    }

    Math::Vec4f apply(Math::Vec4f color) const {
        return matrix * color + bias;
    }
};

export struct ColorMatrixFilter {
    ColorMatrix colorMatrix;

    void apply(MutPixels p) const {
        auto b = p.bound();

        for (isize y = 0; y < b.height; y++) {
            for (isize x = 0; x < b.width; x++) {
                auto color = p.load({b.x + x, b.y + y});
                p.store({b.x + x, b.y + y}, Color::fromFloats(colorMatrix.apply(color.vec4())));
            }
        }
    }
};

export struct NoiseFilter {
    static constexpr auto NAME = "noise";
    static constexpr frange RANGE = frange::fromStartEnd(0, 1);
    static constexpr f64 DEFAULT = 0.5;

    f64 amount = DEFAULT;

    void apply(MutPixels p) const {
        Math::Rand rand{0x12341234};
        u8 alpha = 255 * amount;
        auto b = p.bound();

        for (isize y = 0; y < b.height; y++) {
            for (isize x = 0; x < b.width; x++) {
                u8 noise = rand.nextU8();

                p.blend(
                    {b.x + x, b.y + y},
                    Color::fromRgba(noise, noise, noise, alpha)
                );
            }
        }
    }
};

export struct OverlayFilter {
    static constexpr auto NAME = "overlay";
    static constexpr Color DEFAULT{};

    Color amount = DEFAULT;

    void apply(MutPixels p) const {
        auto b = p.bound();

        for (isize y = 0; y < b.height; y++) {
            for (isize x = 0; x < b.width; x++) {
                p.blend(
                    {b.x + x, b.y + y},
                    amount
                );
            }
        }
    }
};

export struct Filter;

export struct FilterChain {
    static constexpr auto NAME = "chain";

    Pair<Box<Filter>> filters;
    void apply(MutPixels p) const;
};

using _Filters = Union<
    Unfiltered,
    BlurFilter,
    ColorMatrixFilter,
    NoiseFilter,
    OverlayFilter>;

export struct Filter : _Filters {
    using _Filters::_Filters;

    void apply(MutPixels s) const {
        visit([&](auto const& filter) {
            filter.apply(s);
        });
    }
};

void FilterChain::apply(MutPixels p) const {
    filters.visit([&](auto& f) {
        f->apply(p);
        return true;
    });
}

} // namespace Karm::Gfx
