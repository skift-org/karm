export module Karm.Ui:reactive;

import Karm.Core;
import Karm.Signals;
import Karm.App;
import Karm.Gfx;
import Karm.Math;

import :funcs;
import :node;

namespace Karm::Ui {

export struct Reactive :
    LeafNode<Reactive>,
    Signals::Node {

    Slot _build;
    Opt<Child> _child;
    bool _shouldRebuild = true;

    Reactive(Func<Child()> build)
        : _build(std::move(build)) {}

    ~Reactive() override {
        if (_child)
            (*_child)->detach(this);
    }

    // MARK: Build -------------------------------------------------------------

    void rebuild() {
        _state = Signals::Node::DIRTY;

        auto next = observe([&] {
            return _build();
        });

        if (_child) {
            auto tmp = (*_child)->reconcile(std::move(next));
            if (tmp) {
                (*_child)->detach(this);
                _child = tmp;
                (*_child)->attach(this);
            }
        } else {
            _child = Some(std::move(next));
            (*_child)->attach(this);
        }

        _shouldRebuild = false;
        Ui::event<Ui::Node::RebuiltEvent>(*this);
    }

    void ensureBuild() {
        if (_shouldRebuild)
            rebuild();
    }

    // MARK: Reaction ----------------------------------------------------------

    void recompute() override {
        _shouldRebuild = true;
        shouldLayout(*this);
    }

    bool reaction() override {
        return true;
    }

    // MARK: Node --------------------------------------------------------------

    void reconcile(Reactive& o) override {
        _build = std::move(o._build);
        _shouldRebuild = true;
    }

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        ensureBuild();
        (*_child)->paint(g, r);
    }

    void event(App::Event& e) override {
        ensureBuild();
        (*_child)->event(e);
    }

    void layout(Math::Recti r) override {
        ensureBuild();
        (*_child)->layout(r);
    }

    Math::Vec2i size(Math::Vec2i s, Hint hint) override {
        ensureBuild();
        return (*_child)->size(s, hint);
    }

    Math::Recti bound() override {
        ensureBuild();
        return (*_child)->bound();
    }

    App::HitResult hitTest(Math::Vec2i p) override {
        ensureBuild();
        return (*_child)->hitTest(p);
    }
};

export Child reactive(Slot build) {
    return makeRc<Reactive>(std::move(build));
}

export template <typename S>
struct Stateful :
    LeafNode<Stateful<S>>,
    Signals::Node {

    State _data;
    SharedFunc<Child(S&)> _build;
    Opt<Child> _child;
    bool _shouldRebuild = true;

    Stateful(State data, Func<Child()> build)
        : _data(std::move(data)),
          _build(std::move(build)) {}

    ~Stateful() override {
        if (_child)
            (*_child)->detach(this);
    }

    // MARK: Build -------------------------------------------------------------

    void rebuild() {
        _state = Signals::Node::DIRTY;

        auto next = observe([&] {
            return _build(_data);
        });

        if (_child) {
            auto tmp = (*_child)->reconcile(std::move(next));
            if (tmp) {
                (*_child)->detach(this);
                _child = tmp;
                (*_child)->attach(this);
            }
        } else {
            _child = Some(std::move(next));
            (*_child)->attach(this);
        }

        _shouldRebuild = false;
        Ui::event<Ui::Node::RebuiltEvent>(*this);
    }

    void ensureBuild() {
        if (_shouldRebuild)
            rebuild();
    }

    // MARK: Reaction ----------------------------------------------------------

    void recompute() override {
        _shouldRebuild = true;
        shouldLayout(*this);
    }

    bool reaction() override {
        return true;
    }

    // MARK: Node --------------------------------------------------------------

    void reconcile(Stateful<State>& o) override {
        _build = std::move(o._build);
        _shouldRebuild = true;
    }

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        ensureBuild();
        (*_child)->paint(g, r);
    }

    void event(App::Event& e) override {
        ensureBuild();
        (*_child)->event(e);
    }

    void layout(Math::Recti r) override {
        ensureBuild();
        (*_child)->layout(r);
    }

    Math::Vec2i size(Math::Vec2i s, Hint hint) override {
        ensureBuild();
        return (*_child)->size(s, hint);
    }

    Math::Recti bound() override {
        ensureBuild();
        return (*_child)->bound();
    }

    App::HitResult hitTest(Math::Vec2i p) override {
        ensureBuild();
        return (*_child)->hitTest(p);
    }
};

export template <typename State>
Child stateful(State state, Slot build) {
    return makeRc<Stateful<State>>(std::move(state), std::move(build));
}

export template <typename T>
auto bind(Signals::Signal<T> sig) {
    return [=](auto&, auto const& value) mutable {
        sig.update(value);
    };
}

export template <typename T>
auto bind(Signals::Signal<T> sig, auto value) {
    return [=](auto&...) mutable {
        sig.update(value);
    };
}

} // namespace Karm::Ui
