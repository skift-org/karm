module;

#include <karm/macros>

export module Karm.Math:au;

import Karm.Core;
import :fixed;

namespace Karm::Math {

// Au (aka Application Unit, Atomic Unit, Absurd Unit, Almighty Unit, Annoying Unit, Autistic Unit, Awesome Unit, Anarcho-Unit, Avocado Unit, Adorable Unit, etc...) is the fundamental
// unit of measurement in Karm.
// It's inspired by Mozilla's AppUnits, see:
//  - https://docs.rs/app_units/latest/app_units/
//  - https://bugzilla.mozilla.org/show_bug.cgi?id=177805
export struct Au {
    static constexpr i32 _DENO = 60.0;
    static constexpr i32 _MAX = (1 << 30) - 1;
    static constexpr i32 _MIN = -_MAX;

    i32 _val = 0;

    constexpr Au() = default;

    explicit constexpr Au(Meta::Float auto value) : Au(fromFloatNearest(value)) {}

    explicit constexpr Au(Meta::Integral auto value) : Au(static_cast<f64>(value)) {}

    always_inline static constexpr Au fromRaw(i32 value) {
        Au au;
        au._val = value;
        return au;
    }

    template <Meta::Float F>
    always_inline static constexpr Au fromFloatNearest(F value) {
        if (isNan(value))
            return Au{};

        f64 val = static_cast<f64>(value) * _DENO;

        if (val > static_cast<f64>(_MAX))
            return fromRaw(_MAX);
        if (val < static_cast<f64>(_MIN))
            return fromRaw(_MIN);

        return fromRaw(roundi(val));
    }

    template <Meta::Float F>
    always_inline static constexpr Au fromFloatFloor(F value) {
        if (isNan(value))
            return Au{};

        f64 val = static_cast<f64>(value) * _DENO;

        if (val > static_cast<f64>(_MAX))
            return fromRaw(_MAX);
        if (val < static_cast<f64>(_MIN))
            return fromRaw(_MIN);

        return fromRaw(floori(val));
    }

    template <Meta::Float F>
    always_inline static constexpr Au fromFloatCeil(F value) {
        if (isNan(value))
            return Au{};

        f64 val = static_cast<f64>(value) * _DENO;

        if (val > static_cast<f64>(_MAX))
            return fromRaw(_MAX);
        if (val < static_cast<f64>(_MIN))
            return fromRaw(_MIN);

        return fromRaw(ceili(val));
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
        return static_cast<f64>(_val) / _DENO;
    }

    template <typename U>
        requires Meta::Float<U> or Meta::Integral<U>
    explicit constexpr operator U() const {
        return cast<U>();
    }

    void repr(Io::Emit& e) const {
        e("{}au", cast<f64>());
    }
};

export constexpr Au abs(Au const& val) {
    return val < Au{0} ? -val : val;
}

} // namespace Karm::Math

export template <>
struct Karm::Limits<Karm::Math::Au> {
    static constexpr Math::Au MIN = Math::Au::fromRaw(Math::Au::_MIN);
    static constexpr Math::Au MAX = Math::Au::fromRaw(Math::Au::_MAX);
    static constexpr Math::Au EPSILON = Math::Au::fromRaw(1);
    static constexpr bool SIGNED = true;
};

namespace Karm::Math::Literals {

export constexpr Au operator""_au(unsigned long long val) {
    return Au{val};
}

export constexpr Au operator""_au(long double val) {
    return Au{val};
}

} // namespace Karm::Math::Literals
