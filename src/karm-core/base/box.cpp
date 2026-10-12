module;

#include <karm/macros>

export module Karm.Core:base.box;

import :meta.traits;
import :base.manual;
import :base.opt;
import :base.slice;

namespace Karm {

template <typename T, typename U>
concept DeleterFor = Meta::Callable<T, U*>;

template <typename T>
struct DeleteDeleter {
    always_inline void operator()(T* p) const {
        delete p;
    };
};

static_assert(DeleterFor<DeleteDeleter<void>, void>);

export template <typename T, DeleterFor<T> D = DeleteDeleter<T>>
struct Box {
    T* _ptr{};
    [[no_unique_address]] D _deleter{};

    constexpr Box() = delete;

    constexpr Box(Move, T* ptr)
        : _ptr(ptr) {}

    constexpr Box(T const& v)
        : _ptr(new T(v)) {}

    constexpr Box(T&& v)
        : _ptr(new T(std::move(v))) {}

    constexpr Box(Box const& other)
        : _ptr(new T(*other._ptr)) {}

    template <Meta::Derive<T> U>
    constexpr Box(Box<U>&& other)
        : _ptr(std::exchange(other._ptr, nullptr)) {}

    constexpr ~Box() {
        if (_ptr)
            _deleter(_ptr);
        _ptr = nullptr;
    }

    constexpr Box& operator=(Box const& other) {
        *this = Box(other);
        return *this;
    }

    template <Meta::Derive<T> U>
    constexpr Box& operator=(Box<U>&& other) {
        std::swap(_ptr, other._ptr);
        return *this;
    }

    template <typename U>
        requires Meta::Equatable<T, U>
    constexpr bool operator==(U const& other) const {
        return unwrap() == other;
    }

    template <typename U>
        requires Meta::Comparable<T, U>
    constexpr auto operator<=>(U const& other) const {
        return unwrap() <=> other;
    }

    template <typename U>
        requires Meta::Equatable<T, U>
    constexpr bool operator==(Box<U> const& other) const {
        if (_ptr == other._ptr)
            return true;
        return unwrap() == other.unwrap();
    }

    template <typename U>
        requires Meta::Comparable<T, U>
    constexpr auto operator<=>(Box<U> const& other) const {
        return unwrap() <=> other.unwrap();
    }

    constexpr T* operator->() lifetimebound {
        return &unwrap();
    }

    constexpr T& operator*() lifetimebound {
        return unwrap();
    }

    constexpr T const* operator->() const lifetimebound {
        return &unwrap();
    }

    constexpr T const& operator*() const lifetimebound {
        return unwrap();
    }

    constexpr T const& unwrap() const lifetimebound {
        if (not _ptr) [[unlikely]]
            panic("dereferencing moved from Box<T>");
        return *_ptr;
    }

    constexpr T& unwrap() lifetimebound {
        if (not _ptr) [[unlikely]]
            panic("dereferencing moved from Box<T>");
        return *_ptr;
    }

    constexpr void hash(Meta::Derive<Hasher> auto& h) const {
        Karm::hash(h, unwrap());
    }
};

// MARK: Boxed arrays ---------------------------------------------------------

export template <typename T, typename D>
struct Box<T[], D> {
    static_assert(sizeof(D) == 0, "Box<T[]> doesn't support custom deleters");
};

export template <typename T>
struct Box<T[], DeleteDeleter<T[]>> {
    using Inner = T;

    Manual<T>* _buf{};
    usize _len{};

    static Manual<T>* _alloc(usize len) {
        return len ? new Manual<T>[len] : nullptr;
    }

    static Box filled(usize len, T fill = {}) {
        Box box;
        box._buf = _alloc(len);
        box._len = len;
        for (usize i = 0; i < len; i++)
            box._buf[i].ctor(fill);
        return box;
    }

    constexpr Box() = default;

    Box(Move, T* buf, usize len)
        : _buf(reinterpret_cast<Manual<T>*>(buf)), _len(len) {}

    Box(Sliceable<T> auto const& other)
        : _buf(_alloc(other.len())), _len(other.len()) {
        for (usize i = 0; i < _len; i++)
            _buf[i].ctor(other[i]);
    }

    Box(Box const& other)
        : Box(Slice<T>{other.buf(), other.len()}) {}

    Box(Box&& other)
        : _buf(std::exchange(other._buf, nullptr)),
          _len(std::exchange(other._len, 0)) {}

    ~Box() {
        if (not _buf)
            return;
        for (usize i = 0; i < _len; i++)
            _buf[i].dtor();
        delete[] _buf;
    }

    Box& operator=(Box const& other) {
        *this = Box(other);
        return *this;
    }

    Box& operator=(Box&& other) {
        std::swap(_buf, other._buf);
        std::swap(_len, other._len);
        return *this;
    }

    T* take() {
        T* buf = this->buf();
        _buf = nullptr;
        _len = 0;
        return buf;
    }

    usize len() const {
        return _len;
    }

    T* buf() lifetimebound {
        return reinterpret_cast<T*>(_buf);
    }

    T const* buf() const lifetimebound {
        return reinterpret_cast<T const*>(_buf);
    }

    T& operator[](usize i) lifetimebound {
        return _buf[i].unwrap();
    }

    T const& operator[](usize i) const lifetimebound {
        return _buf[i].unwrap();
    }
};

// MARK: Niche ------------------------------------------------------------------

export template <typename T, typename D>
struct Niche<Box<T, D>> {
    struct Content {
        char const* ptr;

        always_inline constexpr Content() : ptr(NICHE_PTR) {}

        always_inline constexpr bool has() const {
            return ptr != NICHE_PTR;
        }
    };
};

export template <typename T, typename D = DeleteDeleter<T>, typename... Args>
    requires Meta::Constructible<T, Args...>
constexpr Box<T, D> makeBox(Args... args) {
    return {MOVE, new T(std::forward<Args>(args)...)};
}

export template <typename T, typename D = DeleteDeleter<T>>
constexpr Box<T, D> makeBox(T&& value) {
    return {MOVE, new T(std::forward<T>(value))};
}

} // namespace Karm
