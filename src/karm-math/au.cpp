module;

#include <karm/macros>

export module Karm.Math:au;

import :fixed;
import :insets;
import :radii;
import :rect;

// NOTE: Not in the Karm::Math namespace because it's pretty commonly used
//       and typing Math::Au is a bit too much.
namespace Karm {

// Au (aka Application Unit, Atomic Unit, Absurd Unit, Almighty Unit, Annoying Unit, Autistic Unit, Awesome Unit, Anarcho-Unit, Avocado Unit, Adorable Unit, etc...) is the fundamental
// unit of measurement in Karm.
// It's inspired by Mozilla's AppUnits, see:
//  - https://docs.rs/app_units/latest/app_units/
//  - https://bugzilla.mozilla.org/show_bug.cgi?id=177805
export struct Au {
    static constexpr f64 _UNIT_PER_PX = 60.0;
    static constexpr i32 _MAX = (1 << 30) - 1;
    static constexpr i32 _MIN = -_MAX;

    i32 _val = 0;

    constexpr Au() = default;

    template <Meta::Float F>
    explicit constexpr Au(F px) : Au(fromFloatNearest(px)) {}

    template <Meta::Integral I>
    explicit constexpr Au(I px) : Au(static_cast<f64>(px)) {}

    always_inline static constexpr Au fromRaw(i32 value) {
        Au au;
        au._val = value;
        return au;
    }

    template <Meta::Float F>
    always_inline static constexpr Au fromFloatNearest(F px) {
        if (Math::isNan(px))
            return Au{};

        f64 val = static_cast<f64>(px) * _UNIT_PER_PX;

        if (val > static_cast<f64>(_MAX))
            return fromRaw(_MAX);
        if (val < static_cast<f64>(_MIN))
            return fromRaw(_MIN);

        return fromRaw(Math::roundi(val));
    }

    template <Meta::Float F>
    always_inline static constexpr Au fromFloatFloor(F px) {
        if (Math::isNan(px))
            return Au{};

        f64 val = static_cast<f64>(px) * _UNIT_PER_PX;

        if (val > static_cast<f64>(_MAX))
            return fromRaw(_MAX);
        if (val < static_cast<f64>(_MIN))
            return fromRaw(_MIN);

        return fromRaw(Math::floori(val));
    }

    template <Meta::Float F>
    always_inline static constexpr Au fromFloatCeil(F px) {
        if (Math::isNan(px))
            return Au{};

        f64 val = static_cast<f64>(px) * _UNIT_PER_PX;

        if (val > static_cast<f64>(_MAX))
            return fromRaw(_MAX);
        if (val < static_cast<f64>(_MIN))
            return fromRaw(_MIN);

        return fromRaw(Math::ceili(val));
    }

    always_inline constexpr bool operator==(Au const& other) const = default;

    always_inline constexpr auto operator<=>(Au const& other) const = default;

    always_inline constexpr Au clamp() const {
        if (_val > _MAX)
            return fromRaw(_MAX);
        if (_val < _MIN)
            return fromRaw(_MIN);
        return *this;
    }

    always_inline constexpr Au operator-() const {
        return fromRaw(-_val).clamp();
    }

    always_inline constexpr Au operator+(Au other) const {
        return fromRaw(_val + other._val).clamp();
    }

    always_inline constexpr Au operator-(Au other) const {
        return fromRaw(_val - other._val).clamp();
    }

    always_inline constexpr Au operator*(f64 other) const {
        return fromRaw(static_cast<f64>(_val) * other).clamp();
    }

    always_inline constexpr Au operator/(f64 other) const {
        return fromRaw(static_cast<f64>(_val) / other).clamp();
    }

    always_inline constexpr f64 operator/(Au other) const {
        return static_cast<f64>(_val) / static_cast<f64>(other._val);
    }

    always_inline constexpr Au& operator+=(Au other) {
        *this = *this + other;
        return *this;
    }

    always_inline constexpr Au& operator-=(Au other) {
        *this = *this - other;
        return *this;
    }

    always_inline constexpr Au& operator*=(f64 other) {
        *this = *this * other;
        return *this;
    }

    always_inline constexpr Au& operator/=(f64 other) {
        *this = *this / other;
        return *this;
    }

    template <typename U>
    constexpr U cast() const {
        return static_cast<f64>(_val) / _UNIT_PER_PX;
    }

    template <typename U>
        requires Meta::Float<U> or Meta::Integral<U>
    explicit constexpr operator U() const {
        return cast<U>();
    }

    void repr(Io::Emit& e) const {
        e("{}px", this->cast<f64>());
    }
};

export always_inline constexpr Au operator*(f64 lhs, Au rhs) {
    return rhs * lhs;
}

export template <>
struct Karm::Limits<Au> {
    static constexpr Au MIN = Au::fromRaw(Au::_MIN);
    static constexpr Au MAX = Au::fromRaw(Au::_MAX);
    static constexpr Au EPSILON = Au::fromRaw(1);
    static constexpr bool SIGNED = true;
};

export constexpr Au abs(Au const& val) {
    return val < Au{0} ? -val : val;
}

export using RectAu = Math::Rect<Au>;

export using Vec2Au = Math::Vec2<Au>;

export using InsetsAu = Math::Insets<Au>;

export using RadiiAu = Math::Radii<Au>;

} // namespace Karm

namespace Karm::Math::Literals {

export constexpr Karm::Au operator""_au(unsigned long long val) {
    return Karm::Au{val};
}

export constexpr Karm::Au operator""_au(long double val) {
    return Karm::Au{val};
}

} // namespace Karm::Math::Literals
