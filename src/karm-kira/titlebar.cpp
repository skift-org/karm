export module Karm.Kira:titlebar;

import Karm.App;
import Karm.Ui;
import Karm.Gfx;

import Mdi;

import :aboutDialog;
import :contextMenu;

namespace Karm::Kira {

export Ui::Child titlebarTitle(Gfx::Icon icon, String title, bool compact = false) {
    if (compact) {
        return Ui::button(
            [=](Ui::Node& n) {
                Ui::showDialog(n, aboutDialog(title));
            },
            Ui::ButtonStyle::subtle(),
            icon
        );
    }

    return Ui::button(
        [=](auto& n) {
            Ui::showDialog(n, aboutDialog(title));
        },
        Ui::ButtonStyle::subtle(), icon, title
    );
}

export Ui::Child titlebarClose() {
    return Ui::hflow(
        4,
        Ui::button(
            Ui::bindBubble<App::RequestCloseEvent>(),
            Ui::ButtonStyle::subtle(),
            Mdi::WINDOW_CLOSE
        )
    );
}

export struct TitlebarContent {
    Ui::Child start;
    Ui::Child middle = Ui::empty();
    Ui::Child end;

    operator Ui::Child() const {
        return Ui::hflow(
                   4,
                   start | Ui::insets({8, 0}),
                   middle | Ui::grow(),
                   end | Ui::insets({8, 0})
               ) |
               Ui::insets({0, 8}) |
               contextMenu([] {
                   return contextMenuContent({
                       contextMenuItem(Ui::bindBubble<App::RequestMinimizeEvent>(), Mdi::WINDOW_MINIMIZE, "Minimize"),
                       contextMenuItem(Ui::bindBubble<App::RequestMaximizeEvent>(), Mdi::WINDOW_MAXIMIZE, "Maximize"),
                       separator(),
                       contextMenuItem(Ui::bindBubble<App::RequestCloseEvent>(), Mdi::WINDOW_CLOSE, "Close"),
                   });
               }) |
               Ui::dragRegion();
    }
};

export Ui::Child titlebar(Gfx::Icon icon, String title) {
    return TitlebarContent{
        .start = titlebarTitle(icon, title),
        .end = titlebarClose()
    };
}

export Ui::Child titlebar(Gfx::Icon icon, String title, Ui::Child middle) {
    return TitlebarContent{
        .start = titlebarTitle(icon, title),
        .middle = middle,
        .end = titlebarClose(),
    };
}

} // namespace Karm::Kira
