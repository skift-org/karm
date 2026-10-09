export module Karm.Kira:sideNav;

import Karm.Core;
import Karm.Math;
import Karm.Ui;
import Karm.Gfx;
import Mdi;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child sidenavContent(Ui::Children children) {
    return Ui::vflow(8_au, children) |
           Ui::insets(8_au) |
           Ui::vscroll() |
           Ui::minSize({198_au, Ui::UNCONSTRAINED});
}

export Ui::Child sidenavTree(Gfx::Icon icon, String title, Ui::Slot child) {
    return Ui::state(true, [=](bool state, auto bind) {
        return Ui::vflow(
            Ui::button(
                Some(bind(not state)),
                Ui::ButtonStyle::subtle(),
                Ui::hflow(
                    Ui::empty(8_au),
                    Ui::icon(icon, 18_au),
                    Ui::empty(12_au),
                    Ui::labelMedium(title) |
                        Ui::vcenter() |
                        Ui::grow(),
                    Ui::icon(state ? Mdi::CHEVRON_UP : Mdi::CHEVRON_DOWN, 18_au)
                ) | Ui::insets({8_au, 12_au, 8_au, 0_au})
            ),

            state
                ? child() |
                      Ui::insets({0_au, 0_au, 0_au, 32_au}) |
                      Ui::slideIn(Ui::SlideFrom::TOP)
                : Ui::empty()
        );
    });
}

export Ui::Child sidenavItem(bool selected, Opt<Ui::Send<>> onPress, Ui::Child content) {
    auto buttonStyle = Ui::ButtonStyle::regular();

    buttonStyle.idleStyle = {
        .borderRadii = 4,
        .backgroundFill = Some(selected ? Ui::GRAY700 : Gfx::ALPHA),
    };

    auto indicator = Ui::box(
        {
            .borderRadii = 99,
            .backgroundFill = Some(selected ? Ui::ACCENT600 : Gfx::ALPHA),
        },
        Ui::empty(2_au)
    );

    return Ui::button(
        std::move(onPress),
        buttonStyle,
        Ui::hflow(
            indicator,
            Ui::empty(8_au),
            content
        ) |
            Ui::insets({8_au, 12_au, 8_au, 0_au})
    );
}

export Ui::Child sidenavItem(bool selected, Opt<Ui::Send<>> onPress, Gfx::Icon icon, String title) {
    return sidenavItem(
        selected,
        onPress,
        Ui::hflow(
            Ui::icon(icon, 18_au),
            Ui::empty(12_au),
            Ui::labelMedium(title) | Ui::center()
        )
    );
}

export Ui::Child sidenavTitle(String title) {
    return Ui::titleMedium(title) | Ui::insets({8_au, 12_au, 8_au, 8_au});
}

} // namespace Karm::Kira
