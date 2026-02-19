module;

#include <karm/macros>

export module Karm.Core:base.opt;

import :base.std;
import :base.niche;
import :base.panic;
import :base.try_;
import :base.ok;
import :meta.callable;
import :meta.traits;

namespace Karm {

template <typename T>
struct _OptStore;

template <typename T>
struct _OptStore {
    struct _Empty {};

    union _Inner {
        _Empty _empty;
        T _value;

        constexpr _Inner() : _empty{} {}

        constexpr ~_Inner() {}
    };

    bool _present{false};
    _Inner _inner;

    always_inline constexpr _OptStore()
        : _inner{} {}

    always_inline constexpr ~_OptStore() {
        clear();
    }

    template <typename... Args>
    always_inline constexpr T& emplace(Args&&... args) {
        clear();
        std::construct_at(&_inner._value, std::forward<Args>(args)...);
        _present = true;
        return unwrap();
    }

    always_inline constexpr void clear() {
        if (_present) {
            _inner._value.~T();
            _present = false;
        }
    }

    always_inline constexpr bool has() const {
        return _present;
    }

    always_inline constexpr T& unwrap() {
        return _inner._value;
    }

    always_inline constexpr T const& unwrap() const {
        return _inner._value;
    }

    [[clang::coro_wrapper]]
    always_inline constexpr T take() {
        T v = std::move(_inner._value);
        clear();
        return v;
    }
};

template <typename T>
struct _OptStore<T&> {
    T* _value = nullptr;

    always_inline constexpr ~_OptStore() {
        clear();
    }

    always_inline constexpr T& emplace(T& v) {
        _value = &v;
        return unwrap();
    }

    always_inline constexpr void clear() {
        _value = nullptr;
    }

    always_inline constexpr bool has() const {
        return _value != nullptr;
    }

    always_inline constexpr T& unwrap() {
        return *_value;
    }

    always_inline constexpr T const& unwrap() const {
        return *_value;
    }

    always_inline constexpr T& take() {
        T& v = *_value;
        _value = nullptr;
        return v;
    }
};

export template <Nicheable T>
struct _OptStore<T> {
    union {
        T _value;
        Niche<T>::Content _content;
    };

    static_assert(sizeof(T) >= sizeof(typename Niche<T>::Content));

    always_inline constexpr _OptStore()
        : _content{} {
    }

    always_inline constexpr ~_OptStore() {
        clear();
    }

    template <typename... Args>
    always_inline constexpr T& emplace(Args&&... args) {
        clear();
        std::construct_at(&_value, std::forward<Args>(args)...);
        return unwrap();
    }

    always_inline constexpr void clear() {
        if (has()) {
            _value.~T();
            std::construct_at(&_content);
        }
    }

    always_inline constexpr bool has() const {
        return _content.has();
    }

    always_inline constexpr T& unwrap() {
        return _value;
    }

    always_inline constexpr T const& unwrap() const {
        return _value;
    }

    always_inline constexpr T take() {
        T v = std::move(_value);
        clear();
        return v;
    }
};

export template <typename T>
struct [[nodiscard]] Opt {
    using Value = Meta::RemoveRef<T>;

    _OptStore<T> _store{};

    always_inline constexpr Opt() {}

    always_inline constexpr Opt(None) {}

    template <typename U = T>
    always_inline constexpr Opt(U const& value)
        requires(
            not Meta::Same<Meta::RemoveConstVolatileRef<U>, Opt> and
            (Meta::CopyConstructible<T, U> or Meta::LvalueRef<T>)
        )
    {
        _store.emplace(value);
    }

    template <typename U = T>
    always_inline constexpr Opt(U&& value)
        requires(
            not Meta::Same<Meta::RemoveConstVolatileRef<U>, Opt> and
            (Meta::MoveConstructible<T, U> or Meta::LvalueRef<T>)
        )
    {
        _store.emplace(std::forward<U>(value));
    }

    always_inline constexpr Opt(Opt& other)
        requires(Meta::CopyConstructible<T> or Meta::LvalueRef<T>)
    {
        if (other.has())
            _store.emplace(other.unwrap());
    }

