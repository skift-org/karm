#include <karm-font/database.h>
#include <karm-test/macros.h>

namespace Karm::Font::Tests {

test$("karm-text-common-family") {
    expectEq$(commonFamily("Noto"_sym, "Noto"_sym), "Noto"_sym);
    expectEq$(commonFamily("Not"_sym, "Noto"_sym), ""_sym);
    expectEq$(commonFamily("Noto"_sym, "Arial"_sym), ""_sym);
    expectEq$(commonFamily("Noto Sans Condensed"_sym, "Noto Sans Condensed Bold"_sym), "Noto Sans Condensed"_sym);
    expectEq$(commonFamily("Noto Sans ExtraCondensed"_sym, "Noto Sans Condensed Bold"_sym), "Noto Sans"_sym);
    expectEq$(commonFamily("Comic Sans"_sym, "Comic Serif"_sym), "Comic"_sym);

    return Ok();
}

} // namespace Karm::Text::Tests
