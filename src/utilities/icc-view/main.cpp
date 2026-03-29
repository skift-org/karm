#include <karm/entry>

import Mdi;
import Karm.Kira;
import Karm.Ui;
import Karm.Icc;
import Karm.Logger;

using namespace Karm;

struct ProfileItem {
    String name;
    Rc<Icc::ColorProfile> profile;
};

struct State {
    Vec<ProfileItem> profiles = {};
    usize selected = 0;
    Opt<Icc::ProfileClass> filter = NONE;
};

struct SelectProfile {
    usize index;
};

struct SetProfileFilter {
    Opt<Icc::ProfileClass> profileClass;
};

using Action = Union<SelectProfile, SetProfileFilter>;

bool matchesProfileFilter(ProfileItem const& profile, Opt<Icc::ProfileClass> filter) {
    if (not filter)
        return true;

    auto profileClass = profile.profile->profileDeviceClass();
    return profileClass._name == filter.unwrap();
}

Opt<usize> firstVisibleProfile(State const& state) {
    for (usize index = 0; index < state.profiles.len(); index++) {
        if (matchesProfileFilter(state.profiles[index], state.filter))
            return index;
    }

    return NONE;
}

Ui::Task<Action> reduce(State& state, Action action) {
    action.visit(Visitor{
        [&](SelectProfile a) {
            state.selected = a.index;
        },
        [&](SetProfileFilter a) {
            state.filter = a.profileClass;

            if (state.profiles.len() == 0)
                return;

            if (matchesProfileFilter(state.profiles[state.selected], state.filter))
                return;

            if (auto first = firstVisibleProfile(state))
                state.selected = first.unwrap();
        },
    });
    return NONE;
}

using Model = Ui::Model<State, Action, reduce>;

auto profileIcon(Opt<Icc::ProfileClass> profileClass) {
    if (not profileClass)
        return Mdi::FILE;

    if (profileClass == Icc::ProfileClass::INPUT_DEVICE)
        return Mdi::CAMERA;

    if (profileClass == Icc::ProfileClass::DISPLAY_DEVICE)
        return Mdi::LAPTOP;

    if (profileClass == Icc::ProfileClass::OUTPUT_DEVICE)
        return Mdi::PRINTER;

    if (profileClass == Icc::ProfileClass::DEVICE_LINK)
        return Mdi::TUNE;

    if (profileClass == Icc::ProfileClass::COLOR_SPACE)
        return Mdi::PALETTE_SWATCH;

    if (profileClass == Icc::ProfileClass::ABSTRACT)
        return Mdi::SHAPE;

    if (profileClass == Icc::ProfileClass::NAMED_COLOR)
        return Mdi::PALETTE;

    return Mdi::FILE;
}

Res<Vec<ProfileItem>> loadAll() {
    Vec<ProfileItem> profiles;
    for (auto& bundle : try$(Sys::Bundle::installed())) {
        auto maybeDir = Sys::Dir::open(bundle.url() / "/public/color-profiles");
        if (not maybeDir)
            continue;
        for (auto& entry : maybeDir.unwrap().entries()) {
            if (entry.type != Sys::Type::FILE)
                continue;

            auto profileUrl = maybeDir.unwrap().url() / entry.name;
            if (profileUrl.path.suffix() != "icc")
                continue;

            auto res = Icc::ColorProfile::from(profileUrl);
            if (not res) {
                logWarn("could not load icc profile from {}: {}", profileUrl, res.error());
                continue;
            }

            profiles.pushBack({
                .name = Io::format("{}", Ref::Path::parse(entry.name).basename()),
                .profile = res.unwrap(),
            });
        }
    }
    return Ok(std::move(profiles));
}

Ui::Child profileNavitem(State const& s, ProfileItem const& profile, usize index) {
    return Kr::sidenavItem(
        s.selected == index,
        Model::bind<SelectProfile>(index),
        profileIcon(profile.profile->profileDeviceClass()),
        profile.name
    );
}

