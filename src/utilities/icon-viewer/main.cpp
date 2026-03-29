#include <karm/entry>

import Mdi;
import Karm.Kira;
import Karm.Ui;
import Karm.Gfx;
import Karm.Logger;

using namespace Karm;

struct IconMetadata {
    String name;
    String path;
};

struct State {
    Vec<IconMetadata> icons = {};
    Vec<IconMetadata> filtered = {};
    String searchQuery = ""s;
    Opt<usize> selected = NONE;

    void filter() {
        Vec<Tuple<IconMetadata, int>> matches;
        for (auto l : icons) {
            if (not searchQuery) {
                matches.pushBack({l, {}});
                continue;
            }

            auto match = Glob::matchFuzzy(l.name, searchQuery);
            if (not match)
                continue;
            matches.pushBack({l, match->score});
        }

        if (searchQuery)
            sort(matches, [](auto& a, auto& b) {
                return b.v1 <=> a.v1;
            });

        selected = NONE;
        filtered = iter(matches) |
                   Select([](auto& m) {
                       return m.v0;
                   }) |
                   Collect<Vec<IconMetadata>>();
    }
};

struct UpdateSearch {
    String query;
};

struct SelectIcon {
    usize index;
};

using Action = Union<UpdateSearch, SelectIcon>;

Ui::Task<Action> reduce(State& state, Action action) {
    action.visit(Visitor{
        [&](UpdateSearch a) {
            state.searchQuery = a.query;
            state.filter();
        },
        [&](SelectIcon a) {
            state.selected = a.index;
        },
    });
    return NONE;
}

using Model = Ui::Model<State, Action, reduce>;

Ui::Child iconGrid(State const& s) {
    return Ui::grid(
               Ui::GridStyle::simpleFixed({((isize)s.icons.len() / 8) + 1, 48}, {8, 48}, 4),
               iter(s.filtered) | Selecti([&](IconMetadata const& i, usize index) {
                   return Ui::icon(Gfx::Icon{i.path, 24}, 48) |
                          Ui::center() |
                          Ui::bound() |
                          Ui::button(
                              Model::bind<SelectIcon>(index),
                              index == s.selected ? Ui::ButtonStyle::regular() : Ui::ButtonStyle::subtle()
                          );
               }) | Collect<Ui::Children>()
           ) |
           Ui::vscroll();
}

Ui::Child iconDetails(State const& s) {
    auto& metadata = s.filtered[s.selected.unwrapOr(0)];

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

Ui::Child app(Vec<IconMetadata> icons) {
    return Ui::reducer<Model>({icons, icons}, [](State const& s) {
        return Kr::scaffold({
            .icon = Mdi::PALETTE,
            .title = "Icons"s,
            .body = [&] {
                auto content = iconGrid(s) |
                               Kr::scaffoldContent() |
                               Ui::grow();

                if (s.selected) {
                    content = Ui::hflow(4, content, iconDetails(s) | Kr::scaffoldContent()) |
                              Ui::grow();
                }

                return Ui::vflow(
                    4,
                    Kr::searchbar(s.searchQuery, Model::map<UpdateSearch>()),
                    content
                );
            },
        });
    });
}

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto data = co_try$(Sys::readAllUtf8("bundle://mdi/icons.json"_url));
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
