export module Karm.Ui:scroll;

import Karm.App;
import Karm.Gfx;
import Karm.Math;

import :anim;
import :node;
import :atoms;

using namespace Karm::Math::Literals;

namespace Karm::Ui {

// MARK: Scroll ----------------------------------------------------------------

export struct ScrollToEvent {
    Math::RectAu bound;
};

export struct ScrollListener {
    static constexpr Au SCROLL_STEP = 32_au;
    static constexpr Au SCROLL_BAR_WIDTH = 4_au;

    bool _mouseIn = false;
    bool _animated = false;
    Math::Orien _orient{};
    Math::Vec2Au _animatedScroll{};
    Math::Vec2Au _targetScroll{};
    Easedf _scrollOpacity;

    Math::RectAu _contentBound;
    Math::RectAu _containerBound;

    ScrollListener(Math::Orien orient = Math::Orien::BOTH)
        : _orient(orient) {}

    Math::Orien orient() const {
        return _orient;
    }

    void updateContentBound(Math::RectAu rect) {
        _contentBound = rect;
        scroll(_targetScroll);
    }

    Math::RectAu contentBound() const {
        return _contentBound;
    }

    void updateContainerBound(Math::RectAu rect) {
        _containerBound = rect;
        scroll(_targetScroll);
    }

    Math::RectAu containerBound() const {
        return _containerBound;
    }

    Math::Vec2Au scroll() {
        return _animatedScroll;
    }

    bool _closeToTarget() {
        return _animatedScroll.cast<f64>().dist(_targetScroll.cast<f64>()) < 0.5;
    }

    void scroll(Math::Vec2Au s) {
        _targetScroll.x = clamp(s.x, -(_contentBound.width - min(_contentBound.width, _containerBound.width)), 0_au);
        _targetScroll.y = clamp(s.y, -(_contentBound.height - min(_contentBound.height, _containerBound.height)), 0_au);

        if (_closeToTarget()) {
            _animatedScroll = _targetScroll;
            _animated = false;
        } else {
            _animated = true;
        }
    }

    void scrollIntoView(Math::RectAu rect) {
        auto nextScroll = _targetScroll;

        if (canHScroll()) {
            Au containerStart = _containerBound.start();
            Au containerEnd = _containerBound.end();

            Au elementStart = rect.start() + nextScroll.x;
            Au elementEnd = rect.end() + nextScroll.x;

            if (elementStart < containerStart) {
                nextScroll.x = containerStart - rect.start();
            } else if (elementEnd > containerEnd) {
                nextScroll.x = containerEnd - rect.end();
            }
        }

        if (canVScroll()) {
            Au containerTop = _containerBound.top();
            Au containerBottom = _containerBound.bottom();

            Au elementTop = rect.top() + nextScroll.y;
            Au elementBottom = rect.bottom() + nextScroll.y;

            if (elementTop < containerTop) {
                nextScroll.y = containerTop - rect.top();
            } else if (elementBottom > containerBottom) {
                nextScroll.y = containerBottom - rect.bottom();
            }
        }

        scroll(nextScroll);
    }

    bool canHScroll() {
        return (_orient == Math::Orien::HORIZONTAL or _orient == Math::Orien::BOTH) and _contentBound.width > _containerBound.width;
    }

    Math::RectAu hTrack() {
        return Math::RectAu{_containerBound.start(), _containerBound.bottom() - SCROLL_BAR_WIDTH, _containerBound.width, SCROLL_BAR_WIDTH};
    }

    bool canVScroll() {
        return (_orient == Math::Orien::VERTICAL or _orient == Math::Orien::BOTH) and _contentBound.height > _containerBound.height;
    }

    Math::RectAu vTrack() {
        return Math::RectAu{_containerBound.end() - SCROLL_BAR_WIDTH, _containerBound.top(), SCROLL_BAR_WIDTH, _containerBound.height};
    }

