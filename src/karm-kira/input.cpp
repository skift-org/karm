export module Karm.Kira:input;

import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child input(Gfx::Icon icon, String placeholder, String text, Ui::Send<String> onChange) {
    return Ui::hflow(
               8_au,
               Math::Align::VCENTER | Math::Align::START,
               Ui::icon(icon),
               Ui::stack(
                   text ? Ui::empty() : Ui::labelMedium(Gfx::ZINC600, placeholder),
                   Ui::input(Ui::TextStyles::labelMedium(), text, onChange)
               ) | Ui::grow()
           ) |
           Ui::box({
               .padding = {6_au, 12_au, 6_au, 12_au},
               .borderRadii = 4,
               .borderWidth = 1,
               .borderFill = Some(Ui::GRAY700),
           }) |
           Ui::minSize({Ui::UNCONSTRAINED, 32_au}) |
           Ui::focusable();
}

export Ui::Child input(String placeholder, String text, Ui::Send<String> onChange) {
    return Ui::hflow(
               8_au,
               Math::Align::VCENTER | Math::Align::START,
               Ui::stack(
                   text ? Ui::empty() : Ui::labelMedium(Gfx::ZINC600, placeholder),
                   Ui::input(Ui::TextStyles::labelMedium(), text, onChange)
               ) | Ui::grow()
           ) |
           Ui::box({
               .padding = {6_au, 12_au, 6_au, 12_au},
               .borderRadii = 4,
               .borderWidth = 1,
               .borderFill = Some(Ui::GRAY700),
           }) |
           Ui::minSize({Ui::UNCONSTRAINED, 32_au}) |
           Ui::focusable();
}

} // namespace Karm::Kira
