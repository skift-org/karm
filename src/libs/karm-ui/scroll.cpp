export module Karm.Ui:scroll;

import Karm.App;
import Karm.Gfx;
import Karm.Math;

import :anim;
import :node;
import :atoms;

namespace Karm::Ui {

// MARK: Scroll ----------------------------------------------------------------

export struct ScrollListener {
    static constexpr isize SCROLL_BAR_WIDTH = 4;

    bool _mouseIn = false;
    bool _animated = false;
    Math::Orien _orient{};
    Math::Vec2f _scroll{};
    Math::Vec2f _targetScroll{};
    Easedf _scrollOpacity;

    Math::Recti _contentBound;
    Math::Recti _containerBound;

    ScrollListener(Math::Orien orient = Math::Orien::BOTH)
        : _orient(orient) {}

    Math::Orien orient() const {
        return _orient;
    }

    void updateContentBound(Math::Recti rect) {
        _contentBound = rect;
        scroll(_targetScroll.cast<isize>());
    }

    Math::Recti contentBound() const {
        return _contentBound;
    }

    void updateContainerBound(Math::Recti rect) {
        _containerBound = rect;
        scroll(_targetScroll.cast<isize>());
    }

    Math::Recti containerBound() const {
        return _containerBound;
    }

    Math::Vec2f scroll() {
        return _scroll;
    }

    void scroll(Math::Vec2i s) {
        _targetScroll.x = clamp(s.x, -(_contentBound.width - min(_contentBound.width, _containerBound.width)), 0);
        _targetScroll.y = clamp(s.y, -(_contentBound.height - min(_contentBound.height, _containerBound.height)), 0);

        if (_scroll.dist(_targetScroll) < 0.5) {
            _scroll = _targetScroll;
            _animated = false;
        } else {
            _animated = true;
        }
    }

    bool canHScroll() {
        return (_orient == Math::Orien::HORIZONTAL or _orient == Math::Orien::BOTH) and _contentBound.width > _containerBound.width;
    }

    Math::Recti hTrack() {
        return Math::Recti{_containerBound.start(), _containerBound.bottom() - SCROLL_BAR_WIDTH, _containerBound.width, SCROLL_BAR_WIDTH};
    }

    bool canVScroll() {
        return (_orient == Math::Orien::VERTICAL or _orient == Math::Orien::BOTH) and _contentBound.height > _containerBound.height;
    }

    Math::Recti vTrack() {
        return Math::Recti{_containerBound.end() - SCROLL_BAR_WIDTH, _containerBound.top(), SCROLL_BAR_WIDTH, _containerBound.height};
    }

    void paint(Gfx::Canvas& g) {
        g.push();
        g.clip(_containerBound);

        if (canHScroll()) {
            auto scrollBarWidth = (_containerBound.width) * _containerBound.width / _contentBound.width;
            auto scrollBarX = _containerBound.start() + (-_scroll.x * _containerBound.width / _contentBound.width);

            g.fillStyle(Gfx::GRAY500.withOpacity(0.5 * clamp01(_scrollOpacity.value())));
            g.fill(Math::Recti{(isize)scrollBarX, _containerBound.bottom() - SCROLL_BAR_WIDTH, scrollBarWidth, SCROLL_BAR_WIDTH});
        }

        if (canVScroll()) {
            auto scrollBarHeight = (_containerBound.height) * _containerBound.height / _contentBound.height;
            auto scrollBarY = _containerBound.top() + (-_scroll.y * _containerBound.height / _contentBound.height);

            g.fillStyle(Ui::GRAY500.withOpacity(0.5 * clamp01(_scrollOpacity.value())));
            g.fill(Math::Recti{_containerBound.end() - SCROLL_BAR_WIDTH, (isize)scrollBarY, SCROLL_BAR_WIDTH, scrollBarHeight});
        }

        g.pop();
    }

    void listen(Node& n, App::Event& e) {
        if (e.accepted())
            return;

        if (_scrollOpacity.needRepaint(n, e)) {
            if (canHScroll())
                shouldRepaint(*n.parent(), hTrack());

            if (canVScroll())
                shouldRepaint(*n.parent(), vTrack());
        }

        if (auto me = e.is<App::MouseEvent>()) {
            if (_containerBound.contains(me->pos)) {
                _mouseIn = true;

                if (me->type == App::MouseEvent::SCROLL) {
                    if (_orient == Math::Orien::BOTH) {
                        scroll((_scroll + me->scroll * 128).cast<isize>());
                    } else if (_orient == Math::Orien::HORIZONTAL) {
                        scroll((_scroll + Math::Vec2f{(me->scroll.x + me->scroll.y) * 128, 0}).cast<isize>());
                    } else if (_orient == Math::Orien::VERTICAL) {
                        scroll((_scroll + Math::Vec2f{0, (me->scroll.x + me->scroll.y) * 128}).cast<isize>());
                    }
                    shouldAnimate(n);
                    _scrollOpacity.delay(0).animate(n, 1, 0.3);
                    e.accept();
                }
            } else if (_mouseIn) {
                _mouseIn = false;
                mouseLeave(n);
            }
        } else if (e.is<Node::AnimateEvent>() and _animated) {
            shouldRepaint(*n.parent(), _containerBound);

            auto delta = _targetScroll - _scroll;

            _scroll = _scroll + delta * (e.unwrap<Node::AnimateEvent>().dt * 12);

            if (_scroll.dist(_targetScroll) < 0.5) {
                _scroll = _targetScroll;
                _animated = false;
                _scrollOpacity.delay(1.0).animate(n, 0, 0.3);
            } else {
                shouldAnimate(n);
            }
        }
    }
};

struct Scroll : ProxyNode<Scroll> {
    static constexpr isize SCROLL_BAR_WIDTH = 4;
    ScrollListener _listener;

