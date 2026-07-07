export module Karm.Kira:progress;

import Karm.App;
import Karm.Core;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;

namespace Karm::Kira {

struct IndeterminedProgress : Ui::View<IndeterminedProgress> {
    isize _size;
    f64 _spin = 0;

    IndeterminedProgress(isize size)
        : _size(size) {
    }

    void reconcile(IndeterminedProgress& o) override {
        _size = o._size;
    }

    void paint(Gfx::Canvas& g, Math::Recti) override {
        g.push();
        g.clip(bound());

        g.translate(bound().center().cast<f64>());
        g.rotate(_spin);

        g.beginPath();
        g.arc({
            {0, 0},
            min(bound().width / 2., bound().height / 2.),
            0,
            Math::PI * 1.25,
        });

        g.stroke(
            Gfx::stroke(Ui::ACCENT500)
                .withWidth(bound().width / 8.)
                .withAlign(Gfx::INSIDE_ALIGN)
                .withCap(Gfx::ROUND_CAP)
        );

        g.pop();
    }

    void event(App::Event& e) override {
        if (auto ae = e.is<Node::AnimateEvent>()) {
            _spin += ae->dt * 8;
            Ui::shouldAnimate(*this);
            Ui::shouldRepaint(*this);
        }

        Ui::View<IndeterminedProgress>::event(e);
    }

    Math::Vec2i size(Math::Vec2i, Ui::Hint) override {
        return _size;
    }
};

export Ui::Child indeterminedProgress(isize size = 16) {
    return makeRc<IndeterminedProgress>(size);
}

struct PieCountDown : Ui::View<PieCountDown> {
    f64 _value;
    isize _size;

    PieCountDown(f64 value, isize size)
        : _value(value), _size(size) {
    }

    void reconcile(PieCountDown& o) override {
        _value = o._value;
    }

    void paint(Gfx::Canvas& g, Math::Recti) override {
        g.push();
        g.clip(bound());

        auto radii = min(bound().width / 2., bound().height / 2.);
        auto center = bound().cast<f64>().center();

        g.fill(Ui::ACCENT500.withOpacity(0.25));
        g.fill(Math::Ellipsef{center, radii});

        g.beginPath();
        g.arc({
            center,
            radii,
            Math::PI * 2 * _value - (Math::PI / 2),
            Math::PI * 2 - (Math::PI / 2),
        });
        g.lineTo(center);
        g.closePath();
        g.fill(Ui::ACCENT500);

        g.pop();
    }

    Math::Vec2i size(Math::Vec2i, Ui::Hint) override {
        return _size;
    }
};

export Ui::Child pieCountDown(f64 value, isize size) {
    return makeRc<PieCountDown>(value, size);
}

} // namespace Karm::Kira
