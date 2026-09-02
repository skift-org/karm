export module Karm.Kira:contextMenu;

import Karm.App;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;

import :checkbox;

namespace Karm::Kira {

export void showContextMenu(Ui::Node& n, Math::Vec2i at, Ui::Child menu) {
    if (App::formFactor == App::FormFactor::DESKTOP) {
        Ui::showPopover(n, at, menu);
    } else {
        Ui::showDialog(n, menu | Ui::center());
    }
}

struct ContextMenu : Ui::ProxyNode<ContextMenu> {
    Ui::Slot _menu;

    ContextMenu(Ui::Child child, Ui::Slot menu)
        : Ui::ProxyNode<ContextMenu>(child),
          _menu(std::move(menu)) {
    }

    void reconcile(ContextMenu& o) override {
        Ui::ProxyNode<ContextMenu>::reconcile(o);
        _menu = std::move(o._menu);
    }

    void event(App::Event& event) override {
        Ui::ProxyNode<ContextMenu>::event(event);

        if (event.accepted())
            return;

        if (auto e = event.is<App::MouseEvent>()) {
            if (e->type == App::MouseEvent::PRESS and
                e->button == App::MouseButton::RIGHT and
                bound().contains(e->pos)) {
                showContextMenu(*this, e->pos, _menu());
                event.accept();
            }
        }
    }
};

export Ui::Child contextMenu(Ui::Child child, Ui::Slot menu) {
    return makeRc<ContextMenu>(child, std::move(menu));
}

export auto contextMenu(Ui::Slot menu) {
    return [menu = std::move(menu)](Ui::Child child) mutable {
        return contextMenu(child, std::move(menu));
    };
}

export Ui::Child contextMenuContent(Ui::Children children) {
    return Ui::vflow(
               children
           ) |
           Ui::minSize({200, Ui::UNCONSTRAINED}) |
           Ui::box({
               .margin = 4,
               .borderRadii = 6,
               .borderWidth = 1,
               .borderFill = Some(Ui::GRAY800),
               .backgroundFill = Some(Ui::GRAY900),
               .shadowStyle = Some(Gfx::BoxShadow::elevated(4)),
           }) |
           Ui::scaleIn();
}

export Ui::Child contextMenuItem(Opt<Ui::Send<>> onPress, Opt<Gfx::Icon> i, Str t) {
    return Ui::hflow(
               12,
               Math::Align::CENTER,
               i ? Ui::icon(*i) : Ui::empty(18),
               Ui::text(t)
           ) |
           Ui::insets({6, 6, 6, 10}) |
           Ui::minSize({Ui::UNCONSTRAINED, 32}) |
           Ui::button(
               onPress ? Opt<Ui::Send<>>(Some([onPress = std::move(onPress)](auto& n) {
                   onPress(n);
                   Ui::closePopover(n);
               }))
                       : Ui::DISABLED<>,
               Ui::ButtonStyle::subtle()
           ) |
           Ui::insets(4);
}

export Ui::Child contextMenuCheck(Opt<Ui::Send<>> onPress, bool checked, Str t) {
    return Ui::hflow(
               12,
               Math::Align::CENTER,
               checkbox(checked, Ui::SINK<bool>),
               Ui::text(t)
           ) |
           Ui::insets({6, 6, 6, 10}) |
           Ui::minSize({Ui::UNCONSTRAINED, 32}) |
           Ui::button(
               Some([onPress = std::move(onPress)](auto& n) {
                   onPress(n);
                   Ui::closePopover(n);
               }),
               Ui::ButtonStyle::subtle()
           ) |
           Ui::insets(4);
}

export Ui::Child contextMenuDock(Ui::Children children) {
    return Ui::hflow(
               2,
               Math::Align::CENTER,
               children
           ) |
           Ui::insets(4);
}

export Ui::Child contextMenuIcon(Opt<Ui::Send<>> onPress, Gfx::Icon i) {
    if (onPress) {
        onPress = Some([onPress = std::move(onPress)](auto& n) {
            onPress(n);
            Ui::closePopover(n);
        });
    }

    return Ui::button(
        std::move(onPress),
        Ui::ButtonStyle::subtle(),
        i
    );
}

} // namespace Karm::Kira
