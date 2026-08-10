module;

#include <karm/macros>

export module Karm.Core:base.some;

import :base.base;
import :meta.traits;
import :meta.cvrp;

namespace Karm {

export template <typename T = None>
struct Some;

export template <typename T>
struct Some {
    T _inner;

    template <typename... Args>
    always_inline constexpr Some(Args&&... args)
        : _inner(std::forward<Args>(args)...) {}

    always_inline explicit operator bool() const {
        return true;
    }

    always_inline constexpr T take() {
        return std::move(_inner);
    }

    always_inline constexpr T& unwrap() {
        return _inner;
    }

    always_inline constexpr T const& unwrap() const {
        return _inner;
    }
};

export template <typename T>
struct Some<T&> {
    T* _inner;

    always_inline constexpr Some(T& inner)
        : _inner(&inner) {}

    always_inline explicit operator bool() const {
        return true;
    }

    always_inline constexpr T& take() {
        return *_inner;
    }

    always_inline constexpr T& unwrap() {
        return *_inner;
    }

    always_inline constexpr T const& unwrap() const {
        return *_inner;
    }
};

export template <typename T>
Some(T&) -> Some<T&>;

export template <typename T>
Some(T&&) -> Some<Meta::RemoveConstVolatileRef<T>>;

} // namespace Karm
