export module Karm.Math:ray;

import :vec;

namespace Karm::Math {

export template <typename V, typename T = typename V::Scalar>
struct Ray {
    using Scalar = T;

    V o;
    V dir;

    constexpr Ray() = default;

    constexpr Ray(V o, V dir)
        : o(o), dir(dir) {
    }

    constexpr V at(T t) const {
        return o + dir * t;
    }

    constexpr auto map(auto f) const {
        using U = decltype(o.map(f));
        return Ray<U>{o.map(f), dir.map(f)};
    }
};

using Rayi = Ray<i64, i64>;

using Rayf = Ray<f64, f64>;

using Ray2i = Ray<Vec2i>;

using Ray2f = Ray<Vec2f>;

using Ray3i = Ray<Vec3i>;

using Ray3f = Ray<Vec3f>;

} // namespace Karm::Math