    Scroll(Child child, Math::Orien orient)
        : ProxyNode(child), _listener(orient) {}

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        g.push();
        g.clip(_listener.containerBound());
        g.origin(_listener.scroll());
        r.xy = r.xy - _listener.scroll().cast<isize>();
        child().paint(g, r);
        g.pop();

        // draw scroll bar
        _listener.paint(g);
    }

    void event(App::Event& e) override {
        if (auto me = e.is<App::MouseEvent>(); me) {
            if (_listener.containerBound().contains(me->pos)) {
                me->pos = me->pos - _listener.scroll().cast<isize>();
                ProxyNode<Scroll>::event(e);
                me->pos = me->pos + _listener.scroll().cast<isize>();
            }
        } else {
            ProxyNode<Scroll>::event(e);
        }

        if (not e.accepted()) {
            _listener.listen(*this, e);
        }
    }

    void bubble(App::Event& e) override {
        if (auto pe = e.is<Node::PaintEvent>()) {
            pe->bound.xy = pe->bound.xy + _listener.scroll().cast<isize>();
            pe->bound = pe->bound.clipTo(bound());
        }
        ProxyNode::bubble(e);
    }

    void layout(Math::Recti r) override {
        _listener.updateContainerBound(r);
        auto childSize = child().size(r.size(), Hint::MAX);
        if (_listener.orient() == Math::Orien::HORIZONTAL) {
            childSize.height = r.height;
        } else if (_listener.orient() == Math::Orien::VERTICAL) {
            childSize.width = r.width;
        }

        // Make sure the child is at least as big as the parent
        childSize.width = max(childSize.width, r.width);
        childSize.height = max(childSize.height, r.height);
        child().layout({r.xy, childSize});
        _listener.updateContentBound(childSize);
    }

    Math::Vec2i size(Math::Vec2i s, Hint hint) override {
        auto childSize = child().size(s, hint);
        if (hint == Hint::MIN) {
            if (_listener.orient() == Math::Orien::HORIZONTAL) {
                childSize.x = min(childSize.x, s.x);
            } else if (_listener.orient() == Math::Orien::VERTICAL) {
                childSize.y = min(childSize.y, s.y);
            } else {
                childSize = childSize.min(s);
            }
        }
        return childSize;
    }

    Math::Recti bound() override {
        return _listener.containerBound();
    }
};

export Child vhscroll(Child child) {
    return makeRc<Scroll>(child, Math::Orien::BOTH);
}

export auto vhscroll() {
    return [](Child child) {
        return vhscroll(child);
    };
}

export Child hscroll(Child child) {
    return makeRc<Scroll>(child, Math::Orien::HORIZONTAL);
}

export auto hscroll() {
    return [](Child child) {
        return hscroll(child);
    };
}

export Child vscroll(Child child) {
    return makeRc<Scroll>(child, Math::Orien::VERTICAL);
}

export auto vscroll() {
    return [](Child child) {
        return vscroll(child);
    };
}

// MARK: Clip ------------------------------------------------------------------

struct Clip : ProxyNode<Clip> {
    static constexpr isize SCROLL_BAR_WIDTH = 4;

    Math::Orien _orient{};
    Math::Recti _bound{};

    Clip(Child child, Math::Orien orient)
        : ProxyNode(child), _orient(orient) {}

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        g.push();
        g.clip(_bound);
        child().paint(g, r);
        g.pop();
    }

    void layout(Math::Recti r) override {
        _bound = r;
        auto childSize = child().size(_bound.size(), Hint::MAX);
        if (_orient == Math::Orien::HORIZONTAL) {
            childSize.height = r.height;
        } else if (_orient == Math::Orien::VERTICAL) {
            childSize.width = r.width;
        }
        r.wh = childSize;
        child().layout(r);
    }

    Math::Vec2i size(Math::Vec2i s, Hint hint) override {
        auto childSize = child().size(s, hint);

        if (hint == Hint::MIN) {
            if (_orient == Math::Orien::HORIZONTAL) {
                childSize.x = min(childSize.x, s.x);
            } else if (_orient == Math::Orien::VERTICAL) {
                childSize.y = min(childSize.y, s.y);
            } else {
                childSize = childSize.min(s);
            }
        }

        return childSize;
    }

    Math::Recti bound() override {
        return _bound;
    }
};

export Child vhclip(Child child) {
    return makeRc<Clip>(child, Math::Orien::BOTH);
}

export auto vhclip() {
    return [](Child child) {
        return vhclip(child);
    };
}

export Child hclip(Child child) {
    return makeRc<Clip>(child, Math::Orien::HORIZONTAL);
}

export auto hclip() {
    return [](Child child) {
        return hclip(child);
    };
}

export Child vclip(Child child) {
    return makeRc<Clip>(child, Math::Orien::VERTICAL);
}

export auto vclip() {
    return [](Child child) {
        return vclip(child);
    };
}

} // namespace Karm::Ui