    always_inline constexpr Opt(Opt const& other)
        requires(Meta::CopyConstructible<T> or Meta::LvalueRef<T>)
    {
        if (other.has())
            _store.emplace(other.unwrap());
    }

    always_inline constexpr Opt(Opt&& other)
        requires(Meta::MoveConstructible<T> or Meta::LvalueRef<T>)
    {
        if (other.has())
            _store.emplace(other.take());
    }

    template <typename U>
    always_inline constexpr Opt(Opt<U> const& other)
        requires(Meta::CopyConstructible<T, U>)
    {
        if (other.has())
            _store.emplace(other.unwrap());
    }

    template <typename U>
        requires(Meta::MoveConstructible<T, U>)
    always_inline constexpr Opt(Opt<U>&& other) {
        if (other.has())
            _store.emplace(other.take());
    }

    always_inline constexpr ~Opt() {
        _store.clear();
    }

    always_inline constexpr Opt& operator=(None) {
        _store.clear();
        return *this;
    }

    template <typename U = T>
        requires(
            not Meta::Same<Meta::RemoveConstVolatileRef<U>, Opt> and
            (Meta::Convertible<U, T> or Meta::LvalueRef<T>)
        )
    always_inline constexpr Opt& operator=(U& value) {
        _store.emplace(value);
        return *this;
    }

    template <typename U = T>
        requires(
            not Meta::Same<Meta::RemoveConstVolatileRef<U>, Opt> and
            (Meta::Convertible<U, T> or Meta::LvalueRef<T>)
        )
    always_inline constexpr Opt& operator=(U const& value) {
        _store.emplace(value);
        return *this;
    }

    template <typename U = T>
        requires(not Meta::Same<Meta::RemoveConstVolatileRef<U>, Opt> and Meta::MoveConstructible<T, U>)
    always_inline constexpr Opt& operator=(U&& value) {
        _store.emplace(std::forward<U>(value));
        return *this;
    }

    always_inline constexpr Opt& operator=(Opt& other)
        requires(Meta::CopyConstructible<T> or Meta::LvalueRef<T>)
    {
        clear();
        if (other.has())
            _store.emplace(other.unwrap());
        return *this;
    }

    always_inline constexpr Opt& operator=(Opt const& other)
        requires(Meta::CopyConstructible<T> or Meta::LvalueRef<T>)
    {
        clear();
        if (other.has())
            _store.emplace(other.unwrap());
        return *this;
    }

    always_inline constexpr Opt& operator=(Opt&& other)
        requires(Meta::MoveConstructible<T> or Meta::LvalueRef<T>)
    {
        clear();
        if (other.has())
            _store.emplace(other.take());

        return *this;
    }

    template <typename U = T>
    always_inline constexpr Opt& operator=(Opt<U> const& other)
        requires(Meta::CopyConstructible<T, U>)
    {
        clear();
        if (other.has())
            _store.emplace(other.unwrap());
        return *this;
    }

    template <typename U>
    always_inline constexpr Opt& operator=(Opt<U>&& other)
        requires(Meta::MoveConstructible<T, U>)
    {
        clear();
        if (other.has())
            _store.emplace(other.take());

        return *this;
    }

    always_inline constexpr explicit operator bool() const {
        return _store.has();
    }

    always_inline constexpr bool has() const {
        return _store.has();
    }

    always_inline constexpr Value* operator->() lifetimebound {
        if (not _store.has()) [[unlikely]]
            panic("unwrapping None");

        return &_store.unwrap();
    }

    always_inline constexpr Value& operator*() lifetimebound {
        if (not _store.has()) [[unlikely]]
            panic("unwrapping None");

        return _store.unwrap();
    }

    always_inline constexpr Value const* operator->() const lifetimebound {
        if (not _store.has()) [[unlikely]]
            panic("unwrapping None");

        return &_store.unwrap();
    }

    always_inline constexpr Value const& operator*() const lifetimebound {
        if (not _store.has()) [[unlikely]]
            panic("unwrapping None");

        return _store.unwrap();
    }

    template <typename... Args>
    always_inline constexpr Value& emplace(Args&&... args) lifetimebound {
        return _store.emplace(std::forward<Args>(args)...);
    }

