export module Karm.Kira:dialog;

import Karm.Core;
import Karm.Ui;
import Karm.App;
import Karm.Gfx;
import Karm.Math;
import Mdi;

using namespace Karm::Math::Literals;
using namespace Karm::Literals;

namespace Karm::Kira {

export Ui::Child dialogContent(Ui::Children children) {
    Ui::BoxStyle const boxStyle = {
        .borderRadii = 8,
        .borderWidth = 1,
        .borderFill = Some(Ui::GRAY800),
        .backgroundFill = Some(Ui::GRAY900),
        .shadowStyle = Some(Gfx::BoxShadow::elevated(16))
    };

    return Ui::vflow(children) |
           Ui::box(boxStyle) |
           Ui::dragRegion() |
           Ui::align(Math::Align::CENTER | Math::Align::CLAMP) |
           Ui::insets(16_au);
}

export Ui::Child dialogTitleBar(String title) {
    return Ui::hflow(
               Ui::titleSmall(title) | Ui::vcenter(),
               Ui::grow(NONE),
               Ui::button(Some(Ui::closeDialog), Ui::ButtonStyle::subtle(), Mdi::CLOSE)
           ) |
           Ui::insets({4_au, 4_au, 4_au, 16_au});
}

export Ui::Child dialogHeader(Ui::Children children) {
    return Ui::vflow(8_au, children) |
           Ui::insets({16_au, 16_au, 8_au, 16_au});
}

export Ui::Child dialogBody(Ui::Children children) {
    return Ui::vflow(8_au, children) |
           Ui::insets({8_au, 16_au, 8_au, 16_au});
}

export Ui::Child dialogTitle(String text) {
    return Ui::titleMedium(text);
}

export Ui::Child dialogDescription(String text) {
    return Ui::bodySmall(Ui::GRAY400, text) |
           Ui::pinSize({380_au, Ui::UNCONSTRAINED});
}

export Ui::Child dialogFooter(Ui::Children children) {
    auto isMobile = App::formFactor == App::FormFactor::MOBILE;
    return Ui::flow(
               {
                   isMobile ? Math::Flow::TOP_TO_BOTTOM : Math::Flow::LEFT_TO_RIGHT,
                   Math::Align::FILL,
                   4_au,
               },
               children
           ) |
           Ui::insets({4_au, 8_au, 8_au, 8_au});
}

export Ui::Child dialogAction(Opt<Ui::Send<>> onPress, String text) {
    return Ui::button(
               Some([onPress = std::move(onPress)](auto& n) {
                   onPress(n);
                   Ui::closeDialog(n);
               }),
               Ui::ButtonStyle::primary(),
               text
           ) |
           Ui::keyboardShortcut(App::Key::ENTER);
}

export Ui::Child dialogCancel() {
    return Ui::button(
        Some(Ui::closeDialog),
        "Cancel"
    );
}

export Ui::Child alertDialog(String title, String description) {
    return dialogContent({
        dialogHeader({
            dialogTitle(title),
            dialogDescription(description),
        }),
        dialogFooter({
            Ui::grow(NONE),
            dialogAction(Some(Ui::SINK<>), "Ok"s),
        }),
    });
}

} // namespace Karm::Kira
