export module Karm.Kira:toolbar;

import Karm.Math;
import Karm.Ui;

import :separator;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child toolbar(Ui::Children children) {
    return Ui::vflow(
        Ui::hflow(4_au, children) |
        Ui::insets(8_au)
    );
}

export Ui::Child bottombar(Ui::Children children) {
    return Ui::vflow(
        separator(),
        Ui::hflow(4_au, children) |
            Ui::insets(8_au)
    );
}

} // namespace Karm::Kira
