export module Karm.Kira:resizable;

import Karm.App;
import Karm.Ui;
import Karm.Math;
import Karm.Core;

namespace Karm::Kira {

export enum struct ResizeHandle {
    TOP,
    START,
    BOTTOM,
    END
};

struct Resizable : Ui::ProxyNode<Resizable> {
    Math::Vec2i _size;
    Opt<Ui::Send<Math::Vec2i>> _onChange;
    bool _grabbed = false;

    Resizable(Ui::Child child, Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange)
        : ProxyNode<Resizable>(child),
          _size(size),
          _onChange(std::move(onChange)) {}

    void reconcile(Resizable& o) override {
        if (o._onChange) {
            _size = o._size;
        }
        _onChange = std::move(o._onChange);
        ProxyNode<Resizable>::reconcile(o);
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
                _size = _size + it->delta;
                auto minSize = child().size({}, Ui::Hint::MIN);
                _size = _size.max(minSize);
                if (_onChange) {
                    _onChange(*this, _size);
                } else {
                    Ui::shouldLayout(*this);
                }
                e.accept();
            }
        }
    }

    void bubble(App::Event& e) override {
        if (auto de = e.is<App::DragStartEvent>()) {
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
};

export Ui::Child resizable(Ui::Child child, Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange) {
    return makeRc<Resizable>(child, size, std::move(onChange));
}

export auto resizable(Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange) {
    return [size, onChange = std::move(onChange)](Ui::Child child) mutable -> Ui::Child {
        return resizable(child, size, std::move(onChange));
    };
}

static Ui::Child _resizeHandle(Math::Vec2i dir) {
    return Ui::empty(4) |
           Ui::box({
               .backgroundFill = Ui::GRAY800,
           }) |
           Ui::dragRegion(dir);
}

export Ui::Child resizable(Ui::Child child, ResizeHandle handlePosition, Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange) {
    if (handlePosition == ResizeHandle::TOP) {
        return Ui::vflow(
                   _resizeHandle({0, -1}),
                   child | Ui::grow()
               ) |
               resizable(size, std::move(onChange));
    } else if (handlePosition == ResizeHandle::START) {
        return Ui::hflow(
                   _resizeHandle({-1, 0}),
                   child | Ui::grow()
               ) |
               resizable(size, std::move(onChange));
    } else if (handlePosition == ResizeHandle::BOTTOM) {
        return Ui::vflow(
                   child | Ui::grow(),
                   _resizeHandle({0, 1})
               ) |
               resizable(size, std::move(onChange));
    } else if (handlePosition == ResizeHandle::END) {
        return Ui::hflow(
                   child | Ui::grow(),
                   _resizeHandle({1, 0})
               ) |
               resizable(size, std::move(onChange));
    } else {
        unreachable();
    }
}

export auto resizable(ResizeHandle handlePosition, Math::Vec2i size, Opt<Ui::Send<Math::Vec2i>> onChange) {
    return [handlePosition, size, onChange](Ui::Child child) mutable -> Ui::Child {
        return resizable(child, handlePosition, size, onChange);
    };
}

} // namespace Karm::Kira
