export module Karm.Kira:pointer;

import Karm.Ui;
import Karm.Gfx;
import Karm.App;
import Karm.Math;
import Karm.Core;

namespace Karm::Kira {

struct Pointer : Ui::ProxyNode<Pointer> {
    bool _mouseDirty = false;
    Math::Path _pointerPath = Math::Path::fromSvg(
#include "defs/pointer.path"

    );

    Math::Path _pointerResizePath = Math::Path::fromSvg(
#include "defs/pointer-resize.path"

    );
    Gfx::Color _pointerFill = Gfx::BLACK;
    Gfx::Color _pointerStroke = Gfx::WHITE;
    App::CursorStyle _cursorStyle = App::CursorStyle::DEFAULT;
    App::CursorStyle _cursorRequest = App::CursorStyle::DEFAULT;
    Math::Vec2i _mousePos = {};

    Pointer(Ui::Child child)
        : ProxyNode(std::move(child)) {}

    Math::Recti _pointerBound(App::CursorStyle style, Math::Vec2i pos) const {
        if (style == App::CursorStyle::DEFAULT) {
            return Math::Recti{
                pos,
                {28, 28},
            }
                .grow(2);
        } else {
            return Math::Recti::fromCenter(pos, {48, 48}).grow(2);
        }
    }

    f64 _pointerAngle(App::CursorStyle style) const {
        switch (style) {
        case App::CursorStyle::RESIZE_EW:
            return 0.;
        case App::CursorStyle::RESIZE_NS:
            return Math::PI / 2;
        case App::CursorStyle::RESIZE_NWSE:
            return Math::PI / 4;
        case App::CursorStyle::RESIZE_NESW:
            return -Math::PI / 4;
        default:
            return 0.;
        }
    }

    void _pointerPaint(App::CursorStyle style, Math::Vec2i pos, Gfx::Canvas& g) const {
        if (style == App::CursorStyle::DEFAULT) {
            g.push();
            g.translate(pos.cast<f64>());
            g.beginPath();
            g.path(_pointerPath);
            g.fill(_pointerFill);
            g.stroke({
                .fill = _pointerStroke,
                .width = 1.6,
                .align = Gfx::CENTER_ALIGN,
                .join = Gfx::MITER_JOIN,
            });
            g.pop();
        } else {
            g.push();
            g.translate(pos.cast<f64>());
            g.rotate(_pointerAngle(style));
            g.beginPath();
            g.path(_pointerResizePath);
            g.fill(_pointerFill);
            g.stroke({
                .fill = _pointerStroke,
                .width = 1.6,
                .align = Gfx::INSIDE_ALIGN,
                .join = Gfx::MITER_JOIN,
            });
            g.pop();
        }
    }

    void event(App::Event& e) override {
        if (auto m = e.is<App::MouseEvent>()) {
            if (not _mouseDirty) {
                _mouseDirty = true;
                Ui::shouldRepaint(*this, _pointerBound(_cursorStyle, _mousePos).cast<Math::Au>());
                Ui::shouldAnimate(*this);
            }

            _mousePos = m->pos;

            if (m->delta != Math::Vec2i{}) {
                _cursorRequest = App::CursorStyle::DEFAULT;

                ProxyNode::event(e);

                if (_cursorRequest != _cursorStyle) {
                    Ui::shouldRepaint(*this, _pointerBound(_cursorStyle, _mousePos).cast<Math::Au>());
                    _cursorStyle = _cursorRequest;
                    Ui::shouldRepaint(*this, _pointerBound(_cursorStyle, _mousePos).cast<Math::Au>());
                }
            } else {
                ProxyNode::event(e);
            }
        } else if (auto k = e.is<AnimateEvent>()) {
            if (_mouseDirty) {
                _mouseDirty = false;
                Ui::shouldRepaint(*this, _pointerBound(_cursorStyle, _mousePos).cast<Math::Au>());
            }
            ProxyNode::event(e);
        } else {
            ProxyNode::event(e);
        }
    }

    void bubble(App::Event& e) override {
        if (auto it = e.is<App::RequestCursorEvent>()) {
            _cursorRequest = it->style;
            e.accept();
            return;
        }

        ProxyNode::bubble(e);
    }

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        child().paint(g, r);

        if (_pointerBound(_cursorStyle, _mousePos).collide(r.cast<isize>())) {
            _pointerPaint(_cursorStyle, _mousePos, g);
        }
    }
};

export auto pointer() {
    return [](Ui::Child child) -> Ui::Child {
        return makeRc<Pointer>(std::move(child));
    };
}

} // namespace Karm::Kira
