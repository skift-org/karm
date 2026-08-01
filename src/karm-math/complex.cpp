export module Karm.Math:complex;

import Karm.Core;

namespace Karm::Math {

export template <typename T>
struct Complex {
    T re{};
    T im{};

    static constexpr Complex fromAngle(T angle) {
        return {
            Math::cos(angle),
            Math::sin(angle),
        };
    }

    constexpr Complex operator+(Complex const& rhs) const {
        return {re + rhs.re, im + rhs.im};
    }

    constexpr Complex operator-(Complex const& rhs) const {
        return {re - rhs.re, im - rhs.im};
    }

    constexpr Complex operator*(Complex const& rhs) const {
        return {
            re * rhs.re - im * rhs.im,
            re * rhs.im + im * rhs.re,
        };
    }

    constexpr Complex operator*(T rhs) const {
        return {re * rhs, im * rhs};
    }

    constexpr Complex conjugate() const {
        return {re, -im};
    }

    constexpr T norm() const {
        return re * re + im * im;
    }

    constexpr T magnitude() const {
        return Math::sqrt(norm());
    }

    constexpr bool operator==(Complex const&) const = default;

    void repr(Io::Emit& e) const {
        e("{}+{}i", re, im);
    }
};

export using Complexf = Complex<f32>;
export using Complexd = Complex<f64>;

} // namespace Karm::Math