    void paint(Gfx::Canvas& g) {
        g.push();
        g.clip(_containerBound.cast<f64>());

        if (canHScroll()) {
            auto ratio = _containerBound.width / _contentBound.width;
            auto scrollBarWidth = _containerBound.width * ratio;
            auto scrollBarX = _containerBound.start() - _animatedScroll.x * ratio;

            g.fillStyle(Gfx::GRAY500.withOpacity(0.5 * clamp01(_scrollOpacity.value())));
            g.fill(Math::RectAu{scrollBarX, _containerBound.bottom() - SCROLL_BAR_WIDTH, scrollBarWidth, SCROLL_BAR_WIDTH}.cast<f64>());
        }

        if (canVScroll()) {
            auto ratio = _containerBound.height / _contentBound.height;
            auto scrollBarHeight = _containerBound.height * ratio;
            auto scrollBarY = _containerBound.top() - _animatedScroll.y * ratio;

            g.fillStyle(Ui::GRAY500.withOpacity(0.5 * clamp01(_scrollOpacity.value())));
            g.fill(Math::RectAu{_containerBound.end() - SCROLL_BAR_WIDTH, scrollBarY, SCROLL_BAR_WIDTH, scrollBarHeight}.cast<f64>());
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

        if (auto ke = e.is<App::KeyboardEvent>(); ke and (ke->type == App::KeyboardEvent::PRESS or ke->type == App::KeyboardEvent::REPEAT)) {
            if (ke->key == App::Key::PGUP) {
                scroll(_targetScroll + Math::Vec2Au{0_au, containerBound().height});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::PGDOWN) {
                scroll(_targetScroll - Math::Vec2Au{0_au, containerBound().height});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::UP) {
                scroll(_targetScroll + Math::Vec2Au{0_au, SCROLL_STEP});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::DOWN) {
                scroll(_targetScroll - Math::Vec2Au{0_au, SCROLL_STEP});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::LEFT) {
                scroll(_targetScroll + Math::Vec2Au{SCROLL_STEP, 0_au});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::RIGHT) {
                scroll(_targetScroll - Math::Vec2Au{SCROLL_STEP, 0_au});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::HOME) {
                scroll(Math::Vec2Au{0_au, 0_au});
                shouldAnimate(n);
                e.accept();
            } else if (ke->key == App::Key::END) {
                scroll(Math::Vec2Au{-(_contentBound.width - _containerBound.width), -(_contentBound.height - _containerBound.height)});
                shouldAnimate(n);
                e.accept();
            }
        } else if (auto me = e.is<App::MouseEvent>()) {
            if (_containerBound.contains(me->pos.cast<Au>())) {
                _mouseIn = true;

                if (me->type == App::MouseEvent::SCROLL) {
                    if (_orient == Math::Orien::BOTH) {
                        scroll(_targetScroll + (me->scroll * SCROLL_STEP.cast<f64>()).cast<Au>());
                    } else if (_orient == Math::Orien::HORIZONTAL) {
                        scroll(_targetScroll + Math::Vec2Au{SCROLL_STEP * (me->scroll.x + me->scroll.y), 0_au});
                    } else if (_orient == Math::Orien::VERTICAL) {
                        scroll(_targetScroll + Math::Vec2Au{0_au, SCROLL_STEP * (me->scroll.x + me->scroll.y)});
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

            auto delta = _targetScroll - _animatedScroll;

            _animatedScroll = _animatedScroll + delta * Math::Vec2f{e.unwrap<Node::AnimateEvent>().dt * 12};

            if (_closeToTarget()) {
                _animatedScroll = _targetScroll;
                _animated = false;
                _scrollOpacity.delay(1.0).animate(n, 0, 0.3);
            } else {
                shouldAnimate(n);
            }
        }
    }
};

struct Scroll : ProxyNode<Scroll> {
    ScrollListener _listener;

    Scroll(Child child, Math::Orien orient)
        : ProxyNode(child), _listener(orient) {}

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        g.push();
        g.clip(_listener.containerBound().cast<f64>());
        g.origin(_listener.scroll().cast<f64>());
        r.xy = r.xy - _listener.scroll();
        child().paint(g, r);
        g.pop();

        // draw scroll bar
        _listener.paint(g);
    }

    void event(App::Event& e) override {
        if (auto me = e.is<App::MouseEvent>(); me) {
            if (_listener.containerBound().contains(me->pos.cast<Au>())) {
                me->pos = me->pos - _listener.scroll().cast<isize>();
                ProxyNode::event(e);
                me->pos = me->pos + _listener.scroll().cast<isize>();
            }
        } else {
            ProxyNode::event(e);
        }

        if (not e.accepted()) {
            _listener.listen(*this, e);
        }
    }

    void bubble(App::Event& event) override {
        if (auto e = event.is<Node::PaintEvent>()) {
            e->bound.xy = e->bound.xy + _listener.scroll();
            e->bound = e->bound.clipTo(bound());
        } else if (auto e = event.is<ScrollToEvent>()) {
            _listener.scrollIntoView(e->bound);
            event.accept();
        }
        ProxyNode::bubble(event);
    }

    void layout(Math::RectAu r) override {
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

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
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

    Math::RectAu bound() override {
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

struct ScrollToMe : ProxyNode<ScrollToMe> {
    Math::InsetsAu _margin;
    bool _activated = true;

    explicit ScrollToMe(Child const& child, Math::InsetsAu margin)
        : ProxyNode(child), _margin(margin) {}

    void event(App::Event& event) override {
        if (event.is<RebuiltEvent>()) {
            _activated = true;
        }

        ProxyNode::event(event);
    }

    void layout(Math::RectAu r) override {
        ProxyNode::layout(r);
        if (_activated) {
            bubble<ScrollToEvent>(*this, bound().grow(_margin));
            _activated = false;
        }
    }
};

export auto scrollToMe(Math::InsetsAu margin = {}) {
    return [margin](Child child) -> Ui::Child {
        return makeRc<ScrollToMe>(child, margin);
    };
}

// MARK: Clip ------------------------------------------------------------------

struct VhClip : ProxyNode<VhClip> {
    Math::Orien _orient{};
    Math::RectAu _bound{};

    VhClip(Child child, Math::Orien orient)
        : ProxyNode(child), _orient(orient) {}

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        g.push();
        g.clip(_bound.cast<f64>());
        child().paint(g, r);
        g.pop();
    }

    void layout(Math::RectAu r) override {
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

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
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

    Math::RectAu bound() override {
        return _bound;
    }
};

struct Clip : ProxyNode<Clip> {
    Clip(Child child)
        : ProxyNode(child) {}

    void
    paint(Gfx::Canvas& g, Math::RectAu r) override {
        g.push();
        g.clip(bound().cast<f64>());
        ProxyNode::paint(g, r);
        g.pop();
    }
};

export auto clip() {
    return [](Child child) -> Child {
        return makeRc<Clip>(child);
    };
}

export Child vhclip(Child child) {
    return makeRc<VhClip>(child, Math::Orien::BOTH);
}

export auto vhclip() {
    return [](Child child) {
        return vhclip(child);
    };
}

export Child hclip(Child child) {
    return makeRc<VhClip>(child, Math::Orien::HORIZONTAL);
}

export auto hclip() {
    return [](Child child) {
        return hclip(child);
    };
}

export Child vclip(Child child) {
    return makeRc<VhClip>(child, Math::Orien::VERTICAL);
}

export auto vclip() {
    return [](Child child) {
        return vclip(child);
    };
}

} // namespace Karm::Ui
