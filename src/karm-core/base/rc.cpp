module;

#include <karm/macros>

export module Karm.Core:base.rc;

import :base.cursor;
import :base.manual;
import :base.atomic;
import :base.opt;
import :meta.traits;

namespace Karm {

/// A reference-counted object heap cell.
export template <typename I>
struct _Cell {
    I _strong = 0;
    I _weak = 0;

    virtual ~_Cell() = default;

    virtual void* _unwrap() lifetimebound = 0;

    virtual void clear() = 0;

    virtual Meta::Id id() = 0;

    void collectAndRelease() {
        if (_strong == 0 and _weak == 0)
            delete this;
    }

    _Cell* refStrong() lifetimebound {
        auto v = ++_strong;
        if (v < 0) [[unlikely]]
            panic("refStrong() overflow");

        return this;
    }

    void derefStrong() {
        auto v = --_strong;
        if (v == 0)
            clear();

        if (v < 0) [[unlikely]]
            panic("derefStrong() underflow");

        collectAndRelease();
    }

    _Cell* refWeak() lifetimebound {
        auto v = ++_weak;
        if (v < 0) [[unlikely]]
            panic("refWeak() overflow");

        return this;
    }

    void derefWeak() {
        auto v = --_weak;
        if (v < 0) [[unlikely]]
            panic("derefWeak() underflow");

        collectAndRelease();
    }

    template <typename T>
    T& unwrap() lifetimebound {
        return *static_cast<T*>(_unwrap());
    }
};

export template <typename I, typename T>
struct Cell : _Cell<I> {
    Manual<T> _buf{};

    template <typename... Args>
    Cell(Args&&... args) {
        _buf.ctor(std::forward<Args>(args)...);
    }

    void* _unwrap() lifetimebound override {
        return &_buf.unwrap();
    }

    Meta::Id id() override {
        return Meta::idOf<T>();
    }

    void clear() override {
        _buf.dtor();
    }
};

/// A strong reference to an object of type  `T`.
///
/// A strong reference keeps the object alive as long as the
/// reference is in scope. When the reference goes out of scope
/// the object is deallocated if there are no other strong
/// references to it.
export template <typename I, typename T>
struct _Rc {
    _Cell<I>* _cell{};

    // MARK: Rule of Five ------------------------------------------------------

    /// Get a strong reference back from a raw pointer.
    static _Rc fromRaw(T* ptr) {
        using CellType = Cell<I, T>;
        usize offset = offsetof(CellType, _buf);
        auto cell = reinterpret_cast<_Cell<I>*>(reinterpret_cast<u8*>(ptr) - offset);
        return _Rc(MOVE, cell);
    }

    static _Rc fromRef(T& ref) {
        return fromRaw(&ref);
    }

    constexpr _Rc() = delete;

    constexpr _Rc(Move, _Cell<I>* ptr)
        : _cell(ptr->refStrong()) {
    }

    constexpr _Rc(_Rc const& other)
        : _cell(other._cell) {
        if (not _cell)
            panic("null rc copy");
        _cell->refStrong();
    }

    constexpr _Rc(_Rc&& other)
        : _cell(std::exchange(other._cell, nullptr)) {
    }

    template <Meta::Derive<T> U>
    constexpr _Rc(_Rc<I, U> const& other)
        : _cell(other._cell->refStrong()) {
    }

    template <Meta::Derive<T> U>
    constexpr _Rc(_Rc<I, U>&& other)
        : _cell(std::exchange(other._cell, nullptr)) {
    }

    constexpr ~_Rc() {
        if (_cell) {
            _cell->derefStrong();
            _cell = nullptr;
        }
    }

    constexpr _Rc& operator=(_Rc const& other) {
        *this = _Rc(other);
        return *this;
    }

    constexpr _Rc& operator=(_Rc&& other) {
        std::swap(_cell, other._cell);
        return *this;
    }

    constexpr bool sameInstance(_Rc const& other) const {
        return &unwrap() == &other.unwrap();
    }

    // MARK: Operators ---------------------------------------------------------

    constexpr T const* operator->() const lifetimebound {
        return &unwrap();
    }

    constexpr T* operator->() lifetimebound {
        return &unwrap();
    }

    constexpr T const& operator*() const lifetimebound {
        return unwrap();
    }

    constexpr T& operator*() lifetimebound {
        return unwrap();
    }

    // MARK: Methods -----------------------------------------------------------

    /// Returns the number of strong references to the object.
    constexpr usize strong() const {
        return _cell ? _cell->_strong : 0;
    }

    /// Returns the number of weak references to the object.
    constexpr usize weak() const {
        return _cell ? _cell->_weak : 0;
    }

    /// Returns the total number of references to the object.
    constexpr usize refs() const {
        return strong() + weak();
    }

    constexpr void ensure() const {
        if (not _cell) [[unlikely]]
            panic("null dereference");
    }

    constexpr T const& unwrap() const lifetimebound {
        ensure();
        return _cell->template unwrap<T>();
    }

    constexpr T& unwrap() lifetimebound {
        ensure();
        return _cell->template unwrap<T>();
    }

    template <Meta::Derive<T> U>
    constexpr U const& unwrap() const lifetimebound {
        ensure();
        if (not is<U>()) [[unlikely]]
            panic("unwrapping T as U");

        return _cell->template unwrap<U>();
    }

    template <Meta::Derive<T> U>
    constexpr U& unwrap() lifetimebound {
        ensure();
        if (not is<U>()) [[unlikely]]
            panic("unwrapping T as U");

        return _cell->template unwrap<U>();
    }

