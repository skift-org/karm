export module Karm.Kira:avatar;

import Karm.Core;
import Karm.Ui;
import Karm.Gfx;
import Mdi;

namespace Karm::Kira {

export Ui::Child avatar(Union<None, String, Gfx::Icon, Rc<Gfx::Surface>> icon = NONE, usize size = 46) {
    Ui::BoxStyle boxStyle = {
        .borderRadii = 99,
        .backgroundFill = Ui::GRAY800,
        .foregroundFill = Ui::GRAY400
    };

    auto innerSize = Math::ceili(size * 0.56);

    Ui::Child inner = icon.visit(Visitor{
        [&](None) {
            return Ui::icon(Mdi::ACCOUNT, innerSize);
        },
        [&](String s) {
            return Ui::text(Ui::TextStyles::labelMedium().withSize(innerSize), s);
        },
        [&](Gfx::Icon i) {
            return Ui::icon(i, Math::ceili(innerSize));
        },
        [](Rc<Gfx::Surface> s) {
            return Ui::image(s, {999});
        },
    });

    return inner |
           Ui::center() |
           Ui::pinSize(size) |
           Ui::box(boxStyle) |
           Ui::center();
}

} // namespace Karm::Kira
