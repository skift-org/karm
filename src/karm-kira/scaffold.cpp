export module Karm.Kira:scaffold;

import Mdi;
import Karm.App;
import Karm.Ui;
import Karm.Gfx;
import Karm.Math;

import :titlebar;
import :toolbar;

namespace Karm::Kira {

export struct Scaffold : Meta::NoCopy {
    Gfx::Icon icon;
    String title;

    Opt<Ui::Slots> startTools = NONE;
    Opt<Ui::Slots> middleTools = NONE;
    Opt<Ui::Slots> endTools = NONE;
    Opt<Ui::Slot> sidebar = NONE;
    Ui::Slot body;

    Math::Vec2i size = {800, 600};

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
        body.pushBack(toolbar(scaffold.middleTools().unwrap()));

    if (s.sidebarOpen and scaffold.sidebar) {
        body.pushBack(
            (scaffold.sidebar().unwrap()) |
            Ui::grow()
        );
    } else {
        body.pushBack(scaffold.body() | Ui::grow());
    }

    Ui::Children tools;

    if (scaffold.sidebar)
        tools.pushBack(
            Ui::button(
                Scaffold::Model::bind<Scaffold::ToggleSidebar>(),
                Ui::ButtonStyle::subtle(),
                s.sidebarOpen
                    ? Mdi::MENU_OPEN
                    : Mdi::MENU
            ) |
            Ui::keyboardShortcut(App::Key::M, App::KeyMod::ALT)
        );

    if (scaffold.startTools)
        tools.pushBack(
            hflow(4, scaffold.startTools().unwrap())
        );

    if (scaffold.startTools and scaffold.endTools)
        tools.pushBack(Ui::grow(NONE));

    if (scaffold.endTools)
        tools.pushBack(
            hflow(4, scaffold.endTools().unwrap())
        );

    if (tools.len())
        body.pushBack(bottombar(tools));

    return Ui::vflow(body) |
           Ui::pinSize(Math::Vec2i{411, 731}) |
           Ui::dialogLayer() |
           Ui::popoverLayer();
}

static Ui::Child _desktopScaffoldToolbar(Scaffold::State const& s, Scaffold const& scaffold) {
    Ui::Children tools;
    if (scaffold.sidebar)
        tools.pushBack(
            button(
                Scaffold::Model::bind<Scaffold::ToggleSidebar>(),
                Ui::ButtonStyle::subtle(),
                s.sidebarOpen ? Mdi::MENU_OPEN : Mdi::MENU
            ) |
            Ui::keyboardShortcut(App::Key::M, App::KeyMod::ALT)
        );

    if (scaffold.startTools)
        tools.pushBack(
            hflow(4, scaffold.startTools().unwrap())
        );

    if (scaffold.middleTools)
        tools.pushBack(
            hflow(4, scaffold.middleTools().unwrap()) | Ui::grow()
        );
    else {
        tools.pushBack(Ui::labelMedium(scaffold.title) | Ui::center() | Ui::grow());
    }

    if (scaffold.endTools)
        tools.pushBack(
            hflow(4, scaffold.endTools().unwrap())
        );

    tools.pushBack(titlebarClose());

    if (tools.len())
        return toolbar(tools);

    return Ui::empty();
}

static Ui::Child _desktopScaffoldHeader(Scaffold::State const& s, Scaffold const& scaffold) {
    return _desktopScaffoldToolbar(s, scaffold) |
           contextMenu([] {
               return contextMenuContent({
                   contextMenuItem(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::NONE), Mdi::WINDOW_RESTORE, "Restore"),
                   contextMenuItem(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::FULL), Mdi::WINDOW_MAXIMIZE, "Maximize"),
                   contextMenuItem(Ui::bindBubble<App::RequestMinimizeEvent>(), Mdi::WINDOW_MINIMIZE, "Minimize"),
                   separator(),
                   contextMenuItem(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::LEFT), Mdi::DOCK_LEFT, "Snap Left"),
                   contextMenuItem(Ui::bindBubble<App::RequestSnapeEvent>(App::Snap::RIGHT), Mdi::DOCK_RIGHT, "Snap Right"),
                   separator(),
                   contextMenuItem(Ui::bindBubble<App::RequestCloseEvent>(), Mdi::WINDOW_CLOSE, "Close"),
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
                scaffold.sidebar().unwrap(),
                scaffold.body() | Ui::insets({0, 4, 4, 0}) | Ui::grow()
            ) |
            Ui::grow()
        );
    } else {
        body.pushBack(scaffold.body() | Ui::insets({0, 4, 4, 4}) | Ui::grow());
    }

    return Ui::vflow(body) |
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
                   .borderFill = Ui::GRAY800,
                   .backgroundFill = Ui::GRAY950,
               });
    };
}

} // namespace Karm::Kira
