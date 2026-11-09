export module Karm.Core:math;

import :base.base;
import :base.defer;
import :base.limits;
import :base.panic;
import :base.float_;
import :meta.traits;

// Most function are taken from 9front libc
// Copyright © 2021 Plan 9 Foundation
// Copyright © 20XX 9front authors
// https://github.com/9front/9front/tree/fe19492531980b1772e2912de8163e17f95dfd5e/sys/src/libc/port

namespace Karm::Math {

export constexpr f64 PI = 3.14159265358979323846;                    // π
export constexpr f64 TAU = 2 * PI;                                   // 2π, one full turn
export constexpr f64 HALF_PI = PI / 2.0;                             // π/2
export constexpr f64 SQRT2 = 1.4142135623730950488016887;            // √2
export constexpr f64 L2_E = 1.442695040888963407359924681001892137L; // log2(e) = 1 / ln 2
export constexpr f64 E = 2.71828182845904523536;                     // e, Euler's number
export constexpr f64 NAN = __builtin_nan("");                        // quiet NaN
export constexpr f64 INF = __builtin_inf();                          // +∞
export constexpr f64 NEG_INF = -INF;                                 // −∞
export constexpr f64 DEG2RAD = PI / 180.0;                           // π/180, degrees → radians
export constexpr f64 RAD2DEG = 180.0 / PI;                           // 180/π, radians → degrees
export constexpr f64 SQ2P1 = 2.414213562373095048802e0;              // √2 + 1 = tan(3π/8), atan range reduction
export constexpr f64 SQ2M1 = .414213562373095048802e0;               // √2 − 1 = tan(π/8),  atan range reduction
export constexpr f64 LOG2 = 0.693147180559945309e0;                  // ln 2, log range reduction
export constexpr f64 LOG2O1 = 1 / LOG2;                              // 1/ln 2 = log2(e), log2(x) = ln(x) · this
export constexpr f64 SQRTO2 = 0.707106781186547524e0;                // 1/√2 = √2/2, log mantissa threshold
export constexpr f64 LN10O1 = .4342944819032518276511;               // 1/ln 10 = log10(e), log10(x) = ln(x) · this

#define dispatchBuiltin$(F, ARGS...)              \
    do {                                          \
        if constexpr (Meta::Same<T, long double>) \
            return __builtin_##F##l(ARGS);        \
        else if constexpr (Meta::Same<T, double>) \
            return __builtin_##F(ARGS);           \
        else if constexpr (Meta::Same<T, float>)  \
            return __builtin_##F##f(ARGS);        \
    } while (0)

#define dispatchConstexpr$(F, ARGS...)                           \
    if (std::is_constant_evaluated()) \
        dispatchBuiltin$(F, ARGS);

#ifndef __ck_freestanding__

// MARK: Floating-point manipulation functions ---------------------------------

// https://en.cppreference.com/cpp/numeric/math/frexp
export template <Meta::Float T>
constexpr T frexp(T d, int* ep) {
    dispatchConstexpr$(frexp, d, ep);

    using Bits = FloatBits<T>;
    using U = typename Bits::Raw;

    *ep = 0;
    auto x = Bits::from(d);
    int e = x.exponent;
    U m = x.mantissa;

    if (e == Bits::exponentMax or d == 0)
        return d;

    if (e == 0) { /* normalize subnormal numbers */
        e = 1;
        while (not(m & Bits::implicitBit)) {
            m <<= 1;
            e--;
        }
    }

    /* result mantissa is in [0.5, 1), i.e. biased exponent is exponentBias - 1 */
    *ep = e - (Bits::exponentBias - 1);
    return Bits::from(static_cast<U>(x.sign), static_cast<U>(Bits::exponentBias - 1), m & Bits::mantissaMax).d;
}

// https://en.cppreference.com/cpp/numeric/math/ldexp
export template <Meta::Float T>
constexpr T ldexp(T d, int deltae) {
    dispatchConstexpr$(ldexp, d, deltae);

    using Bits = FloatBits<T>;
    using U = typename Bits::Raw;

    auto x = Bits::from(d);
    int e = x.exponent;
    U m = x.mantissa;
    U sign = x.sign;

    if (d == 0 or e == Bits::exponentMax)
        return d;

    if (e == 0) {
        e = 1;
        while (not(m & Bits::implicitBit)) {
            m <<= 1;
            e--;
        }
    } else {
        m |= Bits::implicitBit;
    }

    constexpr int LIMIT = 2 * Bits::exponentMax + Bits::mantissaBits;
    if (deltae > LIMIT)
        deltae = LIMIT;
    if (deltae < -LIMIT)
        deltae = -LIMIT;
    e += deltae;

    if (e >= 1) {                   /* no underflow */
        if (e >= Bits::exponentMax) /* overflow */
            return Bits::from(sign, (U)Bits::exponentMax, 0).d;
        return Bits::from(sign, (U)e, m & Bits::mantissaMax).d;
    } else { /* underflow gracefully */
        /* need to shift d right deltae */
        int shift = 1 - e;
        if (shift > Bits::mantissaBits + 1)
            return Bits::from(sign, 0, 0).d;

        /* shift bits down */
        U q = m >> shift;
        U rem = m & ((U{1} << shift) - 1);
        U half = U{1} << (shift - 1);
        if (rem > half or (rem == half and (q & 1)))
            q++;

        return Bits::from(sign, q >> Bits::mantissaBits, q & Bits::mantissaMax).d;
    }
}

// https://en.cppreference.com/c/numeric/math/modf
export template <Meta::Float T>
constexpr T modf(T d, T* ip) {
    dispatchConstexpr$(modf, d, ip);

    using Bits = FloatBits<T>;
    using U = typename Bits::Raw;

    auto x = Bits::from(d);

    if (static_cast<int>(x.exponent) == Bits::exponentMax) {
        *ip = d;
        if (x.mantissa != 0) /* NaN */
            return d;
        /* ±Inf: fractional part is ±0 */
        return Bits::from(static_cast<U>(x.sign), U{0}, U{0}).d;
    }

    if (d < 1) {
        if (d < 0) {
            T f = modf(-d, ip);
            *ip = -*ip;
            return -f;
        }
        *ip = 0;
        return d;
    }

    /* d >= 1, so d = 1.m × 2^e with e >= 0 */
    int e = static_cast<int>(x.exponent) - Bits::exponentBias;
    if (e < Bits::mantissaBits) {
        /* the low (mantissaBits - e) bits of the mantissa are the fraction: clear them */
        U fracMask = (U{1} << (Bits::mantissaBits - e)) - 1;
        *ip = Bits::from(
                  static_cast<U>(x.sign),
                  static_cast<U>(x.exponent),
                  static_cast<U>(x.mantissa) & ~fracMask
        )
                  .d;
    } else {
        /* no fractional bits left, d is already an integer */
        *ip = d;
    }
    return d - *ip;
}

// MARK: Basic operations ------------------------------------------------------

// https://en.cppreference.com/c/numeric/math/fabs
export template <typename T>
constexpr T abs(T x) {
    return x < T{} ? -x : x;
}

export template <typename T>
constexpr bool epsilonEq(T lhs, T rhs, T epsilon = Limits<T>::EPSILON) {
    return abs(lhs - rhs) < epsilon;
}

export template <typename V, typename T>
constexpr auto lerp(V a, V b, T p) {
    return a + (b - a) * p;
}

// https://en.cppreference.com/c/numeric/math/fmod
export template <Meta::Float T>
constexpr T fmod(T x, T y) {
    dispatchConstexpr$(fmod, x, y);

    int yexp;
    int rexp;
    double r;

    if (y == 0)
        return x;
    if (y < 0)
        y = -y;
    double yfr = frexp(y, &yexp);
    int sign = 0;
    if (x < 0) {
        r = -x;
        sign++;
    } else
        r = x;
    while (r >= y) {
        double rfr = frexp(r, &rexp);
        r -= ldexp(y, rexp - yexp - (rfr < yfr));
    }
    if (sign)
        r = -r;
    return r;
}

// MARK: Nearest integer floating-point operations -----------------------------

export template <typename T>
constexpr isize floori(T x) {
    isize inum = (isize)x;
    if ((T)inum == x)
        return inum;
    if (x < T{0})
        return inum - 1;
    return inum;
}

export template <typename T>
constexpr isize ceili(T x) {
    isize inum = (isize)x;
    if ((T)inum == x)
        return inum;
    if (x < T{0})
        return inum;
    return inum + 1;
}

export template <typename T>
constexpr isize roundi(T x) {
    if (x < T{0})
        return (long)(x - T{0.5});
    return (long)(x + T{0.5});
}

// https://en.cppreference.com/c/numeric/math/floor
export template <typename T>
constexpr T floor(T x) {
    dispatchConstexpr$(floor, x);
    return static_cast<T>(floori(x));
}

// https://en.cppreference.com/c/numeric/math/ceil
export template <typename T>
constexpr T ceil(T x) {
    dispatchConstexpr$(ceil, x);
    return static_cast<T>(ceili(x));
}

// https://en.cppreference.com/c/numeric/math/round
export template <typename T>
constexpr T round(T x) {
    dispatchConstexpr$(round, x);
    return static_cast<T>(roundi(x));
}

// https://en.cppreference.com/c/numeric/math/trunc
export template <Meta::Float T>
constexpr T trunc(T d) {
    dispatchConstexpr$(trunc, d);

    using Bits = FloatBits<T>;
    using U = typename Bits::Raw;

    auto x = Bits::from(d);

    if (static_cast<int>(x.exponent) == Bits::exponentMax) /* NaN or ±Inf */
        return d;

    int e = static_cast<int>(x.exponent) - Bits::exponentBias;
    if (e < 0) /* |d| < 1 (incl. subnormals and ±0): result is ±0 */
        return Bits::from(static_cast<U>(x.sign), U{0}, U{0}).d;

    if (e >= Bits::mantissaBits) /* no fractional bits left, d is already an integer */
        return d;

    /* the low (mantissaBits - e) bits of the mantissa are the fraction: clear them */
    U fracMask = (U{1} << (Bits::mantissaBits - e)) - 1;
    return Bits::from(
               static_cast<U>(x.sign),
               static_cast<U>(x.exponent),
               static_cast<U>(x.mantissa) & ~fracMask
    )
        .d;
}

// MARK: Classification and comparison -----------------------------------------

// NOSPEC: return if a number is odd
export template <Meta::Float T>
constexpr bool isOdd(T v) {
    T iv;
    if (modf(v, &iv) != 0)
        return false;
    return static_cast<i64>(iv) & 1;
}

// https://en.cppreference.com/c/numeric/math/isNan
export template <Meta::Float T>
constexpr bool isNan(T v) {
    dispatchConstexpr$(isnan, v);
    using Bits = FloatBits<T>;
    auto bits = Bits::from(v);
    return bits.exponent == Bits::exponentMax and bits.mantissa != 0;
}

export template <typename F>
constexpr F inf(int sign) {
    using Bits = FloatBits<F>;
    return Bits::from(sign < 0, Bits::exponentMax, 0).d;
}

// https://en.cppreference.com/c/numeric/math/isinf
export template <Meta::Float T>
constexpr bool isInf(T v, int sign = 0) {
    using Bits = FloatBits<T>;
    auto bits = Bits::from(v);
    if (bits.exponent != Bits::exponentMax or bits.mantissa != 0)
        return false;
    return bits.sign ? sign <= 0 : sign >= 0;
}

// MARK: Exponential functions -------------------------------------------------

// https://en.cppreference.com/c/numeric/math/exp2
export template <Meta::Float T>
constexpr T exp2(T arg) {
    dispatchConstexpr$(exp2, arg);

    static constexpr auto P0 = .2080384346694663001443843411e7;
    static constexpr auto P1 = .3028697169744036299076048876e5;
    static constexpr auto P2 = .6061485330061080841615584556e2;
    static constexpr auto Q0 = .6002720360238832528230907598e7;
    static constexpr auto Q1 = .3277251518082914423057964422e6;
    static constexpr auto Q2 = .1749287689093076403844945335e4;
    static constexpr auto MAXF = 10000;

    if (isNan(arg))
        return arg;
    if (arg == 0)
        return 1;
    if (arg < -MAXF)
        return 0;
    if (arg > MAXF)
        return inf<T>(1);

    int ent = floori(arg);
    T fract = (arg - ent) - T(0.5);
    T xsq = fract * fract;
    T temp1 = ((P2 * xsq + P1) * xsq + P0) * fract;
    T temp2 = ((xsq + Q2) * xsq + Q1) * xsq + Q0;
    return ldexp(T(SQRT2 * (temp2 + temp1) / (temp2 - temp1)), ent);
}

// https://en.cppreference.com/c/numeric/math/exp
export template <Meta::Float T>
constexpr T exp(T arg) {
    dispatchConstexpr$(exp, arg);
    return exp2(arg * L2_E);
}

// https://en.cppreference.com/c/numeric/math/log
export template <Meta::Float T>
constexpr T log(T arg) {
    dispatchConstexpr$(log, arg);

    static constexpr auto P0 = -.240139179559210510e2;
    static constexpr auto P1 = 0.309572928215376501e2;
    static constexpr auto P2 = -.963769093377840513e1;
    static constexpr auto P3 = 0.421087371217979714e0;
    static constexpr auto Q0 = -.120069589779605255e2;
    static constexpr auto Q1 = 0.194809660700889731e2;
    static constexpr auto Q2 = -.891110902798312337e1;

    if (arg <= 0)
        return NAN;
    int exp;
    double x = frexp(arg, &exp);
    while (x < 0.5) {
        x *= 2;
        exp--;
    }
    if (x < SQRTO2) {
        x *= 2;
        exp--;
    }

    double z = (x - 1) / (x + 1);
    double zsq = z * z;

    double temp = ((P3 * zsq + P2) * zsq + P1) * zsq + P0;
    temp = temp / (((zsq + Q2) * zsq + Q1) * zsq + Q0);
    temp = temp * z + exp * LOG2;
    return temp;
}

// https://en.cppreference.com/c/numeric/math/log10
export template <Meta::Float T>
constexpr T log10(T arg) {
    dispatchConstexpr$(log10, arg);

    if (arg <= 0)
        return NAN;
    return log(arg) * LN10O1;
}

export template <Meta::Float T>
constexpr T log2(T arg) {
    if (arg <= 0)
        return NAN;
    return log(arg) * LOG2O1;
}

// MARK: Power functions -------------------------------------------------------

// https://en.cppreference.com/c/numeric/math/sqrt
export template <Meta::Float T>
constexpr T sqrt(T arg) {
    dispatchConstexpr$(sqrt, arg);

    int exp;

    if (arg <= 0) {
        if (arg < 0)
            return NAN;
        return 0;
    }
    if (isInf(arg, 1))
        return arg;
    double x = frexp(arg, &exp);
    while (x < 0.5) {
        x *= 2;
        exp--;
    }
    /*
     * NOTE
     * this wont work on 1's comp
     */
    if (exp & 1) {
        x *= 2;
        exp--;
    }
    double temp = 0.5 * (1.0 + x);

    while (exp > 60) {
        temp *= (1L << 30);
        exp -= 60;
    }
    while (exp < -60) {
        temp /= (1L << 30);
        exp += 60;
    }
    if (exp >= 0)
        temp *= 1L << (exp / 2);
    else
        temp /= 1L << (-exp / 2);
    for (int i = 0; i <= 4; i++)
        temp = 0.5 * (temp + arg / temp);
    return temp;
}

export template <Meta::Float T>
constexpr T pow2(T x) {
    return x * x;
}


export template <Meta::Integral T>
constexpr T pow(T base, T exponent) {
    if (exponent < 0)
        return 0;
    if (exponent == 0)
        return 1;

    T res = 1;
    while (exponent > 0) {
        if (exponent & 1)
            res *= base;
        base *= base;
        exponent /= 2u;
    }
    return res;
}

// https://en.cppreference.com/c/numeric/math/pow
export template <Meta::Float T>
constexpr T pow(T x, T y) {
    dispatchConstexpr$(pow, x, y);

    double xy, ye;
    long i;
    int ex, ey;

    if (not isNan(x) and x == 1.0)
        return 1.0;
    if (not isNan(y) and y == 0.0)
        return 1.0;
    if (isNan(x) or isNan(y))
        return NAN;
    if (isInf(x)) {
        if (y < 0)
            return 0.0;
        return INF;
    }
    if (isInf(x, -1)) {
        if (y < 0)
            return isOdd(y) ? -0.0 : 0.0;
        if (y > 0)
            return isOdd(y) ? NEG_INF : INF;
    }
    if (isInf(y, 1)) {
        if (x == -1.0)
            return 1.0;
        else if (abs(x) < 1.0)
            return 0.0;
        else if (abs(x) > 1.0)
            return INF;
    } else if (isInf(y, -1)) {
        if (x == -1.0)
            return 1.0;
        else if (abs(x) < 1.0)
            return INF;
        else if (abs(x) > 1.0)
            return 0.0;
    }

    int flip = 0;
    if (y < 0.) {
        y = -y;
        flip = 1;
    }
    double y1 = modf(y, &ye);
    if (y1 != 0.0) {
        if (x <= 0.)
            goto zreturn;
        if (y1 > 0.5) {
            y1 -= 1.;
            ye += 1.;
        }
        xy = exp(y1 * log(x));
    } else
        xy = 1.0;
    if (ye > 0x7FFFFFFF) { /* should be ~0UL but compiler can't convert double to ulong */
        if (x <= 0.) {
        zreturn:
            if (x == 0. and not flip)
                return 0.;
            return NAN;
        }
        if (flip) {
            if (y == .5)
                return 1. / sqrt(x);
            y = -y;
        } else if (y == .5)
            return sqrt(x);
        return exp(y * log(x));
    }
    x = frexp(x, &ex);
    ey = 0;
    i = ye;
    if (i)
        for (;;) {
            if (i & 1) {
                xy *= x;
                ey += ex;
            }
            i >>= 1;
            if (i == 0)
                break;
            x *= x;
            ex <<= 1;
            if (x < .5) {
                x += x;
                ex -= 1;
            }
        }
    if (flip) {
        xy = 1. / xy;
        ey = -ey;
    }
    return ldexp(xy, ey);
}

// https://en.cppreference.com/c/numeric/math/hypot
export template <Meta::Float T>
constexpr T hypot(T x, T y) {
    dispatchConstexpr$(hypot, x, y);

    /* ±Inf wins even over NaN */
    if (isInf(x) or isInf(y))
        return INF;
    if (isNan(x) or isNan(y))
        return NAN;

    T p = abs(x);
    T q = abs(y);
    if (p < q) {
        T t = p;
        p = q;
        q = t;
    }
    if (p == 0)
        return 0;

    /*
     * Moler-Morrison iteration: converges cubically to √(p² + q²)
     * without ever squaring p or q, so it overflows only if the result does.
     * See Cleve Moler and Donald Morrison,
     * ``Replacing Square Roots by Pythagorean Sums,''
     * IBM Journal of Research and Development,
     * Vol. 27, Number 6, pp. 577-581, Nov. 1983
     */
    T pfac = p;
    T r = q = q / p;
    p = 1;
    for (;;) {
        r *= r;
        T s = r + 4;
        if (s == 4)
            return p * pfac;
        r /= s;
        p += 2 * r * p;
        q *= r;
        r = q / p;
    }
}

// MARK: Trigonometric functions -----------------------------------------------

template <typename T>
constexpr T _sin(T x, int quad) {
    // Coefficients are #3370 from Hart & Cheney (18.80D).
    static constexpr auto P0 = .1357884097877375669092680e8;
    static constexpr auto P1 = -.4942908100902844161158627e7;
    static constexpr auto P2 = .4401030535375266501944918e6;
    static constexpr auto P3 = -.1384727249982452873054457e5;
    static constexpr auto P4 = .1459688406665768722226959e3;
    static constexpr auto Q0 = .8644558652922534429915149e7;
    static constexpr auto Q1 = .4081792252343299749395779e6;
    static constexpr auto Q2 = .9463096101538208180571257e4;
    static constexpr auto Q3 = .1326534908786136358911494e3;

    T e, f, ysq, y, temp1, temp2;
    int k;

    if (x < 0) {
        x = -x;
        quad += 2;
    }
    x *= 1 / HALF_PI; /* underflow? */
    if (x > 32764) {
        y = modf(x, &e);
        e += quad;
        modf(0.25 * e, &f);
        quad = e - 4 * f;
    } else {
        k = x;
        y = x - k;
        quad += k;
        quad &= 3;
    }
    if (quad & 1)
        y = 1 - y;
    if (quad > 1)
        y = -y;

    ysq = y * y;
    temp1 = ((((P4 * ysq + P3) * ysq + P2) * ysq + P1) * ysq + P0) * y;
    temp2 = ((((ysq + Q3) * ysq + Q2) * ysq + Q1) * ysq + Q0);
    return temp1 / temp2;
}

// https://en.cppreference.com/c/numeric/math/cos
export template <Meta::Float T>
constexpr T cos(T x) {
    dispatchConstexpr$(cos, x);
    return _sin(x < 0 ? -x : x, 1);
}

// https://en.cppreference.com/c/numeric/math/sin
export template <Meta::Float T>
constexpr T sin(T x) {
    dispatchConstexpr$(sin, x);
    return _sin(x, 0);
}

// https://en.cppreference.com/c/numeric/math/tan
export template <Meta::Float T>
constexpr T tan(T arg) {
    dispatchConstexpr$(tan, arg);

    static constexpr double P0 = -0.1306820264754825668269611177e+5;
    static constexpr double P1 = 0.1055970901714953193602353981e+4;
    static constexpr double P2 = -0.1550685653483266376941705728e+2;
    static constexpr double P3 = 0.3422554387241003435328470489e-1;
    static constexpr double P4 = 0.3386638642677172096076369e-4;
    static constexpr double Q0 = -0.1663895238947119001851464661e+5;
    static constexpr double Q1 = 0.4765751362916483698926655581e+4;
    static constexpr double Q2 = -0.1555033164031709966900124574e+3;

    int flag = 0;
    int sign = 0;
    if (arg < 0) {
        arg = -arg;
        sign++;
    }
    arg = 2 * arg / HALF_PI; /* overflow? */
    double e;
    double x = modf(arg, &e);
    int i = e;
    switch (i % 4) {
    case 1:
        x = 1 - x;
        flag = 1;
        break;

    case 2:
        sign = not sign;
        flag = 1;
        break;

    case 3:
        x = 1 - x;
        sign = not sign;
        break;

    case 0:
        break;
    }

    double xsq = x * x;
    double temp = ((((P4 * xsq + P3) * xsq + P2) * xsq + P1) * xsq + P0) * x;
    temp = temp / (((xsq + Q2) * xsq + Q1) * xsq + Q0);

    if (flag) {
        if (temp == 0)
            return NAN;
        temp = 1 / temp;
    }
    if (sign)
        temp = -temp;
    return temp;
}

export template <Meta::Float T>
constexpr T cot(T arg) {
    return T(1.) / tan(arg);
}

// https://en.cppreference.com/c/numeric/math/atan
export template <Meta::Float T>
constexpr T atan(T arg) {
    dispatchConstexpr$(atan, arg);

    static constexpr auto P4 = .161536412982230228262e2;
    static constexpr auto P3 = .26842548195503973794141e3;
    static constexpr auto P2 = .11530293515404850115428136e4;
    static constexpr auto P1 = .178040631643319697105464587e4;
    static constexpr auto P0 = .89678597403663861959987488e3;
    static constexpr auto Q4 = .5895697050844462222791e2;
    static constexpr auto Q3 = .536265374031215315104235e3;
    static constexpr auto Q2 = .16667838148816337184521798e4;
    static constexpr auto Q1 = .207933497444540981287275926e4;
    static constexpr auto Q0 = .89678597403663861962481162e3;

    auto xatan = [](auto arg) {
        double argsq = arg * arg;
        double value = ((((P4 * argsq + P3) * argsq + P2) * argsq + P1) * argsq + P0);
        value = value / (((((argsq + Q4) * argsq + Q3) * argsq + Q2) * argsq + Q1) * argsq + Q0);
        return value * arg;
    };

    auto satan = [&](auto arg) {
        if (arg < SQ2M1)
            return xatan(arg);
        if (arg > SQ2P1)
            return HALF_PI - xatan(1 / arg);
        return HALF_PI / 2 + xatan((arg - 1) / (arg + 1));
    };

    if (arg > 0)
        return satan(arg);
    return -satan(-arg);
}

// https://en.cppreference.com/c/numeric/math/atan2
export template <Meta::Float T>
constexpr T atan2(T y, T x) {
    dispatchConstexpr$(atan2, y, x);

    if (isNan(y) or isNan(x))
        return NAN;

    /* the result takes the sign of y, including when y is -0 */
    bool neg = FloatBits<T>::from(y).sign;

    if (isInf(y)) {
        T r = isInf(x) ? (x > 0 ? PI / 4 : 3 * PI / 4) : HALF_PI;
        return neg ? -r : r;
    }

    if (y == 0 or isInf(x)) {
        /* result is ±0 when x is +0 or positive, ±π when x is -0 or negative */
        T r = FloatBits<T>::from(x).sign ? PI : 0;
        return neg ? -r : r;
    }

    if (x + y == y) /* x is negligible next to y, includes x = ±0 */
        return neg ? -HALF_PI : HALF_PI;

    T r = atan(y / x);
    if (x < 0)
        return neg ? r - PI : r + PI;
    return r;
}

// https://en.cppreference.com/c/numeric/math/asin
export template <Meta::Float T>
constexpr T asin(T arg) {
    dispatchConstexpr$(asin, arg);

    int sign = 0;
    if (arg < 0) {
        arg = -arg;
        sign++;
    }
    if (arg > 1)
        return NAN;
    double temp = sqrt(1 - arg * arg);
    if (arg > 0.7)
        temp = HALF_PI - atan(temp / arg);
    else
        temp = atan(arg / temp);
    if (sign)
        temp = -temp;
    return temp;
}

// https://en.cppreference.com/c/numeric/math/acos
export template <Meta::Float T>
constexpr T acos(T arg) {
    dispatchConstexpr$(acos, arg);

    if (arg > 1 or arg < -1)
        return NAN;
    return HALF_PI - asin(arg);
}

// MARK: Hyperbolic functions --------------------------------------------------

// https://en.cppreference.com/c/numeric/math/sinh
export template <Meta::Float T>
constexpr T sinh(T arg) {
    dispatchConstexpr$(sinh, arg);

    static constexpr double P0 = -0.6307673640497716991184787251e+6;
    static constexpr double P1 = -0.8991272022039509355398013511e+5;
    static constexpr double P2 = -0.2894211355989563807284660366e+4;
    static constexpr double P3 = -0.2630563213397497062819489e+2;
    static constexpr double Q0 = -0.6307673640497716991212077277e+6;
    static constexpr double Q1 = 0.1521517378790019070696485176e+5;
    static constexpr double Q2 = -0.173678953558233699533450911e+3;

    double temp;
    int sign = 0;
    Defer _ = [&] {
        if (sign)
            temp = -temp;
    };

    if (arg < 0) {
        arg = -arg;
        sign++;
    }
    if (arg > 21) {
        temp = exp(arg) / 2;
        return temp;
    }
    if (arg > 0.5) {
        temp = (exp(arg) - exp(-arg)) / 2;
        return temp;
    }
    double argsq = arg * arg;
    temp = (((P3 * argsq + P2) * argsq + P1) * argsq + P0) * arg;
    temp /= (((argsq + Q2) * argsq + Q1) * argsq + Q0);

    return temp;
}

// https://en.cppreference.com/c/numeric/math/cosh
export template <Meta::Float T>
constexpr T cosh(T arg) {
    dispatchConstexpr$(cosh, arg);

    if (arg < 0)
        arg = -arg;
    if (arg > 21)
        return exp(arg) / 2;
    return (exp(arg) + exp(-arg)) / 2;
}

// https://en.cppreference.com/c/numeric/math/tanh
export template <Meta::Float T>
constexpr T tanh(T arg) {
    dispatchConstexpr$(tanh, arg);

    if (arg < 0) {
        arg = -arg;
        if (arg > 21)
            return -1;
        return -sinh(arg) / cosh(arg);
    }
    if (arg > 21)
        return 1;
    return sinh(arg) / cosh(arg);
}

#endif

} // namespace Karm::Math
