export module Karm.Kira:handle;

import Karm.Core;
import Karm.Math;
import Karm.Ui;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

export Ui::Child handle() {
    return Ui::empty({128_au, 4_au}) |
           Ui::box({
               .borderRadii = 999,
               .backgroundFill = Some(Ui::GRAY50),
           }) |
           Ui::insets(12_au) |
           Ui::center();
}

export Ui::Child dragHandle() {
    return handle() | Ui::dragRegion();
}

export Ui::Child buttonHandle(Opt<Ui::Send<>> press) {
    return handle() |
           Ui::button(std::move(press), Ui::ButtonStyle::none());
}

} // namespace Karm::Kira
