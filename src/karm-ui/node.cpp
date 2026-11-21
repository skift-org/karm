export module Karm.Ui:node;

import Karm.Core;
import Karm.App;
import Karm.Sys;
import Karm.Gfx;
import Karm.Math;

namespace Karm::Ui {

using Math::Au;
using Math::InsetsAu;
using Math::RectAu;
using Math::Vec2Au;

export enum struct Hint {
    MIN,
    PREFERRED,
    MAX,
};

export struct Node;

export using Child = Rc<Node>;
export using Children = Vec<Child>;
export using Visitor = Func<void(Node&)>;

// MARK: Node ------------------------------------------------------------------

export using Key = Opt<u64>;

export template <typename... Ts>
using Send = SharedFunc<void(Node&, Ts...)>;

export template <typename... Ts>
auto SINK(Ui::Node&, Ts&&...) {}

export template <typename... Ts>
Opt<Send<Ts...>> DISABLED = NONE;

struct Node : App::Dispatch {
    Key _key = NONE;
    bool _consumed = false;

    struct PaintEvent {
        Math::RectAu bound;
    };

    struct LayoutEvent {
    };

    struct AnimateEvent {
        f64 dt;
    };

    struct RebuiltEvent {
    };

    Key key() const {
        return _key;
    }

    virtual bool compatible(Child&) { return false; }

    virtual void adopt(Child) {}

    virtual void paint(Gfx::Canvas&, Math::RectAu) {}

    virtual void layout(Math::RectAu) {}

    virtual Math::Vec2Au size(Math::Vec2Au s, Hint) { return s; }

    virtual Math::RectAu bound() { panic("bound() not implemented"); }

    virtual App::HitResult hitTest([[maybe_unused]] Math::Vec2Au p) {
        return App::HitResult::NORMAL;
    }

    virtual Node* parent() { return nullptr; }

    virtual void attach(Node*) {}

    virtual void detach(Node*) {}
};

export auto key(auto const& key) {
    return [key](Child child) {
        child->_key = Some(hash(key));
        return child;
    };
}

export template <typename T>
concept Decorator = requires(T& t, Child& c) {
    { t(c) } -> Meta::Convertible<Child>;
};

export Child operator|(Child child, Decorator auto decorator) {
    return decorator(child);
}

export Child& operator|=(Child& child, Decorator auto decorator) {
    return child = decorator(child);
}

// MARK: LeafNode --------------------------------------------------------------

export template <typename Crtp>
struct LeafNode : Node {
    Node* _parent = nullptr;

    virtual void adopt(Crtp&) {}

    virtual void adoptChildren(Crtp&) {}

    bool compatible(Child& other) override {
        return other.is<Crtp>() and _key == other->key();
    }

    void adopt(Child other) override {
        // NOTE: Nodes should never be part of a state, to
        //       ensure this we check that nodes are not
        //       reused across rebuilds
        if (this == &other.unwrap())
            panic("adopt() called on self, did you forget to wrap the node in a slot?");

        if (not compatible(other))
            return;

        other->_consumed = true;
        auto& old = other.expect<Crtp>();
        adopt(old);
        adoptChildren(old);
    }

    void bubble(App::Event& e) override {
        if (_parent and not e.accepted())
            _parent->bubble(e);
    }

    void event(App::Event&) override {
    }

    Node* parent() override {
        return _parent;
    }

    void attach(Node* parent) override {
        _parent = parent;
    }

    void detach(Node* parent) override {
        if (_parent == parent)
            _parent = nullptr;
    }

    App::HitResult hitTest(Math::Vec2Au) override {
        return App::HitResult::NORMAL;
    }
};

// MARK: GroupNode -------------------------------------------------------------

export template <typename Crtp>
struct GroupNode : LeafNode<Crtp> {
    Children _children;
    Math::RectAu _bound{};

    GroupNode() = default;

    GroupNode(Children children) : _children(children) {
        for (auto& c : _children) {
            c->attach(this);
        }
    }

    ~GroupNode() {
        for (auto& c : _children) {
            c->detach(this);
        }
    }

    Children& children() {
        return _children;
    }

    Children const& children() const {
        return _children;
    }

    static Opt<Child> _counterpart(Child& child, usize index, Children& old) {
        if (child->key()) {
            for (auto& o : old) {
                if (not o->_consumed and o->key() == child->key())
                    return Some(o);
            }
            return NONE;
        }

        if (index < old.len() and not old[index]->key())
            return Some(old[index]);

        return NONE;
    }

    void adoptChildren(Crtp& o) override {
        for (usize i = 0; i < _children.len(); i++) {
            if (auto old = _counterpart(_children[i], i, o.children()))
                _children[i]->adopt(*old);
        }
    }

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        for (auto& child : children()) {
            if (not child->bound().collide(r))
                continue;

            child->paint(g, r);
        }
    }

    void event(App::Event& e) override {
        if (e.accepted())
            return;

        for (auto& child : children()) {
            child->event(e);
            if (e.accepted())
                return;
        }
    }

    void layout(Math::RectAu r) override {
        _bound = r;

        for (auto& child : children())
            child->layout(r);
    }

    Math::RectAu bound() override {
        return _bound;
    }

    App::HitResult hitTest(Math::Vec2Au p) override {
        for (auto& child : children()) {
            if (not child->bound().contains(p))
                continue;
            auto hit = child->hitTest(p);
            if (hit != App::HitResult::NORMAL)
                return hit;
        }
        return App::HitResult::NORMAL;
    }
};

// MARK: ProxyNode -------------------------------------------------------------

export template <typename Crtp>
struct ProxyNode : LeafNode<Crtp> {
    Child _child;

    ProxyNode(Child child) : _child(child) {
        _child->attach(this);
    }

    ~ProxyNode() {
        _child->detach(this);
    }

    Node& child() {
        return *_child;
    }

    Node const& child() const {
        return *_child;
    }

    void adoptChildren(Crtp& o) override {
        _child->adopt(o._child);
    }

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        child().paint(g, r);
    }

    void event(App::Event& e) override {
        if (e.accepted())
            return;

        child().event(e);
    }

    void layout(Math::RectAu r) override {
        child().layout(r);
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        return child().size(s, hint);
    }

    Math::RectAu bound() override {
        return child().bound();
    }

    App::HitResult hitTest(Math::Vec2Au p) override {
        if (not child().bound().contains(p))
            return App::HitResult::NORMAL;
        return child().hitTest(p);
    }
};

export using Slot = SharedFunc<Child()>;

export using Slots = SharedFunc<Children()>;

} // namespace Karm::Ui
