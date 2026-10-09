export module Karm.Kira:empty;

import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

using namespace Karm::Math::Literals;
using namespace Karm::Literals;

namespace Karm::Kira {

export Ui::Child emptyTitle(Gfx::Icon icon, String text) {
    return Ui::vflow(
        0_au,
        Math::Align::CENTER,
        Ui::icon(icon, 48_au) | Ui::insets(16_au),
        Ui::titleLarge(text)
    );
}

export Ui::Child emptySubTitle(String text) {
    return Ui::titleMedium(text);
}

export Ui::Child emptyBody(String text) {
    return Ui::bodyMedium(text);
}

export Ui::Child emptyContent(Ui::Children children) {
    return Ui::vflow(
               6_au,
               Math::Align::CENTER,
               children
           ) |
           Ui::box({
               .foregroundFill = Ui::GRAY500,
           }) |
           Ui::center();
    ;
}

export Ui::Child emptyFooter(Ui::Children children) {
    return Ui::hflow(16_au, children) |
           Ui::insets({8_au, 0_au, 0_au, 0_au});
}

export Ui::Child emptyError(Gfx::Icon icon, String text, String body) {
    return emptyContent({
        emptyTitle(icon, "An error occurred."s),
        emptySubTitle(text),
        emptyBody(body),
    });
}

} // namespace Karm::Kira
