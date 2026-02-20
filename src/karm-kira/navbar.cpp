export module Karm.Kira:navbar;

import Karm.Core;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;

import :separator;

namespace Karm::Kira {

export Ui::Child navbarContent(Ui::Children children) {
    return Ui::hflow(
               4,
               children
           ) |
           Ui::box({
               .margin = 8,
               .padding = 8,
               .borderRadii = 99,
               .backgroundFill = Ui::GRAY800,
           }) |
           Ui::center();
}

export Ui::Child navbarItem(Opt<Ui::Send<>> onPress, Gfx::Icon icon, Str text, bool selected) {
    return Ui::button(
               std::move(onPress),
               selected
                   ? Ui::ButtonStyle::regular().withForegroundFill(Ui::ACCENT500).withRadii(99)
                   : Ui::ButtonStyle::subtle().withRadii(99),
               Ui::vflow(
                   0,
                   Math::Align::CENTER,
                   Ui::icon(icon),
                   Ui::empty(4),
                   Ui::labelSmall(text)
               ) |
                   Ui::insets({8, 6, 4, 6})
           ) |
           Ui::minSize({96, Ui::UNCONSTRAINED}) |
           Ui::grow();
}

} // namespace Karm::Kira
