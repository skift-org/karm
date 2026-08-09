#include <karm/entry>

import Mdi;
import Karm.Kira;
import Karm.Ui;
import Karm.Gfx;
import Karm.Logger;
import Karm.Glob;
import Karm.Signals;
import Karm.Core;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

struct IconMetadata {
    String name;
    String path;
};

struct State : Meta::Pinned {
    Signals::Signal<Vec<IconMetadata>> icons;
    Signals::Signal<String> searchQuery;
    Signals::Signal<Opt<String>> selectedName;
    Signals::Computed<Vec<IconMetadata>> filtered = [icons = this->icons, searchQuery = this->searchQuery] {
        Vec<Tuple<IconMetadata, int>> matches;
        for (auto l : *icons) {
            if (not searchQuery.value()) {
                matches.pushBack({l, {}});
                continue;
            }

            auto match = Glob::matchFuzzy(l.name, searchQuery.value());
            if (not match)
                continue;
            matches.pushBack({l, match->score});
        }

        if (searchQuery.value())
            sort(matches, [](auto& a, auto& b) {
                return b.v1 <=> a.v1;
            });

        return iter(matches) |
               Select([](auto& m) {
                   return m.v0;
               }) |
               Collect<Vec<IconMetadata>>();
    };
    Signals::Computed<Opt<IconMetadata>> selected = [filtered = this->filtered, selectedName = this->selectedName] -> Opt<IconMetadata> {
        if (not selectedName.value())
            return NONE;
        return iter(filtered.value()) | FindFirst([&](auto& i) {
                   return i.name == selectedName.value().unwrap();
               });
    };

    State(Vec<IconMetadata> icons)
        : icons(std::move(icons)) {}
};

Ui::Child iconGrid(State const& s) {
    return Ui::grid(
               Ui::GridStyle::simpleFixed(
                   {((isize)s.filtered.value().len() / 8) + 1, 48},
                   {8, 48}, 4
               ),
               iter(s.filtered.value()) | Select([&](IconMetadata const& i) {
                   return Ui::icon(Gfx::Icon{i.path, 24}, 48) |
                          Ui::center() |
                          Ui::bound() |
                          Ui::button(
                              Some(Ui::bind(s.selectedName, Some(i.name))),
                              i.name == s.selectedName.value() ? Ui::ButtonStyle::regular() : Ui::ButtonStyle::subtle()
                          );
               }) | Collect<Ui::Children>()
           ) |
           Ui::vscroll();
}

Ui::Child iconDetails(IconMetadata const& metadata) {
    return Ui::vflow(
               4,
               Ui::hflow(
                   4,
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 18),
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 24),
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 48),
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 96)
               ),
               Ui::titleMedium(metadata.name)
           ) |
           Ui::pinSize({320, Ui::UNCONSTRAINED});
}

Ui::Child app(State& s) {
    return Ui::reactive([&] mutable {
        return Kr::scaffold({
            .icon = Mdi::PALETTE,
            .title = "Icons"s,
            .body = [&] {
                auto content = iconGrid(s) |
                               Kr::scaffoldContent() |
                               Ui::grow();

                if (auto& [metadata] = s.selected.value()) {
                    content = Ui::hflow(4, content, iconDetails(metadata) | Kr::scaffoldContent()) |
                              Ui::grow();
                }

                return Ui::vflow(
                    4,
                    Kr::searchbar(
                        s.searchQuery.value(),
                        Ui::bind(s.searchQuery)
                    ),
                    content
                );
            },
        });
    });
}

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto data = co_try$(Sys::readAllText<Utf8>("bundle://mdi/icons.json"_url));
    auto icons = co_try$(Json::parse(data));

    Vec<IconMetadata> metadatas;
    for (auto icon : icons.asArray()) {
        metadatas.pushBack({
            icon.get("name"s).asStr(),
            icon.get("path"s).asStr(),
        });
    }
    State state{metadatas};
    co_return co_await Ui::runAsync(
        env,
        app(state),
        ct
    );
}
