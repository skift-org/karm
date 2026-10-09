export module Karm.Kira:select;

import Karm.App;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;
import Mdi;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child select(Ui::Child value, Ui::Slots slots) {
    return Ui::button(
        Some([slots = std::move(slots)](auto& n) {
            auto popover =
                Ui::vflow(
                    slots()
                ) |
                Ui::vscroll() |
                Ui::box({
                    .borderRadii = 6,
                    .borderWidth = 1,
                    .borderFill = Some(Ui::GRAY800),
                    .backgroundFill = Some(Ui::GRAY900),
                    .shadowStyle = Some(Gfx::BoxShadow::elevated(4)),
                }) |
                Ui::scaleIn();

            if (App::formFactor == App::FormFactor::DESKTOP) {
                Ui::showPopover(
                    n,
                    n.bound().bottomStart(),
                    popover |
                        Ui::sizing({n.bound().width, Ui::UNCONSTRAINED}, {Ui::UNCONSTRAINED, 160_au})
                );
            } else {
                Ui::showDialog(
                    n,
                    popover |
                        Ui::sizing({240_au, Ui::UNCONSTRAINED}, {Ui::UNCONSTRAINED, 320_au}) |
                        Ui::center()
                );
            }
        }),
        Ui::ButtonStyle::outline(),
        Ui::hflow(
            8_au,
            Math::Align::CENTER,
            value | Ui::grow(),
            Ui::icon(Mdi::CHEVRON_DOWN)
        ) | Ui::insets({6_au, 12_au, 6_au, 16_au}) |
            Ui::minSize({Ui::UNCONSTRAINED, 32_au})
    );
}

export Ui::Child selectValue(String text) {
    return Ui::labelMedium(text);
}

export Ui::Child selectLabel(String text) {
    return Ui::labelMedium(Ui::GRAY400, text) |
           Ui::insets({12_au, 6_au, 3_au, 14_au});
}

export Ui::Child selectItem(Opt<Ui::Send<>> onPress, String t) {
    return Ui::hflow(
               12_au,
               Math::Align::CENTER,
               Ui::text(t)
           ) |
           Ui::insets({6_au, 6_au, 6_au, 10_au}) |
           Ui::minSize({Ui::UNCONSTRAINED, 28_au}) |
           Ui::button(
               Some([onPress = std::move(onPress)](auto& n) {
                   onPress(n);
                   Ui::closePopover(n);
               }),
               Ui::ButtonStyle::subtle()
           ) |
           Ui::insets(4_au);
}

export Ui::Child selectGroup(Ui::Children children) {
    return Ui::vflow(children);
}

} // namespace Karm::Kira
