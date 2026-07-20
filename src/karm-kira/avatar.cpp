export module Karm.Kira:avatar;

import Karm.Core;
import Karm.Ui;
import Karm.Gfx;
import Mdi;

namespace Karm::Kira {

export Ui::Child avatar(Union<None, String, Gfx::Icon, Rc<Gfx::Image>> icon = NONE, usize size = 46) {
    Ui::BoxStyle boxStyle = {
        .borderRadii = 99,
        .backgroundFill = Ui::GRAY800,
        .foregroundFill = Ui::GRAY400
    };

    auto innerSize = Math::ceili(size * 0.56);

    Ui::Child inner = icon.visit(
        [&](None) {
            return Ui::icon(Mdi::ACCOUNT, innerSize);
        },
        [&](String s) {
            return Ui::text(Ui::TextStyles::labelMedium().withFontSize(innerSize), s);
        },
        [&](Gfx::Icon i) {
            return Ui::icon(i, Math::ceili(innerSize));
        },
        [](Rc<Gfx::Image> s) {
            return Ui::image(s, {999});
        }
    );

    return inner |
           Ui::center() |
           Ui::pinSize(size) |
           Ui::box(boxStyle) |
           Ui::center();
}

} // namespace Karm::Kira
