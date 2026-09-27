export module Karm.Kira:empty;

import Karm.Ui;
import Karm.Gfx;
import Karm.Math;
import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Kira {

export Ui::Child emptyTitle(Gfx::Icon icon, String text) {
    return Ui::vflow(
        0,
        Math::Align::CENTER,
        Ui::icon(icon, 48) | Ui::insets(16),
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
               6,
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
    return Ui::hflow(16, children) |
           Ui::insets({8, 0, 0, 0});
}

export Ui::Child emptyError(Gfx::Icon icon, String text, String body) {
    return emptyContent({
        emptyTitle(icon, "An error occurred."s),
        emptySubTitle(text),
        emptyBody(body),
    });
}

} // namespace Karm::Kira
