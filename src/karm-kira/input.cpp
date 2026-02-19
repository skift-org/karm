export module Karm.Kira:input;

import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

namespace Karm::Kira {

export Ui::Child input(Gfx::Icon icon, String placeholder, String text, Ui::Send<String> onChange) {
    return Ui::hflow(
               8,
               Math::Align::VCENTER | Math::Align::START,
               Ui::icon(icon),
               Ui::stack(
                   text ? Ui::empty() : Ui::labelMedium(Gfx::ZINC600, placeholder),
                   Ui::input(Ui::TextStyles::labelMedium(), text, onChange)
               ) | Ui::grow()
           ) |
           Ui::box({
               .padding = {6, 12, 6, 12},
               .borderRadii = 4,
               .borderWidth = 1,
               .borderFill = Ui::GRAY800,
           }) |
           Ui::minSize({Ui::UNCONSTRAINED, 32}) |
           Ui::focusable();
}

export Ui::Child input(String placeholder, String text, Ui::Send<String> onChange) {
    return Ui::hflow(
               8,
               Math::Align::VCENTER | Math::Align::START,
               Ui::stack(
                   text ? Ui::empty() : Ui::labelMedium(Gfx::ZINC600, placeholder),
                   Ui::input(Ui::TextStyles::labelMedium(), text, onChange)
               ) | Ui::grow()
           ) |
           Ui::box({
               .padding = {6, 12, 6, 12},
               .borderRadii = 4,
               .borderWidth = 1,
               .borderFill = Ui::GRAY800,
           }) |
           Ui::minSize({Ui::UNCONSTRAINED, 32}) |
           Ui::focusable();
}

} // namespace Karm::Kira
