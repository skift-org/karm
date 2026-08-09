export module Karm.Kira:titlebar;

import Karm.App;
import Karm.Ui;
import Karm.Gfx;

import Mdi;

import :aboutDialog;
import :contextMenu;
import :separator;

namespace Karm::Kira {

export Ui::Child titlebarTitle(Gfx::Icon icon, String title, bool compact = false) {
    if (compact) {
        return Ui::button(
            Some([=](Ui::Node& n) {
                Ui::showDialog(n, aboutDialog(title));
            }),
            Ui::ButtonStyle::subtle(),
            icon
        );
    }

    return Ui::button(
        Some([=](auto& n) {
            Ui::showDialog(n, aboutDialog(title));
        }),
        Ui::ButtonStyle::subtle(), icon, title
    );
}

export Ui::Child titlebarClose() {
    return Ui::button(
        Some(Ui::bindBubble<App::RequestCloseEvent>()),
        Ui::ButtonStyle::subtle(),
        Mdi::WINDOW_CLOSE
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
                       contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::NONE)), Some(Mdi::WINDOW_RESTORE), "Restore"),
                       contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::FULL)), Some(Mdi::WINDOW_MAXIMIZE), "Maximize"),
                       contextMenuItem(Some(Ui::bindBubble<App::RequestMinimizeEvent>()), Some(Mdi::WINDOW_MINIMIZE), "Minimize"),
                       separator(),
                       contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::LEFT)), Some(Mdi::DOCK_LEFT), "Snap Left"),
                       contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::RIGHT)), Some(Mdi::DOCK_RIGHT), "Snap Right"),
                       separator(),
                       contextMenuItem(Some(Ui::bindBubble<App::RequestCloseEvent>()), Some(Mdi::WINDOW_CLOSE), "Close"),
                   });
               });
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