    always_inline constexpr void clear() {
        _store.clear();
    }

    always_inline constexpr None none() const {
        return NONE;
    }

    always_inline constexpr Value& unwrap(char const* msg = "unwrapping none") lifetimebound {
        if (not _store.has()) [[unlikely]]
            panic(msg);
        return _store.unwrap();
    }

    always_inline constexpr Value const& unwrap(char const* msg = "unwrapping none") const lifetimebound {
        if (not _store.has()) [[unlikely]]
            panic(msg);
        return _store.unwrap();
    }

    always_inline constexpr Value unwrapOr(Value other) const {
        if (_store.has())
            return _store.unwrap();
        return other;
    }

    always_inline constexpr T unwrapOrElse(auto f) const {
        if (_store.has())
            return _store.unwrap();
        return f();
    }

    template <typename E>
    always_inline constexpr Res<T, E> okOr(E error) const {
        if (not has())
            return error;
        return Ok(unwrap());
    }

    [[clang::coro_wrapper]]
    always_inline constexpr T take(char const* msg = "unwrapping none") {
        if (not _store.has()) [[unlikely]]
            panic(msg);
        return _store.take();
    }

    always_inline constexpr auto visit(auto visitor)
        -> decltype(visitor(_store.unwrap())) {
        if (_store.has())
            return visitor(_store.unwrap());
        return visitor(NONE);
    }

    always_inline constexpr auto visit(auto visitor) const
        -> decltype(visitor(_store.unwrap())) {
        if (_store.has())
            return visitor(_store.unwrap());
        return visitor(NONE);
    }

    always_inline constexpr auto map(auto f) -> Opt<decltype(f(unwrap()))> {
        if (_store.has())
            return {f(unwrap())};
        return {NONE};
    }

    // call operator
    template <typename... Args>
    always_inline constexpr auto operator()(Args&&... args) {
        using OptRet = Opt<Meta::Ret<T, Args...>>;

        if constexpr (Meta::Same<void, Meta::Ret<T, Args...>>) {
            // Handle void return type
            if (not _store.has()) {
                return false;
            }
            unwrap()(std::forward<Args>(args)...);
            return true;
        } else {
            // Handle non-void return type
            if (not _store.has()) {
                return OptRet{NONE};
            }

            return OptRet{unwrap()(std::forward<Args>(args)...)};
        }
    }

    // call operator
    template <typename... Args>
    always_inline constexpr auto operator()(Args&&... args) const {
        using OptRet = Opt<Meta::Ret<T, Args...>>;

        if constexpr (Meta::Same<void, Meta::Ret<T, Args...>>) {
            // Handle void return type
            if (not _store.has()) {
                return false;
            }
            unwrap()(std::forward<Args>(args)...);
            return true;
        } else {
            // Handle non-void return type
            if (not _store.has()) {
                return OptRet{NONE};
            }

            return OptRet{unwrap()(std::forward<Args>(args)...)};
        }
    }

    always_inline constexpr bool operator==(None) const {
        return not _store.has();
    }

    template <typename U>
        requires Meta::Equatable<T, U>
    always_inline constexpr bool operator==(U const& other) const {
        if (_store.has())
            return _store.unwrap() == other;
        return false;
    }

    template <typename U>
        requires Meta::Comparable<T, U>
    always_inline constexpr std::partial_ordering operator<=>(U const& other) const {
        if (_store.has())
            return _store.unwrap() <=> other;
        return std::partial_ordering::unordered;
    }

    always_inline constexpr bool operator==(Opt const& other) const {
        if constexpr (Meta::Equatable<T>)
            if (has() and other.has())
                return unwrap() == other.unwrap();
        return not has() and not other.has();
    }

    always_inline constexpr std::partial_ordering operator<=>(Opt const& other) const {
        if constexpr (Meta::Comparable<T>)
            if (has() and other.has())
                return unwrap() <=> other.unwrap();
        return std::partial_ordering::unordered;
    }

    u64 hash() const {
        if (has())
            return hash(hash(true), unwrap());
        return hash(false);
    }
};

export template <typename T>
Opt(T) -> Opt<T>;

} // namespace Karm
