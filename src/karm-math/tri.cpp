export module Karm.Math:tri;

import :edge;
import :vec;

namespace Karm::Math {

export template <typename T>
struct Tri2 {
    enum struct Orien {
        CLOCKWISE,
        COUNTER_CLOCKWISE,
        COLLINEAR,
    };

    using Scalar = T;

    union {
        struct {
            T ax, ay, bx, by, cx, cy;
        };

        struct {
            Vec2<T> a, b, c;
        };

        Array<T, 6> _els;
        Array<Vec2<T>, 3> _pts;
    };

    constexpr Tri2()
        : _els{} {}

    constexpr Tri2(T ax, T ay, T bx, T by, T cx, T cy)
        : ax{ax}, ay{ay}, bx{bx}, by{by}, cx{cx}, cy{cy} {}

    constexpr Tri2(Vec2<T> a, Vec2<T> b, Vec2<T> c)
        : a{a}, b{b}, c{c} {}

    constexpr Tri2(Tri2 const& other)
        : _els{other._els} {}

    constexpr Tri2(Tri2&& other)
        : _els{std::move(other._els)} {}

    constexpr Tri2& operator=(Tri2 const& other) {
        _els = other._els;
        return *this;
    }

    constexpr Tri2& operator=(Tri2&& other) {
        _els = std::move(other._els);
        return *this;
    }

    constexpr ~Tri2() {
        _els.~Array();
    }

    constexpr Tri2 reversed() const {
        return {c, b, a};
    }

    constexpr Vec2<T> min() const {
        return a.min(b).min(c);
    }

    constexpr Vec2<T> max() const {
        return a.max(b).max(c);
    }

    constexpr Rect<T> bound() const {
        return Rect<T>::fromTwoPoint(min(), max());
    }

    constexpr T signedArea() const {
        return 0.5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));
    }

    constexpr bool contains(Vec2<T> p) const {
        T d1 = (b - a).cross(p - a);
        T d2 = (c - b).cross(p - b);
        T d3 = (a - c).cross(p - c);

        bool hasNeg = (d1 < 0) or (d2 < 0) or (d3 < 0);
        bool hasPos = (d1 > 0) or (d2 > 0) or (d3 > 0);

        return not(hasNeg and hasPos);
    }

    constexpr bool degenerated(T epsilon = Limits<T>::EPSILON) const {
        return epsilonEq(a, b, epsilon) or
               epsilonEq(b, c, epsilon) or
               epsilonEq(c, a, epsilon);
    }

    constexpr T turn() const {
        return (b - a).cross(c - a);
    }

    Orien orien() const {
        auto t = turn();
        if (t > 0)
            return Orien::CLOCKWISE;
        if (t < 0)
            return Orien::COUNTER_CLOCKWISE;
        return Orien::COLLINEAR;
    }

    void repr(Io::Emit& e) const {
        e("(tri {} {} {})", a, b, c);
    }

    constexpr auto map(auto f) const {
        using U = decltype(f(ax));
        return Tri2<U>{f(ax), f(ay), f(bx), f(by), f(cx), f(cy)};
    }
};

export using Tri2i = Tri2<isize>;
export using Tri2u = Tri2<usize>;
export using Tri2f = Tri2<f64>;

} // namespace Karm::Math
