export module Karm.Kira:avatar;

import Karm.Core;
import Karm.Math;
import Karm.Ui;
import Karm.Gfx;
import Mdi;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child avatar(Union<None, String, Gfx::Icon, Rc<Gfx::Image>> icon = NONE, Math::Au size = 32_au) {
    Ui::BoxStyle boxStyle = {
        .borderRadii = 99,
        .borderWidth = 2,
        .borderFill = Some(Ui::GRAY950),
        .backgroundFill = Some(Ui::GRAY800),
        .foregroundFill = Ui::GRAY400,
    };

    auto innerSize = size * 0.56;
    auto textSize = Math::ceili(size.cast<f64>() * 0.4);

    Ui::Child inner = icon.visit(
        [&](None) {
            return Ui::icon(Mdi::ACCOUNT, innerSize);
        },
        [&](String s) {
            return Ui::text(Ui::TextStyles::labelMedium().withFontSize(textSize), s);
        },
        [&](Gfx::Icon i) {
            return Ui::icon(i, innerSize);
        },
        [](Rc<Gfx::Image> s) {
            return Ui::image(s, Some(999));
        }
    );

    return inner |
           Ui::center() |
           Ui::pinSize(size) |
           Ui::box(boxStyle) |
           Ui::center();
}

export Ui::Child avatarGroup(Ui::Children avatars) {
    return Ui::hflow(-12_au, std::move(avatars));
}

} // namespace Karm::Kira
