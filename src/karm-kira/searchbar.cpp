export module Karm.Kira:searchbar;

import Karm.Core;
import Karm.App;
import Karm.Ui;
import Karm.Math;

import Mdi;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child searchbar(String text, Ui::Send<String> onChange = Ui::SINK<String>) {
    return Ui::hflow(
               8_au,
               Math::Align::VCENTER | Math::Align::START,
               Ui::stack(
                   text ? Ui::empty() : Ui::labelMedium(Ui::GRAY600, "Search…"),
                   Ui::input(Ui::TextStyles::labelMedium(), text, onChange)
               ) | Ui::grow(),
               Ui::icon(Mdi::MAGNIFY)
           ) |
           Ui::box({
               .padding = {6_au, 12_au, 6_au, 12_au},
               .borderRadii = 4,
               .backgroundFill = Some(Ui::GRAY800),
           }) |
           Ui::minSize({Ui::UNCONSTRAINED, 32_au}) |
           Ui::focusable() |
           Ui::keyboardShortcut(App::Key::F, App::KeyMod::CTRL);
}

} // namespace Karm::Kira
