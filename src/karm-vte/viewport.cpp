export module Karm.Vte:viewport;

import Karm.Ui;
import Karm.App;

import :terminal;

namespace Karm::Vte {

struct Viewport : Ui::View<Viewport> {
    Rc<Terminal> _terminal;
    Ui::ScrollListener _listen;
    Ui::Send<App::KeyboardEvent> _send;
    bool _animate = false;

    Viewport(Rc<Terminal> terminal, Ui::Send<App::KeyboardEvent> send)
        : _terminal(terminal), _send(send) {}

    void paint(Gfx::Canvas& g, Math::Recti) override {
        g.push();
        g.clip(bound());
        g.origin(bound().xy.cast<f64>());
        _terminal->paint(g);
        g.pop();
    }

    void event(App::Event& event) override {
        if (event.accepted())
            return;
        if (event.is<AnimateEvent>()) {
            if (_animate) {
                Ui::shouldRepaint(*this);
                _animate = false;
            }
        } else if (auto e = event.is<App::MouseEvent>()) {
            if (e->type == App::MouseEvent::SCROLL) {
                _terminal->scroll(-e->scroll.y * 3);
                Ui::shouldAnimate(*this);
                _animate = true;
                event.accept();
            }
        } else if (auto e = event.is<App::KeyboardEvent>()) {
            _send(*this, *e);
        } else
            View::event(event);
    }

    void layout(Math::Recti bound) override {
        _terminal->updateViewport(bound.size());
        View::layout(bound);
    }

    Math::Vec2i size(Math::Vec2i s, Ui::Hint hint) override {
        if (hint == Ui::Hint::MAX)
            return s;
        return {};
    }
};

export Ui::Child viewport(Rc<Terminal> terminal, Ui::Send<App::KeyboardEvent> send) {
    return makeRc<Viewport>(terminal, send);
}

} // namespace Karm::Vte
