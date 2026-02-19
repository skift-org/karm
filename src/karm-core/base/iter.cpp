export module Karm.Core:base.iter;

import :base.opt;
import :base.clamp;

namespace Karm {

export template <typename T>
concept IterItem = requires(T t) {
    *t;
    t == NONE;
    t != NONE;
};

export template <typename T>
concept Iter = requires(T t) {
    { t.next() } -> IterItem;
};

// MARK: Generators -----------------------------------------------------------

export template <typename T>
struct Single {
    Opt<T> value;

    constexpr Opt<T> next() {
        return value.take();
    }
};

export template <typename T>
struct Repeat {
    T value;
    usize count;

    constexpr T next() {
        if (count == 0)
            return NONE;
        count--;
        return value;
    }
};

// NOTE: Range is already taken so we had to be creative with the name
export template <typename T>
struct Iota {
    T start;
    T end;
    T step;

    constexpr Iota(T start, T end, T step = 1)
        : start(start), end(end), step(step) {}

    constexpr Iota(T end)
        : Iota(static_cast<T>(0), end) {}

    constexpr T next() {
        if (start >= end)
            return NONE;
        auto value = start;
        start += step;
        return value;
    }
};

// MARK: Yield -------------------------------------------------------------

export template <typename T>
struct [[nodiscard, clang::coro_return_type, clang::coro_lifetimebound]] Yield {
    struct promise_type;

    struct promise_type {
        Opt<T> _value = NONE;

