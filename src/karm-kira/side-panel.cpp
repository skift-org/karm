export module Karm.Kira:sidePanel;

import Karm.Core;
import Karm.Math;
import Karm.Ui;
import Mdi;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child sidePanelContent(Ui::Children children) {
    return Ui::vflow(children) |
           Ui::pinSize({128_au, Ui::UNCONSTRAINED});
}

export Ui::Child sidePanelTitle(Str title) {
    return Ui::hflow(
               Ui::labelLarge(title),
               Ui::grow(NONE)
           ) |
           Ui::insets(6_au);
}

export Ui::Child sidePanelTitle(Opt<Ui::Send<>> onClose, Str title) {
    return Ui::hflow(
               Ui::labelLarge(title),
               Ui::grow(NONE),
               Ui::button(
                   onClose,
                   Ui::ButtonStyle::subtle(),
                   Ui::icon(Mdi::CLOSE) | Ui::center()
               )
           ) |
           Ui::insets(6_au);
}

} // namespace Karm::Kira
