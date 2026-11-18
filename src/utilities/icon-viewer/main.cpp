#include <karm/entry>
#include <sys/stat.h>

import Mdi;
import Karm.Kira;
import Karm.Math;
import Karm.Ui;
import Karm.Gfx;
import Karm.Logger;
import Karm.Glob;
import Karm.Core;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Math::Literals;
using namespace Karm::Ref::Literals;

struct IconMetadata {
    String name;
    String path;
};

struct State {
    Vec<IconMetadata> icons;
    String searchQuery = ""s;
    Opt<usize> selected = NONE;
    Vec<usize> filtered = Iota(icons.len()) | Collect<Vec<usize>>();

    void filter() {
        Vec<Tuple<usize, int>> matches;
        for (auto [l, i] : iter(icons) | Index()) {
            if (not searchQuery) {
                matches.pushBack({i, {}});
                continue;
            }

            auto match = Glob::matchFuzzy(l.name, searchQuery);
            if (not match)
                continue;
            matches.pushBack({i, match->score});
        }

        if (searchQuery)
            sort(matches, [](auto& a, auto& b) {
                return b.v1 <=> a.v1;
            });

        filtered = iter(matches) |
                   Select([](auto& m) {
                       return m.v0;
                   }) |
                   Collect<Vec<usize>>();
    }

    Opt<IconMetadata const&> selectedIcon() const {
        return icons.index(selected.unwrapOr(Limits<usize>::MAX));
    }
};

struct SelectIcon {
    usize selected;
};

struct UpdateSearch {
    String query;
};

using Action = Union<
    SelectIcon,
    UpdateSearch>;

Ui::Task<Action> reduce(State& state, Action action) {
    action.visit(
        [&](SelectIcon const& a) {
            state.selected = Some(a.selected);
        },
        [&](UpdateSearch const& a) {
            state.searchQuery = a.query;
            state.filter();
        }
    );
    return NONE;
}

using Model = Ui::Model<State, Action, reduce>;

Ui::Child iconGrid(State const& s) {
    return Ui::grid(
               Ui::GridStyle::simpleFixed(
                   {static_cast<isize>(s.filtered.len()) / 8 + 1, 48_au},
                   {8, 48_au}, 4_au
               ),
               iter(s.filtered) | Select([&](usize i) {
                   return Ui::icon(Gfx::Icon{s.icons[i].path, 24}, 48_au) |
                          Ui::center() |
                          Ui::bound() |
                          Ui::button(
                              Some(Model::bind<SelectIcon>(i)),
                              i == s.selected ? Ui::ButtonStyle::regular() : Ui::ButtonStyle::subtle()
                          );
               }) | Collect<Ui::Children>()
           ) |
           Ui::vscroll();
}

Ui::Child iconDetails(IconMetadata const& metadata) {
    return Ui::vflow(
               4_au,
               Ui::hflow(
                   4_au,
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 18_au),
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 24_au),
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 48_au),
                   Ui::icon(Gfx::Icon{metadata.path, 24}, 96_au)
               ),
               Ui::titleMedium(metadata.name)
           ) |
           Ui::pinSize({320_au, Ui::UNCONSTRAINED});
}

Ui::Child app(Vec<IconMetadata> icons) {
    return Ui::reducer<Model>(State{std::move(icons)}, [](State const& s) {
        return Kr::scaffold({
            .icon = Mdi::PALETTE,
            .title = "Icons"s,
            .body = [&] {
                auto content = iconGrid(s) |
                               Kr::scaffoldContent() |
                               Ui::grow();

                if (auto const& [metadata] = s.selectedIcon()) {
                    content = Ui::hflow(4_au, content, iconDetails(metadata) | Kr::scaffoldContent()) |
                              Ui::grow();
                }

                return Ui::vflow(
                    4_au,
                    Kr::searchbar(
                        s.searchQuery,
                        Model::map<UpdateSearch>()
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
    co_return co_await Ui::runAsync(
        env,
        app(std::move(metadatas)),
        ct
    );
}
