export module Karm.Kira:separator;

import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

struct Separator : Ui::View<Separator> {
    Gfx::Color _color;

    Separator(Gfx::Color color)
        : _color(color) {}

    void paint(Gfx::Canvas& g, Math::RectAu) override {
        g.push();
        g.fillStyle(_color);
        g.fill(bound().cast<f64>());
        g.pop();
    }

    Math::Vec2Au size(Math::Vec2Au, Ui::Hint) override {
        return {1_au};
    }
};

export Ui::Child separator(Gfx::Color color = Ui::GRAY800) {
    return makeRc<Separator>(color);
}

export Ui::Child separator(String text) {
    return Ui::hflow(
               4_au,
               Math::Align::VCENTER | Math::Align::HFILL | Math::Align::TOP_START,
               separator() | Ui::grow(),
               Ui::text(Ui::TextStyles::labelSmall().withColor(Ui::GRAY500), text) | Ui::insets({0_au, 6_au}),
               separator() | Ui::grow()
           ) |
           Ui::insets({0_au, 6_au});
}

} // namespace Karm::Kira
