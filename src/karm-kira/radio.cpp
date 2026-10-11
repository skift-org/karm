export module Karm.Kira:radio;

import Karm.App;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

struct Radio : Ui::View<Radio> {
    static constexpr Math::Au SIZE = 18_au;

    bool _value = false;
    Ui::Send<bool> _onChange;
    Ui::MouseListener _mouseListener;

    Radio(bool value, Ui::Send<bool> onChange)
        : _value(value), _onChange(std::move(onChange)) {
    }

    void reconcile(Radio& o) override {
        _value = o._value;
        _onChange = std::move(o._onChange);
    }

    void paint(Gfx::Canvas& g, Math::RectAu) override {
        auto b = Math::RectAu{SIZE}.center(bound());

        g.push();
        if (_value) {
            g.fillStyle(_mouseListener.isHover() ? Ui::ACCENT400 : Ui::ACCENT500);
            g.fill(b.cast<f64>(), 999);

            g.fillStyle(Gfx::GRAY50);
            g.fill(b.shrink(4_au).cast<f64>(), 999);

            if (_mouseListener.isPress()) {
                g.strokeStyle(Gfx::stroke(Ui::ACCENT500).withWidth(1).withAlign(Gfx::INSIDE_ALIGN));
                g.stroke(b.cast<f64>(), 999);
            }
        } else {
            g.fillStyle(_mouseListener.isHover() ? Ui::GRAY600 : Ui::GRAY700);
            g.fill(b.cast<f64>(), 999);

            if (_mouseListener.isPress()) {
                g.strokeStyle(Gfx::stroke(Ui::GRAY600).withWidth(1).withAlign(Gfx::INSIDE_ALIGN));
                g.stroke(b.cast<f64>(), 999);
            }
        }
        g.pop();
    }

    void event(App::Event& e) override {
        if (_mouseListener.listen(*this, e)) {
            _value = not _value;
            _onChange(*this, _value);
        }
    }

    Math::Vec2Au size(Math::Vec2Au, Ui::Hint) override {
        return {32_au};
    }
};

export Ui::Child radio(bool value, Ui::Send<bool> onChange) {
    return makeRc<Radio>(value, std::move(onChange));
}

export Ui::Child radio(bool value, Ui::Send<bool> onChange, String label) {
    return Ui::hflow(
               8_au,
               Math::Align::CENTER,
               radio(value, onChange) | Ui::pinSize(18_au),
               Ui::labelMedium(label)
           ) |
           Ui::insets({4_au, 14_au, 4_au, 10_au}) |
           Ui::minSize({Ui::UNCONSTRAINED, 32_au}) |
           Ui::bound();
}

} // namespace Karm::Kira
