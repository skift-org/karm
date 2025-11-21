module;

#include <karm-core/macros.h>

export module Karm.Core:base.res;

import :base.error;
import :base.union_;
import :base.opt;
import :meta.traits;

namespace Karm {

export template <typename T = None>
struct Ok;

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

export template <typename V = None, typename E = Error>
struct [[nodiscard]] Res {
    using Inner = Union<Ok<V>, E>;
    using Value = Meta::RemoveConstVolatileRef<V>;

    Inner _inner;

    always_inline constexpr Res(Ok<V> const& ok)
        : _inner(ok) {}

    always_inline constexpr Res(Ok<V>&& ok)
        : _inner(std::move(ok)) {}

    template <typename U>
    always_inline constexpr Res(Ok<U> ok)
        : _inner(Ok<V>{ok.unwrap()}) {}

    always_inline constexpr Res(E err)
        : _inner(err) {}

    always_inline constexpr Res(None)
        : _inner(Error::other("unexpected none")) {}

    template <typename U>
    always_inline constexpr Res(Res<U, E> other)
        : _inner(other ? Inner{Ok<V>{other.unwrap()}} : Inner{other.none()}) {}

    always_inline constexpr explicit operator bool() const {
        return _inner.template is<Ok<V>>();
    }

    always_inline constexpr Opt<V> ok() const {
        if (_inner.template is<E>()) [[unlikely]]
            return NONE;
        return _inner.template unwrap<Ok<V>>().unwrap();
    }

    always_inline constexpr Opt<E> err() const {
        if (not _inner.template is<E>()) [[unlikely]]
            return NONE;
        return _inner.template unwrap<E>();
    }

    always_inline constexpr bool has() const {
        return _inner.template is<Ok<V>>();
    }

    always_inline constexpr E const& none() const lifetimebound {
        if (not _inner.template is<E>()) [[unlikely]]
            panic("none() called on an ok");

        return _inner.template unwrap<E>();
    }

    always_inline constexpr V& unwrap(char const* msg = "unwraping an error") lifetimebound {
        if (not _inner.template is<Ok<V>>()) [[unlikely]]
            panic(msg);

        return _inner.template unwrap<Ok<V>>().unwrap();
    }

    always_inline constexpr V const& unwrap(char const* msg = "unwraping an error") const lifetimebound {
        if (not _inner.template is<Ok<V>>()) [[unlikely]]
            panic(msg);
        return _inner.template unwrap<Ok<V>>().unwrap();
    }

    always_inline constexpr Value unwrapOr(Value other) const {
        if (_inner.template is<Ok<V>>())
            return _inner.template unwrap<Ok<V>>().unwrap();
        return other;
    }

    always_inline constexpr Value unwrapOrDefault(Value other) const {
        if (_inner.template is<Ok<V>>())
            return _inner.template unwrap<Ok<V>>().unwrap();
        return other;
    }

    always_inline constexpr Value unwrapOrElse(auto f) const {
        if (_inner.template is<Ok<V>>())
            return _inner.template unwrap<Ok<V>>().unwrap();
        return f();
    }

    always_inline constexpr V take(char const* msg = "take() called on an error") {
        if (not _inner.template is<Ok<V>>()) [[unlikely]]
            panic(msg);

        return _inner.template unwrap<Ok<V>>().take();
    }

    template <typename U>
    always_inline constexpr Res<V, U> mapErr(auto f) {
        if (_inner.template is<Ok<V>>())
            return _inner.template unwrap<Ok<V>>();
        return f(_inner.template unwrap<E>());
    }

    template <typename U>
    always_inline constexpr Res<V, U> mapErr() {
        if (_inner.template is<Ok<V>>())
            return _inner.template unwrap<Ok<V>>();
        return U{};
    }

    template <typename U>
    always_inline constexpr Res<U, E> map(auto f) {
        if (_inner.template is<Ok<V>>())
            return Ok(f(_inner.template unwrap<Ok<V>>().unwrap()));
        return _inner.template unwrap<E>();
    }

    template <typename U>
    always_inline constexpr Res<U, E> map() {
        if (_inner.template is<Ok<V>>())
            return Ok(_inner.template unwrap<Ok<V>>().unwrap());
        return _inner.template unwrap<E>();
    }

    always_inline auto operator<=>(Res const&) const
        requires Meta::Comparable<Inner>
    = default;

    always_inline bool operator==(bool b) const {
        return has() == b;
    }

    always_inline bool operator==(Res const&) const = default;
};

static_assert(Tryable<Res<isize, Error>>);

} // namespace Karm
