export module Karm.Kira:selection;

import Karm.Core;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.App;

namespace Karm::Kira {

struct SelectionSet {
    bool _all = false;
    Set<Ui::Key> _keys = {};

    static SelectionSet all() {
        return {
            ._all = true,
        };
    }
};

struct SelectionUpdateEvent {
    Math::Recti rect;
};

struct SelectionArea : Ui::ProxyNode<SelectionArea> {
    Ui::MouseListener _listener;
    bool _selecting = false;
    Math::Vec2i _startPos = {};
    Math::Vec2i _endPos = {};

    explicit SelectionArea(Ui::Child const& child)
        : ProxyNode(child) {}

    Math::Recti selectionRect() const {
        return Math::Recti::fromTwoPoint(_startPos, _endPos);
    }

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        ProxyNode::paint(g, r);

        if (_selecting) {
            g.push();
            g.fillStyle(Ui::ACCENT500.withOpacity(0.25));
            g.fill(selectionRect(), 6);
            g.strokeStyle({Ui::ACCENT500, 1, Gfx::INSIDE_ALIGN});
            g.stroke(selectionRect().cast<f64>(), 6);
            g.pop();
        }
    }

    void event(App::Event& e) override {
        ProxyNode::event(e);
        if (e.accepted())
            return;

        if (auto mouseEvent = e.is<App::MouseEvent>()) {
            if (_selecting) {
                if (mouseEvent->type == App::MouseEvent::RELEASE and
                    mouseEvent->button == App::MouseButton::LEFT) {
                    _selecting = false;
                    Ui::shouldRepaint(*this, selectionRect().clipTo(bound()));
                    e.accept();
                }

                if (mouseEvent->type == App::MouseEvent::MOVE) {
                    Ui::shouldRepaint(*this, selectionRect().clipTo(bound()));
                    _endPos = mouseEvent->pos;
                    Ui::shouldRepaint(*this, selectionRect().clipTo(bound()));

                    auto selectionChange = App::makeEvent<SelectionUpdateEvent>(selectionRect());
                    child().event(*selectionChange);
                }
            } else {
                if (mouseEvent->type == App::MouseEvent::PRESS and
                    mouseEvent->button == App::MouseButton::LEFT and
                    bound().contains(mouseEvent->pos)) {
                    _selecting = true;
                    _startPos = mouseEvent->pos;
                    _endPos = mouseEvent->pos;
                    e.accept();

                    auto selectionChange = App::makeEvent<SelectionUpdateEvent>(selectionRect());
                    child().event(*selectionChange);
                }
            }
        }
    }
};

export auto selectionArea() {
    return [](Ui::Child child) -> Ui::Child {
        return makeRc<SelectionArea>(child);
    };
}

struct SelectionItem : Ui::ProxyNode<SelectionItem> {
    bool _selected = false;

    explicit SelectionItem(Ui::Child const& child)
        : ProxyNode(child) {}

    void event(App::Event& e) override {
        if (auto selectionEvent = e.is<SelectionUpdateEvent>()) {
            bool const newSelected = selectionEvent->rect.colide(bound());
            if (_selected != newSelected) {
                _selected = newSelected;
                Ui::shouldRepaint(*this);
            }
            return;
        }

        ProxyNode::event(e);
    }

    void paint(Gfx::Canvas& g, Math::Recti r) override {
        if (_selected) {
            g.push();
            g.fillStyle(Ui::GRAY500.withOpacity(0.2));
            g.fill(bound(), 4);
            g.pop();
        }
        ProxyNode::paint(g, r);
    }
};

export auto selectionItem() {
    return [](Ui::Child child) -> Ui::Child {
        return makeRc<SelectionItem>(child);
    };
}

} // namespace Karm::Kira