String profileClassText(Icc::ProfileClass profileClass) {
    if (profileClass == Icc::ProfileClass::INPUT_DEVICE)
        return "Input Device"s;

    if (profileClass == Icc::ProfileClass::DISPLAY_DEVICE)
        return "Display Device"s;

    if (profileClass == Icc::ProfileClass::OUTPUT_DEVICE)
        return "Output Device"s;

    if (profileClass == Icc::ProfileClass::DEVICE_LINK)
        return "Device Link"s;

    if (profileClass == Icc::ProfileClass::COLOR_SPACE)
        return "Color Space"s;

    if (profileClass == Icc::ProfileClass::ABSTRACT)
        return "Abstract"s;

    if (profileClass == Icc::ProfileClass::NAMED_COLOR)
        return "Named Color"s;

    return "Unknown"s;
}

String profileFilterText(Opt<Icc::ProfileClass> profileClass) {
    if (profileClass == NONE)
        return "All color profiles"s;
    return profileClassText(profileClass.unwrap());
}

Ui::Child profileNavbar(State const& s) {
    Ui::Children items;
    items.pushBack(
        Kr::select(
            Kr::selectValue(profileFilterText(s.filter)),
            [] -> Ui::Children {
                return {
                    Kr::selectItem(Model::bind<SetProfileFilter>(NONE), profileFilterText(NONE)),
                    Kr::separator(),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::INPUT_DEVICE), profileFilterText(Icc::ProfileClass::INPUT_DEVICE)),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::DISPLAY_DEVICE), "Display Device"s),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::OUTPUT_DEVICE), "Output Device"s),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::DEVICE_LINK), "Device Link"s),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::COLOR_SPACE), "Color Space"s),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::ABSTRACT), "Abstract"s),
                    Kr::selectItem(Model::bind<SetProfileFilter>(Icc::ProfileClass::NAMED_COLOR), "Named Color"s),
                };
            }
        )
    );

    bool hasMatchingProfile = false;

    for (usize index = 0; index < s.profiles.len(); index++) {
        auto const& profile = s.profiles[index];
        if (not matchesProfileFilter(profile, s.filter))
            continue;

        hasMatchingProfile = true;
        items.pushBack(profileNavitem(s, profile, index));
    }

    if (not hasMatchingProfile) {
        items.pushBack(
            Ui::bodySmall(Ui::GRAY500, "No color profiles."s) | Ui::center()
        );
    }

    return Kr::sidenavContent(std::move(items));
}

Ui::Child infoRow(String title, String value) {
    return Ui::hflow(
        Ui::bodyMedium(title) | Ui::grow(),
        Ui::bodyMedium(value)
    );
}

Ui::Child profileDetails(Rc<Icc::ColorProfile> const& profile) {
    return Ui::vflow(
               Ui::titleMedium("Profile Information"s),
               Ui::empty(8),
               infoRow("Profile Type"s, profileClassText(profile->profileDeviceClass())),
               infoRow("Color Space"s, Io::format("{}", profile->colorSpace())),
               infoRow("Connection Space"s, Io::format("{}", profile->profileConnectionSpace())),
               infoRow("Device Dependent"s, profile->isDeviceDependent() ? "Yes"s : "No"s)
           ) |
           Ui::insets(16);
}

Ui::Child appContent(State const& s) {
    if (s.profiles.len() == 0)
        return Ui::bodyMedium(Ui::GRAY500, "No ICC profile found."s) | Ui::center();

    if (not firstVisibleProfile(s))
        return Ui::bodyMedium(Ui::GRAY500, "No profile matches this filter."s) | Ui::center();

    if (s.selected >= s.profiles.len())
        return Ui::bodyMedium(Ui::GRAY500, "No profile selected."s) | Ui::center();

    if (not matchesProfileFilter(s.profiles[s.selected], s.filter))
        return Ui::bodyMedium(Ui::GRAY500, "No profile selected."s) | Ui::center();

    return profileDetails(s.profiles[s.selected].profile);
}

Ui::Child app(Vec<ProfileItem> profiles) {
    return Ui::reducer<Model>({std::move(profiles)}, [](State const& s) {
        return Kr::scaffold({
            .icon = Mdi::PALETTE,
            .title = "Color Profiles"s,
            .sidebar = [&] {
                return profileNavbar(s);
            },
            .body = [&] {
                return appContent(s) | Kr::scaffoldContent();
            },
        });
    });
}

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto profiles = co_try$(loadAll());
    co_return co_await Ui::runAsync(
        env,
        app(std::move(profiles)),
        ct
    );
}
