export module Karm.Kira:resizable;

import Karm.App;
import Karm.Ui;
import Karm.Math;
import Karm.Core;
import Karm.Gfx;

namespace Karm::Kira {

export enum struct ResizeHandlePosition {
    TOP,
    START,
    BOTTOM,
    END
};

struct Resizable : Ui::ProxyNode<Resizable> {
    Math::Vec2i _size;
    Math::Vec2i _resizeDirection;
    Opt<Ui::Send<Math::Vec2i>> _onChange;
    bool _grabbed = false;

    Resizable(Ui::Child child, Math::Vec2i size, Math::Vec2i resizeDirection, Opt<Ui::Send<Math::Vec2i>> onChange)
        : ProxyNode(child),
          _size(size),
          _resizeDirection{resizeDirection},
          _onChange(std::move(onChange)) {}

    void reconcile(Resizable& o) override {
        if (o._onChange) {
            _size = o._size;
        }
        _onChange = std::move(o._onChange);
        ProxyNode::reconcile(o);
    }

    void event(App::Event& e) override {
        ProxyNode::event(e);

        if (e.accepted())
            return;

        if (auto it = e.is<App::MouseEvent>(); it and _grabbed) {
            if (it->type == App::MouseEvent::RELEASE) {
                _grabbed = false;
                e.accept();
            } else if (it->type == App::MouseEvent::MOVE) {
                _size = _size + it->delta * _resizeDirection;
                auto minSize = child().size({}, Ui::Hint::MIN);
                _size = _size.max(minSize);
                if (_onChange) {
                    _onChange(*this, _size);
                } else {
                    Ui::shouldLayout(*this);
                }
                Ui::bubble<App::RequestCursorEvent>(*this, _resizeDirection.x ? App::CursorStyle::RESIZE_EW : App::CursorStyle::RESIZE_NS);
                e.accept();
            }
        }
    }

    void bubble(App::Event& e) override {
        if (e.is<App::DragStartEvent>()) {
            _grabbed = true;
            e.accept();
        }

        ProxyNode::bubble(e);
    }

    Math::Vec2i size(Math::Vec2i s, Ui::Hint hint) override {
        return child()
            .size(s, hint)
            .max(_size);
    }

    App::HitResult hitTest(Math::Vec2i) override {
        return App::HitResult::HIT;
    }
};

export Ui::Child resizable(Ui::Child child, Math::Vec2i size, Math::Vec2i resizeDirection, Opt<Ui::Send<Math::Vec2i>> onChange) {
    return makeRc<Resizable>(child, size, resizeDirection, std::move(onChange));
}

export auto resizable(Math::Vec2i size, Math::Vec2i resizeDirection, Opt<Ui::Send<Math::Vec2i>> onChange) {
    return [size, onChange = std::move(onChange), resizeDirection](Ui::Child child) mutable -> Ui::Child {
        return resizable(child, size, resizeDirection, std::move(onChange));
    };
}

struct ResizeHandle : Ui::View<ResizeHandle> {
    App::CursorStyle _cursor;
    bool _pressed = false;
    bool _hover = false;

    ResizeHandle(App::CursorStyle cursor = App::CursorStyle::DEFAULT)
        : _cursor(cursor) {}

    void paint(Gfx::Canvas& g, Math::Recti) override {
        g.push();
        if (_pressed) {
            g.fillStyle(Ui::ACCENT700);
            g.fill(bound().cast<f64>(), 99);
        } else if (_hover) {
            g.fillStyle(Ui::GRAY600);
            g.fill(bound().cast<f64>(), 99);
        }
        g.pop();
    }

    void event(App::Event& event) override {
        if (auto it = event.is<App::MouseEvent>()) {
            bool wasHover = _hover;
            _hover = bound().contains(it->pos);
            if (_hover and it->type == App::MouseEvent::PRESS) {
                _pressed = true;
                bubble<App::DragStartEvent>(*this);
                event.accept();
                Ui::shouldRepaint(*this);
            } else if (_hover and it->type == App::MouseEvent::MOVE) {
                bubble<App::RequestCursorEvent>(*this, _cursor);
            } else if (it->type == App::MouseEvent::RELEASE) {
                _pressed = false;
                Ui::shouldRepaint(*this);
            }

            if (wasHover != _hover) {
                Ui::shouldRepaint(*this);
            }
        }
    }

    Math::Vec2i size(Math::Vec2i, Ui::Hint) override {
        return 4;
    }
};

export Ui::Child resizable(Ui::Child child, ResizeHandlePosition handlePosition, Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange) {
    if (handlePosition == ResizeHandlePosition::TOP) {
        return Ui::vflow(
                   makeRc<ResizeHandle>(App::CursorStyle::RESIZE_NS),
                   child | Ui::grow()
               ) |
               resizable(size, {0, -1}, std::move(onChange));
    } else if (handlePosition == ResizeHandlePosition::START) {
        return Ui::hflow(
                   makeRc<ResizeHandle>(App::CursorStyle::RESIZE_EW),
                   child | Ui::grow()
               ) |
               resizable(size, {-1, 0}, std::move(onChange));
    } else if (handlePosition == ResizeHandlePosition::BOTTOM) {
        return Ui::vflow(
                   child | Ui::grow(),
                   makeRc<ResizeHandle>(App::CursorStyle::RESIZE_NS)
               ) |
               resizable(size, {0, 1}, std::move(onChange));
    } else if (handlePosition == ResizeHandlePosition::END) {
        return Ui::hflow(
                   child | Ui::grow(),
                   makeRc<ResizeHandle>(App::CursorStyle::RESIZE_EW)
               ) |
               resizable(size, {1, 0}, std::move(onChange));
    } else {
        unreachable();
    }
}

export auto resizable(ResizeHandlePosition handlePosition, Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange) {
    return [handlePosition, size, onChange](Ui::Child child) mutable -> Ui::Child {
        return resizable(child, handlePosition, size, onChange);
    };
}

} // namespace Karm::Kira