    template <typename U>
    constexpr MutCursor<U> is() {
        if (not _cell)
            return nullptr;

        if (not Meta::Same<T, U> and
            not Meta::Derive<T, U> and
            not(_cell->id() == Meta::idOf<U>())) {
            return nullptr;
        }

        return &_cell->template unwrap<U>();
    }

    template <typename U>
    constexpr Cursor<U> is() const {
        if (not _cell)
            return nullptr;

        if (not Meta::Same<T, U> and
            not Meta::Derive<T, U> and
            not(_cell->id() == Meta::idOf<U>())) {
            return nullptr;
        }

        return &_cell->template unwrap<U>();
    }

    Meta::Id id() const {
        ensure();
        return _cell->id();
    }

    void hash(Meta::Derive<Hasher> auto& h) const {
        Karm::hash(h, unwrap());
    }

    template <typename U>
    constexpr Opt<_Rc<I, U>> cast() {
        if (not is<U>())
            return NONE;
        return _Rc<I, U>(MOVE, _cell);
    }

    template <typename U>
    constexpr Opt<_Rc<I, U>> cast() const {
        if (not is<U>())
            return NONE;
        return _Rc<I, U>(MOVE, _cell);
    }

    template <typename UI, Meta::Comparable<T> U>
    auto operator<=>(_Rc<UI, U> const& other) const
        requires Meta::Comparable<T>
    {
        if (_cell == other._cell)
            return _cell <=> other._cell;
        return unwrap() <=> other.unwrap();
    }

    template <typename UI, Meta::Equatable<T> U>
    bool operator==(_Rc<UI, U> const& other) const
        requires Meta::Equatable<T>
    {
        if (_cell == other._cell)
            return true;
        return unwrap() == other.unwrap();
    }

    auto operator<=>(Meta::Comparable<T> auto const& other) const {
        return unwrap() <=> other;
    }

    bool operator==(Meta::Equatable<T> auto const& other) const {
        return unwrap() == other;
    }
};

/// A weak reference to a an object of type `T`.
///
/// A weak reference does not keep the object alive, but can be
/// upgraded to a strong reference if the object is still alive.
template <typename I, typename T>
struct _Weak {
    _Cell<I>* _cell;
    
    static _Weak fromRaw(T* ptr) {
        using CellType = Cell<I, T>;
        usize offset = offsetof(CellType, _buf);
        auto cell = reinterpret_cast<_Cell<I>*>(reinterpret_cast<u8*>(ptr) - offset);
        return _Weak(MOVE, cell);
    }

    static _Weak fromRef(T& ref) {
        return fromRaw(&ref);
    }

    constexpr _Weak() = delete;

    template <Meta::Derive<T> U>
    constexpr _Weak(_Rc<I, U> const& other)
        : _cell(other._cell->refWeak()) {}

    template <Meta::Derive<T> U>
    constexpr _Weak(_Weak<I, U> const& other)
        : _cell(other._cell->refWeak()) {}

    template <Meta::Derive<T> U>
    constexpr _Weak(_Weak<I, U>&& other)
        : _cell(std::exchange(other._cell, nullptr)) {
    }

    constexpr _Weak(Move, _Cell<I>* ptr)
        : _cell(ptr->refWeak()) {
    }

    constexpr _Weak& operator=(_Rc<I, T> const& other) {
        *this = _Weak(other);
        return *this;
    }

    constexpr _Weak& operator=(_Weak const& other) {
        *this = _Weak(other);
        return *this;
    }

    constexpr _Weak& operator=(_Weak&& other) {
        std::swap(_cell, other._cell);
        return *this;
    }

    constexpr ~_Weak() {
        if (_cell) {
            _cell->derefWeak();
            _cell = nullptr;
        }
    }

    /// Upgrades the weak reference to a strong reference.
    ///
    /// Returns `NONE` if the object has been deallocated.
    Opt<_Rc<I, T>> upgrade() const {
        if (not _cell or _cell->_strong == 0)
            return NONE;
        return _Rc<I, T>(MOVE, _cell);
    }
};

export template <typename T>
using Rc = _Rc<i32, T>;

export template <typename T>
using Weak = _Weak<i32, T>;

/// Allocates an object of type `T` on the heap and returns
/// a strong reference to it.
export template <typename T, typename... Args>
constexpr Rc<T> makeRc(Args&&... args)
    requires Meta::Constructible<T, Args...>
{
    return {MOVE, new Cell<i32, T>(std::forward<Args>(args)...)};
}

export template <typename T>
constexpr Rc<T> makeRc(T&& value) {
    return {MOVE, new Cell<i32, T>(std::forward<T>(value))};
}

export template <typename T>
using Arc = _Rc<Atomic<i32>, T>;

export template <typename T>
using Aweak = _Weak<Atomic<i32>, T>;

export template <typename T, typename... Args>
constexpr Arc<T> makeArc(Args&&... args)
    requires Meta::Constructible<T, Args...>
{
    return {MOVE, new Cell<Atomic<i32>, T>(std::forward<Args>(args)...)};
}

export template <typename T>
constexpr Arc<T> makeArc(T&& value) {
    return {MOVE, new Cell<Atomic<i32>, T>(std::forward<T>(value))};
}

export template <typename I, typename T>
struct Niche<_Rc<I, T>> {
    struct Content {
        void* ptr;

        constexpr Content() : ptr(nullptr) {}

        constexpr bool has() const {
            return ptr != nullptr;
        }
    };
};

} // namespace Karm
