export module Karm.Ui:intent;

import Karm.Core;
import Karm.App;
import :node;
import :funcs;

namespace Karm::Ui {

export struct KeyboardShortcutActivated {};

struct Intent : ProxyNode<Intent> {
    Send<App::Event&> _map;

    Intent(Send<App::Event&> map, Child child)
        : ProxyNode<Intent>(std::move(child)), _map(std::move(map)) {}

    void reconcile(Intent& o) override {
        _map = std::move(o._map);
        ProxyNode<Intent>::reconcile(o);
    }

    void event(App::Event& e) override {
        if (e.accepted())
            return;
        _map(*this, e);
        ProxyNode<Intent>::event(e);
    }
};

export Child intent(Send<App::Event&> filter, Child child) {
    return makeRc<Intent>(filter, std::move(child));
}

export auto intent(Send<App::Event&> filter) {
    return [filter](Child child) mutable {
        return intent(filter, std::move(child));
    };
}

export auto keyboardShortcut(App::Key key, Flags<App::KeyMod> mods, Send<> onPress) {
    return intent([=](Ui::Node& n, App::Event& e) {
        if (auto it = e.is<App::KeyboardEvent>();
            it and it->type == App::KeyboardEvent::PRESS and it->key == key and match(it->mods, mods)) {
            onPress(n);
            e.accept();
        }
    });
}

export auto keyboardShortcut(App::Key key, Send<> onPress) {
    return keyboardShortcut(
        key, {}, onPress
    );
}

export auto keyboardShortcut(App::Key key, Flags<App::KeyMod> mods = {}) {
    return keyboardShortcut(
        key, mods, [=](Ui::Node& n) {
            event<KeyboardShortcutActivated>(n);
        }
    );
}

export auto doubleClick(Send<> onDoubleClick) {
    return intent([=](Ui::Node& n, App::Event& e) {
        if (auto me = e.is<App::MouseEvent>();
            me and n.bound().contains(me->pos) and me->type == App::MouseEvent::PRESS and me->clicks == 2) {
            onDoubleClick(n);
            e.accept();
        }
    });
}

} // namespace Karm::Ui
