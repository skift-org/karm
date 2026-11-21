export module Karm.Ui:reducer;

import Karm.Core;
import Karm.App;
import Karm.Gfx;
import Karm.Math;

import :funcs;

namespace Karm::Ui {

// MARK: Reducer ---------------------------------------------------------------

export template <typename A>
using Task = Opt<Async::_Task<Opt<A>>>;

export template <typename S, typename A, Task<A> (*R)(S&, A) = [](S& s, A a) {
    return s.reduce(a);
}>
struct Model {
    using State = S;
    using Action = A;
    static constexpr auto reduce = R;

    template <typename X, typename... Args>
    static Send<> bind(Args... args) {
        return bindBubble<Action>(X{std::forward<Args>(args)...});
    }

    template <typename X>
    static Send<> bind(X value) {
        return bindBubble<Action>(value);
    }

    template <typename X, typename... Args>
    static Opt<Send<>> bindIf(bool cond, Args... args) {
        if (not cond)
            return NONE;
        return Some(bindBubble<Action>(X{std::forward<Args>(args)...}));
    }

    template <typename X>
    static Opt<Send<>> bindIf(bool cond, X value) {
        if (not cond)
            return NONE;
        return Some(bindBubble<Action>(value));
    }

    template <typename X>
    static void bubble(Node& n, X value = {}) {
        Ui::bubble<Action>(n, Action{std::move(value)});
    }

    template <typename X>
    static void event(Node& n, X value = {}) {
        Ui::event<Action>(n, Action{std::move(value)});
    }

    template <typename X>
    static auto map() {
        return []<typename... Args>(Node& n, Args... value) {
            Ui::bubble<Action>(n, X{std::forward<Args>(value)...});
        };
    }
};

template <typename Model>
struct Reducer :
    LeafNode<Reducer<Model>> {

    using State = Model::State;
    using Action = Model::Action;

    struct StateCell {
        State state;
        Node* owner;
    };

    Rc<StateCell> _cell;
    Func<Child(State const&)> _build;
    bool _rebuild = true;
    Opt<Child> _child;

    Reducer(State state, Func<Child(State const&)> build)
        : _cell(makeRc<StateCell>(std::move(state), this)),
          _build(std::move(build)) {}

    ~Reducer() {
        if (_cell->owner == this)
            _cell->owner = nullptr;

        if (_child)
            (*_child)->detach(this);
    }

    // MARK: Build -------------------------------------------------------------

    void rebuild() {
        auto next = _build(_cell->state);
        next->attach(this);

        if (auto& [prev] = _child) {
            next->adopt(prev);
            prev->detach(this);
        }

        _child = Some(next);
        _rebuild = false;
        event<Node::RebuiltEvent>(*this);
    }

    Node& ensure() {
        if (_rebuild or not _child)
            rebuild();
        return **_child;
    }

    void dispatch(Action const& a) {
        auto task = Model::reduce(_cell->state, a);
        if (task != NONE) {
            Async::detach(task.take(), [cell = _cell](Opt<Action> const& a) {
                if (a and cell->owner)
                    Ui::bubble<Action>(*cell->owner, *a);
            });
        }
        _rebuild = true;
        shouldLayout(*this);
    }

    // MARK: Node --------------------------------------------------------------

    void adopt(Reducer& o) override {
        _cell = o._cell;
        _cell->owner = this;

        _child = std::exchange(o._child, NONE);
        if (auto& [child] = _child)
            child->attach(this);
    }

    void paint(Gfx::Canvas& g, RectAu r) override {
        ensure().paint(g, r);
    }

    void event(App::Event& e) override {
        if (auto a = e.is<Action>()) {
            dispatch(*a);
            e.accept();
        } else if (e.is<App::FormfactorEvent>()) {
            // FIXME: Probably the state should handle theses but for now
            //        let's just rebuild for element that use App::formFactor
            e.accept();
            _rebuild = true;
            shouldLayout(*this);
        } else {
            ensure().event(e);
        }
    }

    void bubble(App::Event& e) override {
        if (auto a = e.is<Action>()) {
            dispatch(*a);
            e.accept();
        } else {
            LeafNode<Reducer<Model>>::bubble(e);
        }
    }

    void layout(RectAu r) override {
        ensure().layout(r);
    }

    Vec2Au size(Vec2Au s, Hint hint) override {
        return ensure().size(s, hint);
    }

    RectAu bound() override {
        return ensure().bound();
    }

    App::HitResult hitTest(Vec2Au p) override {
        return ensure().hitTest(p);
    }
};

export template <typename Model>
Child reducer(
    typename Model::State init,
    Func<Child(typename Model::State const&)> build
) {

    return makeRc<Reducer<Model>>(std::move(init), std::move(build));
}

export template <typename Model>
Child reducer(Func<Child(typename Model::State const&)> build) {
    return makeRc<Reducer<Model>>(typename Model::State{}, std::move(build));
}

// MARK: State -----------------------------------------------------------------

export template <typename T>
Child state(T init, auto build) {
    auto reduce = [](T& s, T v) -> Task<T> {
        s = v;
        return NONE;
    };

    using M = Model<T, T, reduce>;

    return reducer<M>(init, [build = std::move(build)](T const& state) {
        return build(state, [](T t) {
            return bindBubble<T>(t);
        });
    });
}

export template <typename T>
Child state(auto build) {
    return state<T>({}, std::move(build));
}

export template <typename T>
using Action = SharedFunc<void(Node&, T const&)>;

} // namespace Karm::Ui
