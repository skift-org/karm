export module Karm.Vte:viewport;

import Karm.Ui;
import Karm.App;

import :terminal;

namespace Karm::Vte {

struct Viewport : Ui::View<Viewport> {
    Rc<Terminal> _terminal;
    Ui::ScrollListener _listen;

    Viewport(Rc<Terminal> terminal)
        : _terminal(terminal) {}

    void paint(Gfx::Canvas& g, Math::Recti) override {
        g.push();
        g.clip(bound());
        g.origin(bound().xy.cast<f64>());
        _terminal->paint(g);
        g.pop();
    }

    void event(App::Event& e) override {
        View::event(e);
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

export Ui::Child viewport(Rc<Terminal> terminal) {
    return makeRc<Viewport>(terminal);
}

} // namespace Karm::Vte