        Yield get_return_object() {
            return Yield(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        std::suspend_always initial_suspend() { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() {
            panic("unhandled exception in coroutine");
        }

        template <Meta::Convertible<T> From>
        std::suspend_always yield_value(From&& from) {
            _value = std::forward<From>(from);
            return {};
        }

        void return_void() {}
    };

    std::coroutine_handle<promise_type> _coro = nullptr;
    bool _full = false;

    Yield(std::coroutine_handle<promise_type> coro)
        : _coro(coro) {}

    // Copy is deleted
    Yield(Yield const& other) = delete;

    // Move is allowed
    Yield(Yield&& other)
        : _coro(std::exchange(other._coro, nullptr)) {}

    Yield& operator=(Yield const& other) = delete;

    Yield& operator=(Yield&& other) {
        std::swap(_coro, other._coro);
        return *this;
    }

    ~Yield() {
        if (_coro)
            _coro.destroy();
    }

    void fill() {
        if (not _full) {
            _coro.resume();
            _full = true;
        }
    }

    explicit operator bool() {
        fill();
        return not _coro.done();
    }

    Opt<T> next() {
        fill();
        if (_coro.done())
            return NONE;
        _full = false;
        return _coro.promise()._value.take();
    }
};

static_assert(Iter<Yield<int>>);

// MARK: ForEach ---------------------------------------------------------------

export template <typename F>
struct ForEach {
    F f;

    constexpr void pipe(auto&& iter) {
        while (auto value = iter.next())
            f(*value);
    }
};

export template <typename F>
ForEach(F) -> ForEach<F>;

// MARK: ForEachi --------------------------------------------------------------

export template <typename F>
struct ForEachi {
    F f;

    constexpr void pipe(auto&& iter) {
        usize index = 0;
        while (auto value = iter.next())
            f(*value, index++);
    }
};

export template <typename F>
ForEachi(F) -> ForEachi<F>;

// MARK: First ------------------------------------------------------------------

export template <typename F>
struct Find {
    F f;

    constexpr auto pipe(auto&& iter) {
        while (auto value = iter.next())
            if (f(*value))
                return value;
        return decltype(iter.next()){NONE};
    }
};

export template <typename F>
Find(F) -> Find<F>;

// MARK: FindLast --------------------------------------------------------------

export template <typename F>
struct FindLast {
    F f;

    constexpr auto pipe(auto&& iter) {
        decltype(iter.next()) last = NONE;
        while (auto value = iter.next())
            if (f(*value))
                last = value;
        return last;
    }
};

export template <typename F>
FindLast(F) -> FindLast<F>;

// MARK: Select ----------------------------------------------------------------

export template <typename F>
struct Select {
    F f;

    constexpr auto pipe(auto&& iter) {
        // FIX: Remove reference to store by value (Decay)
        using I = Meta::RemoveConstVolatileRef<decltype(iter)>;

        struct Iter {
            I iter; // Stored by value
            F f;

            auto next() -> Opt<decltype(f(*iter.next()))> {
                while (auto value = iter.next())
                    return f(*value);
                return NONE;
            }
        };

        // FIX: Forward the iterator into the struct
        return Iter{std::forward<decltype(iter)>(iter), std::move(f)};
    }
};

// MARK: Selecti -------------------------------------------------------------------

export template <typename F>
struct Selecti {
    F f;

    constexpr auto pipe(auto&& iter) {
        using I = Meta::RemoveConstVolatileRef<decltype(iter)>;

        struct Iter {
            I iter;
            F f;
            usize index = 0;

            auto next() -> Opt<decltype(f(*iter.next(), index))> {
                while (auto value = iter.next())
                    return f(*value, index++);
                return NONE;
            }
        };

        return Iter{std::forward<decltype(iter)>(iter), std::move(f)};
    }
};

// MARK: Where ----------------------------------------------------------------

export template <typename F>
struct Where {
    F f;

    constexpr auto pipe(auto&& iter) {
        using I = Meta::RemoveConstVolatileRef<decltype(iter)>;

        struct Iter {
            I iter;
            F f;

            auto next() -> decltype(iter.next()) {
                while (auto value = iter.next())
                    if (f(*value))
                        return value;
                return NONE;
            }
        };

        return Iter{std::forward<decltype(iter)>(iter), std::move(f)};
    }
};

// MARK: Any -------------------------------------------------------------------

export template <typename F>
struct Any {
    F f;

    constexpr bool pipe(auto&& iter) {
        while (auto value = iter.next())
            if (f(*value))
                return true;
        return false;
    }
};

template <typename F>
Any(F) -> Any<F>;

// MARK: All -------------------------------------------------------------------

export template <typename F>
struct All {
    F f;

    constexpr bool pipe(auto&& iter) {
        while (auto value = iter.next())
            if (not f(*value))
                return false;
        return true;
    }
};

template <typename F>
All(F) -> All<F>;

// MARK: Collect ---------------------------------------------------------------

export template <typename V>
struct Collect {
    constexpr V pipe(auto&& iter) {
        V v{};
        while (auto value = iter.next())
            v.pushBack(*value);
        return v;
    }
};

// MARK: Reduce ----------------------------------------------------------------

export template <typename F>
struct Reduce {
    F f;

    constexpr auto pipe(auto&& iter) {
        auto first = iter.next();
        if (not first)
            return NONE;
        auto acc = *first;
        while (auto value = iter.next())
            acc = f(std::move(acc), *value);
        return acc;
    }
};

// MARK: Sum -------------------------------------------------------------------

export struct Sum {
    constexpr auto pipe(auto&& iter) {
        using T = Meta::RemoveConstVolatileRef<decltype(*iter.next())>;
        T sum = static_cast<T>(0);
        while (auto value = iter.next())
            sum += *value;
        return sum;
    }
};

// MARK: Max -------------------------------------------------------------------

export struct Max {
    constexpr auto pipe(auto&& iter) {
        using T = decltype(*iter.next());
        Opt<T> max;
        while (auto value = iter.next())
            if (not max or *value > *max)
                max = *value;
        return max;
    }
};

// MARK: Min -------------------------------------------------------------------

export struct Min {
    constexpr auto pipe(auto&& iter) {
        using T = decltype(*iter.next());
        Opt<T> min;
        while (auto value = iter.next())
            if (not min or *value < *min)
                min = *value;
        return min;
    }
};

// MARK: Piping ----------------------------------------------------------------

export template <typename T, typename I>
concept IterPipe = requires(T t, I i) {
    t.pipe(i);
};

export template <typename Iter, IterPipe<Iter> Pipe>
auto operator|(Iter&& i, Pipe&& p) {
    // FIX: Forward both the iterator and the pipe
    return std::forward<Pipe>(p).pipe(std::forward<Iter>(i));
}

// MARK: begin/end -------------------------------------------------------------

template <Iter I>
struct It {
    using Item = decltype(Meta::declval<I>().next());
    Item curr;
    I& iter;

    constexpr auto& operator*() {
        return *curr;
    }

    constexpr auto operator++() {
        curr = iter.next();
        return *this;
    }

    constexpr bool operator!=(None) {
        return curr != NONE;
    }
};

export template <Iter I>
constexpr It<I> begin(I& iter) {
    return {iter.next(), iter};
}

export template <Iter I>
constexpr None end(I&) {
    return NONE;
}

} // namespace Karm
