module;

#include <karm/macros>

export module Karm.Core:base.ok;

import :base.base;
import :meta.traits;
import :meta.cvrp;

namespace Karm {

export template <typename T = None>
struct Ok;

export template <typename V, typename E>
struct [[nodiscard]] Res;

export template <typename T>
struct Ok {
    T _inner;

    template <typename... Args>
    always_inline constexpr Ok(Args&&... args)
        : _inner(std::forward<Args>(args)...) {}

    always_inline explicit operator bool() const {
        return true;
    }

    always_inline auto operator<=>(Ok const&) const
        requires Meta::Comparable<T>
    = default;

    always_inline bool operator==(Ok const&) const
        requires Meta::Equatable<T>
    = default;

    always_inline T take() {
        return std::move(_inner);
    }

    always_inline T& unwrap() {
        return _inner;
    }

    always_inline T const& unwrap() const {
        return _inner;
    }
};

export template <typename T>
struct Ok<T&> {
    T* _inner;

    always_inline constexpr Ok(T& inner)
        : _inner(&inner) {}

    always_inline explicit operator bool() const {
        return true;
    }

    always_inline auto operator<=>(Ok const&) const
        requires Meta::Comparable<T>
    {
        return *_inner <=> *_inner;
    }

    always_inline bool operator==(Ok const&) const
        requires Meta::Equatable<T>
    {
        return *_inner == *_inner;
    }

    always_inline T& take() {
        return *_inner;
    }

    always_inline T& unwrap() {
        return *_inner;
    }

    always_inline T const& unwrap() const {
        return *_inner;
    }
};

export template <typename T>
Ok(T&&) -> Ok<Meta::RemoveConstVolatileRef<T>>;

} // namespace Karm
