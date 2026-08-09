module;

#include <karm/macros>

export module Karm.Signals;

import Karm.Core;

namespace Karm::Signals {

// MARK: Graph -----------------------------------------------------------------

export struct Node;

struct Context {
    Node* observer = nullptr; //< Sink currently being evaluated, if any.
    usize batchDepth = 0;
    bool flushing = false;
    Vec<Node*> pending{}; //< Reactions waiting to run.
};

Context _ctx{};

struct Observing : Meta::Pinned {
    Node* _prev;

    always_inline Observing(Node* s) : _prev(_ctx.observer) {
        _ctx.observer = s;
    }

    always_inline ~Observing() {
        _ctx.observer = _prev;
    }
};

export decltype(auto) untracked(auto f) {
    Observing observing{nullptr};
    return f();
}

export struct Node {
    enum struct State {
        CLEAN, //< Up to date.
        CHECK, //< An ancestor changed, might be stale.
        DIRTY, //< A direct source changed, definitely stale.
    };

    using enum State;

    State _state = State::DIRTY;
    Vec<Node*> _observers{};
    Vec<Rc<Node>> _sources{};
    bool _queued = false;

    virtual ~Node() {
        unschedule();
        unlinkSources();
    }

    virtual void recompute() {}

    virtual bool reaction() {
        return false;
    }

    void schedule() {
        if (reaction() and not _queued) {
            _queued = true;
            _ctx.pending.pushBack(this);
        }

        for (auto* o : _observers) {
            if (o->_state != State::CLEAN)
                continue;
            o->_state = State::CHECK;
            o->schedule();
        }
    }

    void unschedule() {
        for (auto& p : _ctx.pending)
            if (p == this)
                p = nullptr;
        _queued = false;
    }

    void update() {
        if (_state == CHECK) {
            for (auto& s : _sources) {
                s->update();
                if (_state == DIRTY)
                    break; // That source's new value dirtied us.
            }

            if (_state == CHECK)
                _state = CLEAN;
        }

        if (_state == DIRTY)
            recompute();
    }

    auto observe(auto f) {
        unlinkSources();
        Defer clean{[&] {
            _state = State::CLEAN;
        }};
        Observing observing{this};
        return f();
    }

    void unlinkSources() {
        for (auto& s : _sources)
            s->_observers.removeAll(this);
        _sources.clear();
    }

    void notify() {
        for (auto* o : _observers) {
            if (o->_state == State::DIRTY)
                continue;

            o->_state = State::DIRTY;
            o->schedule();
        }
    }

    void stop() {
        unlinkSources();
        unschedule();
        _state = State::CLEAN;
    }
};

export void trackDependency(Rc<Node> src) {
    auto* obs = _ctx.observer;
    if (not obs)
        return;

    for (auto& s : obs->_sources)
        if (&*s == &*src)
            return;

    src->_observers.pushBack(obs);
    obs->_sources.pushBack(std::move(src));
}

template <typename T>
bool diff(T const& lhs, T const& rhs) {
    if constexpr (Meta::Equatable<T, T>)
        return lhs == rhs;
    else
        return false; // No cheap comparison, assume it changed.
}

// MARK: Reaction --------------------------------------------------------------

export void flush() {
    if (_ctx.batchDepth > 0 or _ctx.flushing)
        return;

    _ctx.flushing = true;
    Defer defer{[] {
        _ctx.pending.clear();
        _ctx.flushing = false;
    }};

    for (usize i = 0; i < _ctx.pending.len(); i++) {
        auto* r = _ctx.pending[i];
        if (not r)
            continue;

        r->_queued = false;
        r->update();
    }
}

export void batch(auto f) {
    _ctx.batchDepth++;
    Defer defer{[] {
        _ctx.batchDepth--;
        flush();
    }};
    f();
}

// MARK: Signal ----------------------------------------------------------------

export template <typename T>
struct Signal {
    struct Cell : Node {
        T _value;

        Cell(T value) : _value(std::move(value)) {}
    };

    mutable Rc<Cell> _state;

    Signal(T value = {}) : _state(makeRc<Cell>(std::move(value))) {}

    T const& value() const lifetimebound {
        trackDependency(_state);
        return _state->_value;
    }

    T const& peek() const lifetimebound {
        return _state->_value;
    }

    void update(T value) {
        auto& cell = *_state;
        if (diff(cell._value, value))
            return; //< Nothing downstream needs to know.

        cell._value = std::move(value);
        cell.notify();
        flush();
    }

    void mutate(auto f) {
        update(f(peek()));
    }

    T const& operator*() const lifetimebound {
        return value();
    }
};

// MARK: Computed --------------------------------------------------------------

export template <typename T>
struct Computed {
    struct Cell : Node {
        Func<T()> _f;
        Opt<T> _value = NONE;

        Cell(Func<T()> f) : _f(std::move(f)) {}

        void recompute() override {
            T next = observe([&] {
                return _f();
            });

            bool changed = not _value.has() or not diff(_value.unwrap(), next);
            _value = Some(std::move(next));

            if (changed)
                notify();
        }
    };

    mutable Rc<Cell> _state;

    template <Meta::Callable F>
    Computed(F f) : _state(makeRc<Cell>(std::move(f))) {}

    T const& value() const lifetimebound {
        _state->update();
        trackDependency(_state);
        return _state->_value.unwrap();
    }

    T const& peek() const lifetimebound {
        _state->update();
        return _state->_value.unwrap();
    }

    T const& operator*() const lifetimebound {
        return value();
    }
};

export template <Meta::Callable F>
Computed(F) -> Computed<Meta::Ret<F>>;

// MARK: Effect ----------------------------------------------------------------

export struct Effect {
    struct Cell : Node {
        Func<void()> _f;

        Cell(Func<void()> f) : _f(std::move(f)) {}

        void recompute() override {
            observe([&] {
                _f();
            });
        }

        bool reaction() override {
            return true;
        }
    };

    mutable Rc<Cell> _state;

    template <Meta::Callable F>
    Effect(F f) : _state(makeRc<Cell>(std::move(f))) {
        _state->update(); // Run once to discover what it depends on.
    }

    void stop() {
        _state->stop();
    }
};

} // namespace Karm::Signals
