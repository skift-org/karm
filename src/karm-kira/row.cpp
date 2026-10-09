export module Karm.Kira:row;

import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Mdi;

import :checkbox;
import :colorInput;
import :number;
import :radio;
import :select;
import :slider;
import :toggle;
import :tabbar;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child rowContent(Ui::Child child) {
    return child |
           Ui::align(Math::Align::VCENTER | Math::Align::START | Math::Align::HSTRETCH) |
           Ui::insets(16_au) |
           Ui::minSize({Ui::UNCONSTRAINED, 64_au});
}

export Ui::Child rowSpacer() {
    return Ui::empty(8_au);
}

export Ui::Child rowContent(Opt<Ui::Child> leading, String title, Opt<String> subtitle, Opt<Ui::Child> trailing) {
    auto lead = leading
                    ? *leading |
                          Ui::center() |
                          Ui::sizing(26_au, {Ui::UNCONSTRAINED, 26_au}) |
                          Ui::insets({0_au, 12_au, 0_au, 0_au})
                    : Ui::empty();

    auto t = subtitle
                 ? Ui::vflow(
                       2_au,
                       Ui::labelMedium(title),
                       Ui::labelSmall(Ui::GRAY400, *subtitle)
                   ) | Ui::insets({6_au, 0_au})
                 : Ui::labelMedium(title);

    auto trail = trailing
                     ? *trailing |
                           Ui::center() |
                           Ui::sizing(26_au, {Ui::UNCONSTRAINED, 26_au})
                     : Ui::empty();

    return Ui::hflow(
               0_au,
               Math::Align::VCENTER | Math::Align::HFILL,
               lead,
               t | Ui::grow(),
               trail
           ) |
           Ui::insets({0_au, 16_au}) |
           Ui::minSize({Ui::UNCONSTRAINED, 64_au});
}

export Ui::Child titleRow(String t) {
    return Ui::titleMedium(t) |
           Ui::insets({16_au, 12_au, 8_au, 12_au});
}

export Ui::Child labelRow(String t) {
    return rowContent(Ui::labelMedium(t));
}

export Ui::Child pressableRow(Opt<Ui::Send<>> onPress, Opt<Ui::Child> leading, String title, Opt<String> subtitle, Opt<Ui::Child> trailing) {
    return button(
        std::move(onPress),
        Ui::ButtonStyle::subtle(),
        rowContent(
            leading,
            title,
            subtitle,
            trailing
        )
    );
}

export Ui::Child buttonRow(Opt<Ui::Send<>> onPress, Gfx::Icon i, String title, Opt<String> subtitle, String action) {
    return rowContent(
        Some(Ui::icon(i, 24_au)),
        title,
        subtitle,
        Some(Ui::button(onPress, action) | Ui::insets({0_au, 0_au, 0_au, 12_au}))
    );
}

export Ui::Child buttonRow(Opt<Ui::Send<>> onPress, String title, Opt<String> subtitle, String action) {
    return rowContent(
        NONE,
        title,
        subtitle,
        Some(Ui::button(onPress, action) | Ui::insets({0_au, 0_au, 0_au, 12_au}))
    );
}

export Ui::Child toggleRow(bool value, Ui::Send<bool> onChange, String title) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(toggle(value, std::move(onChange)))
    );
}

export Ui::Child checkboxRow(bool value, Ui::Send<bool> onChange, String title) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(checkbox(value, std::move(onChange)))
    );
}

export Ui::Child radioRow(bool value, Ui::Send<bool> onChange, String title) {
    return rowContent(
        Some(radio(value, std::move(onChange))),
        title,
        NONE,
        NONE
    );
}

export Ui::Child sliderRow(f64 value, Ui::Send<f64> onChange, String title) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(
            Kira::slider(
                value,
                Some(onChange)
            ) |
            Ui::minSize({128_au, Ui::UNCONSTRAINED})
        )
    );
}

export Ui::Child selectRow(Ui::Child value, Ui::Slots options, String title) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(select(std::move(value), std::move(options)))
    );
}

export Ui::Child colorRow(Gfx::Color c, Ui::Send<Gfx::Color> onChange, String title) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(colorInput(c, std::move(onChange)))
    );
}

export Ui::Child numberRow(f64 value, Ui::Send<f64> onChange, f64 step, String title) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(number(value, std::move(onChange), step))
    );
}

export Ui::Child tabRow(String title, Ui::Children tabs) {
    return rowContent(
        NONE,
        title,
        NONE,
        Some(tabbarContent(std::move(tabs)))
    );
}

export Ui::Child treeRow(Opt<Ui::Slot> leading, String title, Opt<String> subtitle, Ui::Slot child) {
    return Ui::state(false, [=](bool state, auto bind) {
        return vflow(
            0_au,
            pressableRow(
                Some(bind(not state)),
                leading(),
                title,
                subtitle,
                Some(Ui::icon(state ? Mdi::CHEVRON_UP : Mdi::CHEVRON_DOWN, 24_au))
            ),
            state ? child() |
                        Ui::insets({0_au, 0_au, 0_au, 0_au}) |
                        slideIn(Ui::SlideFrom::TOP) |
                        Ui::grow()
                  : Ui::empty()
        );
    });
}

export Ui::Child treeRow(Opt<Ui::Slot> leading, String title, Opt<String> subtitle, Ui::Slots children) {
    Ui::Slot slot = [children = std::move(children)] {
        return vflow(children());
    };
    return treeRow(std::move(leading), title, subtitle, std::move(slot));
}

} // namespace Karm::Kira
