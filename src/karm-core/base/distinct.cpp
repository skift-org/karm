module;

#include <karm/macros>

export module Karm.Core:base.distinct;

import :base.base;
import :base.hash;

namespace Karm {

export template <typename T, typename Tag>
struct Distinct {
    T _value;

    always_inline constexpr Distinct() = default;

    always_inline constexpr explicit Distinct(T value) : _value(value) {}

    always_inline constexpr T value() const { return _value; }

    always_inline constexpr bool operator==(Distinct<T, Tag> const& other) const = default;

    always_inline constexpr auto operator<=>(Distinct<T, Tag> const& other) const = default;

    always_inline constexpr Distinct<T, Tag> operator+(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value + other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator-(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value - other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator*(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value * other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator/(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value / other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator%(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value % other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator&(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value & other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator|(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value | other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator^(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value ^ other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator<<(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value << other._value);
    }

    always_inline constexpr Distinct<T, Tag> operator>>(Distinct<T, Tag> other) const {
        return Distinct<T, Tag>(_value >> other._value);
    }

    always_inline constexpr Distinct<T, Tag>& operator+=(Distinct<T, Tag> other) {
        _value += other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator-=(Distinct<T, Tag> other) {
        _value -= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator++() {
        ++_value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag> operator++(int) {
        auto copy = *this;
        ++_value;
        return copy;
    }

    always_inline constexpr Distinct<T, Tag>& operator--() {
        --_value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag> operator--(int) {
        auto copy = *this;
        --_value;
        return copy;
    }

    always_inline constexpr Distinct<T, Tag>& operator*=(Distinct<T, Tag> other) {
        _value *= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator/=(Distinct<T, Tag> other) {
        _value /= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator%=(Distinct<T, Tag> other) {
        _value %= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator&=(Distinct<T, Tag> other) {
        _value &= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator|=(Distinct<T, Tag> other) {
        _value |= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator^=(Distinct<T, Tag> other) {
        _value ^= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator<<=(Distinct<T, Tag> other) {
        _value <<= other._value;
        return *this;
    }

    always_inline constexpr Distinct<T, Tag>& operator>>=(Distinct<T, Tag> other) {
        _value >>= other._value;
        return *this;
    }

    always_inline u64 hash() const {
        return Karm::hash(_value);
    }
};

static_assert(sizeof(Distinct<int, struct _Tag>) == sizeof(int));
static_assert(alignof(Distinct<int, struct _Tag>) == alignof(int));

} // namespace Karm
