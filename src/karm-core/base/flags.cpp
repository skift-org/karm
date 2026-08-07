module;

#include <karm/macros>

export module Karm.Core:base.flags;

import :base.enum_;

namespace Karm {

export template <Meta::Enum E, typename U = Meta::UnderlyingType<E>>
struct Flags {
    U _value = 0;

    always_inline static Flags fromUnderlying(U underlying) {
        Flags f;
        f._value = underlying;
        return f;
    }

    always_inline Flags() = default;

    always_inline Flags(None){};

    always_inline Flags(E value)
        : _value(toUnderlyingType(value)) {}

    always_inline Flags(std::initializer_list<E> values) {
        for (auto value : values)
            _value |= toUnderlyingType(value);
    }

    always_inline bool has(Flags other) const {
        return (_value & other._value) == other._value;
    }

    always_inline void set(Flags other) {
        _value |= other._value;
    }

    always_inline void set(E value, bool on = true) {
        if (on) {
            _value |= toUnderlyingType(value);
        } else {
            _value &= ~toUnderlyingType(value);
        }
    }

    always_inline void unset(Flags other) {
        _value &= ~other._value;
    }

    always_inline void toggle(E value) {
        _value ^= toUnderlyingType(value);
    }

    always_inline void clear() {
        _value = 0;
    }

    always_inline bool empty() const {
        return _value == 0;
    }

    always_inline bool any(Flags other) const {
        return _value & other._value;
    }

    always_inline bool any() const {
        return _value != 0;
    }

    always_inline U raw() const {
        return _value;
    }

    always_inline explicit operator bool() const {
        return _value != 0;
    }

    always_inline Flags operator~() const {
        Flags res;
        res._value = ~_value;
        return res;
    }

    always_inline Flags operator|(Flags other) const {
        Flags res;
        res._value = _value | other._value;
        return res;
    }

    always_inline Flags operator&(Flags other) const {
        Flags res;
        res._value = _value & other._value;
        return res;
    }

    always_inline Flags operator^(Flags other) const {
        Flags res;
        res._value = _value ^ other._value;
        return res;
    }

    always_inline Flags& operator|=(Flags other) {
        _value |= other._value;
        return *this;
    }

    always_inline Flags& operator&=(Flags other) {
        _value &= other._value;
        return *this;
    }

    always_inline Flags& operator^=(Flags other) {
        _value ^= other._value;
        return *this;
    }

    always_inline bool operator!() const {
        return !_value;
    }

    always_inline bool operator==(Flags other) const {
        return _value == other._value;
    }

    always_inline bool operator!=(Flags other) const {
        return _value != other._value;
    }
};

export template <Meta::Enum E, Meta::Same<E>... Es>
Flags(E, Es...) -> Flags<E>;

} // namespace Karm
