export module Karm.Core:base.cow;

import :base.rc;

namespace Karm {

export template <typename T>
struct Cow {
    Rc<T> _inner = default_();

    static Rc<T> default_() {
        static Opt<Rc<T>> _base = NONE;
        if (not _base)
            _base = makeRc<T>();
        return _base.unwrap();
    }

    Cow() = default;

    Cow(Rc<T> inner)
        : _inner(std::move(inner)) {}

    Cow(T const& inner)
        : _inner(makeRc<T>(inner)) {}

    Cow(T&& inner)
        : _inner(makeRc<T>(std::move(inner))) {}

    T& cow() {
        if (_inner.refs() > 1)
            _inner = makeRc<T>(_inner.unwrap());
        return _inner.unwrap();
    }

    bool defaulted() const {
        return &_inner.unwrap() == &default_().unwrap();
    }

    constexpr T const* operator->() const {
        return &_inner.unwrap();
    }

    constexpr T const& operator*() const {
        return _inner.unwrap();
    }

    bool sameInstance(Cow const& other) const {
        return _inner.sameInstance(other._inner);
    }

    bool operator==(Cow const& other) const {
        return _inner == other._inner;
    }

    auto operator<=>(Cow const& other) const {
        return _inner <=> other._inner;
    }

    void hash(Meta::Derive<Hasher> auto& h) const {
        Karm::hash(h, _inner);
    }
};

} // namespace Karm
