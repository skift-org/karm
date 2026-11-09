export module Karm.Core:base.float_;

import :base.base;

namespace Karm {

export template <typename T>
union FloatBits;

export template <>
union FloatBits<f32> {
    using Raw = u32;

    static constexpr int mantissaBits = 23;
    static constexpr Raw implicitBit = Raw{1} << mantissaBits;
    static constexpr Raw mantissaMax = implicitBit - 1;
    static constexpr int exponentBias = 127;
    static constexpr int exponentBits = 8;
    static constexpr int exponentMax = (1 << exponentBits) - 1;

    static FloatBits from(Raw sign, Raw exponent, Raw mantissa) {
        if constexpr (isLittleEndian()) {
            return {
                .mantissa = mantissa,
                .exponent = exponent,
                .sign = sign,
            };
        } else {
            return {
                .sign = sign,
                .exponent = exponent,
                .mantissa = mantissa
            };
        }
    }

    static FloatBits from(f32 d) {
        return {.d = d};
    }

    struct [[gnu::packed]] {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        Raw sign : 1;
        Raw exponent : 8;
        Raw mantissa : 23;
#else
        Raw mantissa : 23;
        Raw exponent : 8;
        Raw sign : 1;
#endif
    };

    f32 d;
};

static_assert(sizeof(f32) == 4);
static_assert(sizeof(FloatBits<f32>) == sizeof(f32));

export template <>
union FloatBits<f64> {
    using Raw = u64;

    static constexpr int mantissaBits = 52;
    static constexpr Raw implicitBit = Raw{1} << mantissaBits;
    static constexpr Raw mantissaMax = implicitBit - 1;
    static constexpr int exponentBias = 1023;
    static constexpr int exponentBits = 11;
    static constexpr int exponentMax = (1 << exponentBits) - 1;

    static FloatBits from(Raw sign, Raw exponent, Raw mantissa) {
        if constexpr (isLittleEndian()) {
            return {
                .mantissa = mantissa,
                .exponent = exponent,
                .sign = sign,
            };
        } else {
            return {
                .sign = sign,
                .exponent = exponent,
                .mantissa = mantissa
            };
        }
    }

    static FloatBits from(f64 d) {
        return {.d = d};
    }

    struct [[gnu::packed]] {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        Raw sign : 1;
        Raw exponent : 11;
        Raw mantissa : 52;
#else
        Raw mantissa : 52;
        Raw exponent : 11;
        Raw sign : 1;
#endif
    };

    f64 d;
};

static_assert(sizeof(f64) == 8);
static_assert(sizeof(FloatBits<f64>) == sizeof(f64));

#ifdef __SIZEOF_FLOAT128__

export template <>
union FloatBits<f128> {
    using Raw = u128;

    static constexpr int mantissaBits = 112;
    static constexpr Raw implicitBit = Raw{1} << mantissaBits;
    static constexpr Raw mantissaMax = implicitBit - 1;
    static constexpr int exponentBias = 16383;
    static constexpr int exponentBits = 15;
    static constexpr int exponentMax = (1 << exponentBits) - 1;

    static FloatBits from(Raw sign, Raw exponent, Raw mantissa) {
        if constexpr (isLittleEndian()) {
            return {
                .mantissa = mantissa,
                .exponent = exponent,
                .sign = sign,
            };
        } else {
            return {
                .sign = sign,
                .exponent = exponent,
                .mantissa = mantissa
            };
        }
    }

    static FloatBits from(f128 d) {
        return {.d = d};
    }

    struct [[gnu::packed]] {
#    if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        Raw sign : 1;
        Raw exponent : 15;
        Raw mantissa : 112;
#    else
        Raw mantissa : 112;
        Raw exponent : 15;
        Raw sign : 1;
#    endif
    };

    f128 d;
};

static_assert(sizeof(f128) == 16);
static_assert(sizeof(FloatBits<f128>) == sizeof(f128));

#endif

} // namespace Karm
