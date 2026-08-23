export module Karm.Kira:tabbar;

import Karm.Ui;
import Karm.Gfx;

import :separator;

namespace Karm::Kira {

export Ui::Child tabbarWrapper(Ui::Children children) {
    return Ui::hscroll(
        Ui::hflow(2, children)
    );
}

export Ui::Child tabbarContent(Ui::Children children) {
    return tabbarWrapper(children) |
           Ui::box({
               .padding = 1,
               .borderRadii = 6,
               .borderWidth = 1,
               .borderFill = Some(Ui::GRAY800),
               .backgroundFill = Some(Ui::GRAY800),
           });
}

export Ui::Child tabbarItem(bool selected, Ui::Send<> onSelect, Ui::Child content) {
    content = content | Ui::insets({0, 8}) |
              Ui::center() |
              Ui::minSize({Ui::UNCONSTRAINED, 30});

    if (not selected) {
        return Ui::button(
            Some(onSelect),
            Ui::ButtonStyle::subtle(),
            content
        );
    }

    return Ui::box(
        {
            .borderRadii = Ui::ButtonStyle::RADIUS,
            .backgroundFill = Some(Ui::GRAY900),
        },
        content | Ui::bound()
    );
}

export Ui::Child tabarItemLabel(Opt<Gfx::Icon> icon, String text) {
    if (auto& [i] = icon)
        return Ui::hflow(8, Math::Align::CENTER, Ui::icon(i), Ui::labelMedium(text));
    return Ui::labelMedium(text);
}

export Ui::Child tabarItemIcon(Gfx::Icon icon) {
    return Ui::icon(icon);
}

} // namespace Karm::Kira
