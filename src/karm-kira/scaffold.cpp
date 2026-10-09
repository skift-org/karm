export module Karm.Kira:scaffold;

import Mdi;
import Karm.App;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;

import :toolbar;
import :aboutDialog;
import :contextMenu;

using namespace Karm::Math::Literals;
using namespace Karm::Fmt::Literals;

namespace Karm::Kira {

export struct Scaffold : Meta::NoCopy {
    Gfx::Icon icon;
    String title;

    Opt<Ui::Slots> startTools = NONE;
    Opt<Ui::Slots> middleTools = NONE;
    Opt<Ui::Slots> endTools = NONE;
    Opt<Ui::Slot> sidebar = NONE;
    Ui::Slot body;

    Math::Vec2Au size = {800_au, 600_au};

    struct State {
        bool sidebarOpen = false;
    };

    struct ToggleSidebar {};

    using Action = Union<ToggleSidebar>;

    static Ui::Task<Action> reduce(State& s, Action a) {
        if (a.is<ToggleSidebar>()) {
            s.sidebarOpen = !s.sidebarOpen;
        }

        return NONE;
    }

    using Model = Ui::Model<State, Action, reduce>;
};

static Ui::Child _mobileScaffold(Scaffold::State const& s, Scaffold const& scaffold) {
    Ui::Children body;

    if (scaffold.middleTools)
        body.pushBack(toolbar(scaffold.middleTools().expect()));

    if (s.sidebarOpen and scaffold.sidebar) {
        body.pushBack(
            (scaffold.sidebar().expect()) |
            Ui::grow()
        );
    } else {
        body.pushBack(Ui::reactive(scaffold.body) | Ui::grow());
    }

    Ui::Children tools;

    if (scaffold.sidebar)
        tools.pushBack(
            Ui::button(
                Some(Scaffold::Model::bind<Scaffold::ToggleSidebar>()),
                Ui::ButtonStyle::subtle(),
                s.sidebarOpen
                    ? Mdi::MENU_OPEN
                    : Mdi::MENU
            ) |
            Ui::keyboardShortcut(App::Key::M, App::KeyMod::ALT)
        );

    if (scaffold.startTools)
        tools.pushBack(
            hflow(4_au, scaffold.startTools().expect())
        );

    if (scaffold.startTools and scaffold.endTools)
        tools.pushBack(Ui::grow(NONE));

    if (scaffold.endTools)
        tools.pushBack(
            hflow(4_au, scaffold.endTools().expect())
        );

    if (tools.len())
        body.pushBack(bottombar(tools));

    return Ui::vflow(body) |
           Ui::pinSize(Math::Vec2Au{411_au, 731_au}) |
           Ui::dialogLayer() |
           Ui::popoverLayer();
}

static Ui::Child titlebarClose() {
    return Ui::button(
        Some(Ui::bindBubble<App::RequestCloseEvent>()),
        Ui::ButtonStyle::subtle(),
        Mdi::WINDOW_CLOSE
    );
}

static Ui::Child _desktopScaffoldToolbar(Scaffold::State const& s, Scaffold const& scaffold) {
    Ui::Children tools;
    if (scaffold.sidebar)
        tools.pushBack(
            button(
                Some(Scaffold::Model::bind<Scaffold::ToggleSidebar>()),
                Ui::ButtonStyle::subtle(),
                s.sidebarOpen ? Mdi::MENU_OPEN : Mdi::MENU
            ) |
            Ui::keyboardShortcut(App::Key::M, App::KeyMod::ALT)
        );

    if (scaffold.startTools)
        tools.pushBack(
            hflow(4_au, scaffold.startTools().expect())
        );

    if (scaffold.middleTools)
        tools.pushBack(
            hflow(4_au, scaffold.middleTools().expect()) | Ui::grow()
        );
    else {
        tools.pushBack(Ui::labelMedium(scaffold.title) | Ui::center() | Ui::grow());
    }

    if (scaffold.endTools)
        tools.pushBack(
            hflow(4_au, scaffold.endTools().expect())
        );

    tools.pushBack(titlebarClose());

    if (tools.len())
        return toolbar(tools);

    return Ui::empty();
}

static Ui::Child _desktopScaffoldHeader(Scaffold::State const& s, Scaffold const& scaffold) {
    return _desktopScaffoldToolbar(s, scaffold) |
           Ui::doubleClick(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::FULL)) |
           contextMenu([&] {
               return contextMenuContent({
                   contextMenuItem(
                       Some([&](Ui::Node& n) {
                           Ui::showDialog(n, aboutDialog(scaffold.title));
                       }),
                       Some(scaffold.icon),
                       "About {}..."_f(scaffold.title)
                   ),
                   separator(),
                   contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::NONE)), Some(Mdi::WINDOW_RESTORE), "Restore"),
                   contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::FULL)), Some(Mdi::WINDOW_MAXIMIZE), "Maximize"),
                   contextMenuItem(Some(Ui::bindBubble<App::RequestMinimizeEvent>()), Some(Mdi::WINDOW_MINIMIZE), "Minimize"),
                   separator(),
                   contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::LEFT)), Some(Mdi::DOCK_LEFT), "Snap Left"),
                   contextMenuItem(Some(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::RIGHT)), Some(Mdi::DOCK_RIGHT), "Snap Right"),
                   separator(),
                   contextMenuItem(Some(Ui::bindBubble<App::RequestCloseEvent>()), Some(Mdi::WINDOW_CLOSE), "Close"),
               });
           }) |
           Ui::dragRegion();
}

static Ui::Child _desktopScaffold(Scaffold::State const& s, Scaffold const& scaffold) {
    Ui::Children body;
    body.pushBack(_desktopScaffoldHeader(s, scaffold));

    if (s.sidebarOpen and scaffold.sidebar) {
        body.pushBack(
            hflow(
                scaffold.sidebar().expect(),
                Ui::reactive(scaffold.body) | Ui::insets({0_au, 4_au, 4_au, 0_au}) | Ui::grow()
            ) |
            Ui::grow()
        );
    } else {
        body.pushBack(Ui::reactive(scaffold.body) | Ui::insets({0_au, 4_au, 4_au, 4_au}) | Ui::grow());
    }

    return Ui::vflow(body) |
           Ui::resizeRegion(8_au) |
           Ui::pinSize(scaffold.size) |
           Ui::dialogLayer() |
           Ui::popoverLayer();
}

export Ui::Child scaffold(Scaffold scaffold) {
    auto isMobile = App::formFactor == App::FormFactor::MOBILE;

    Scaffold::State state{
        .sidebarOpen = not isMobile,
    };

    return Ui::reducer<Scaffold::Model>(state, [scaffold = std::move(scaffold)](Scaffold::State const& state) {
        return App::formFactor == App::FormFactor::MOBILE
                   ? _mobileScaffold(state, scaffold)
                   : _desktopScaffold(state, scaffold);
    });
}

export auto scaffoldContent() {
    return [](Ui::Child child) {
        return child |
               Ui::bound() |
               Ui::box({
                   .borderRadii = 6,
                   .borderWidth = 1,
                   .borderFill = Some(Ui::GRAY800),
                   .backgroundFill = Some(Ui::GRAY950),
                   .overflow = Ui::BoxOverflow::HIDDEN,
               });
    };
}

} // namespace Karm::Kira
