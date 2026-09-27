module;

#include <karm/test>

module Karm.Font;

import :database;

namespace Karm::Font::Tests {

test$("karm-font-common-family") {
    assertEq$(_commonFamily("Noto"_sym, "Noto"_sym), "Noto"_sym);
    assertEq$(_commonFamily("Not"_sym, "Noto"_sym), ""_sym);
    assertEq$(_commonFamily("Noto"_sym, "Arial"_sym), ""_sym);
    assertEq$(_commonFamily("Noto Sans Condensed"_sym, "Noto Sans Condensed Bold"_sym), "Noto Sans Condensed"_sym);
    assertEq$(_commonFamily("Noto Sans ExtraCondensed"_sym, "Noto Sans Condensed Bold"_sym), "Noto Sans"_sym);
    assertEq$(_commonFamily("Comic Sans"_sym, "Comic Serif"_sym), "Comic"_sym);

    return Ok();
}

} // namespace Karm::Font::Tests
