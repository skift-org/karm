#include <karm/test>

import Karm.Core;
import Karm.Font.Ttf;
import Karm.Gfx;
import Karm.Ref;
import Karm.Sys;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Font::Ttf::Tests {

template <typename T>
static T _table(Parser const& font) {
    if (auto bytes = font.lookupTable(T::SIG))
        return T{*bytes};
    return T{};
}

static Res<Bytes> _glyphData(Parser const& font, u16 glyphId) {
    auto head = _table<Head>(font);
    auto loca = _table<Loca>(font);
    auto glyf = _table<Glyf>(font);
    auto start = try$(loca.glyfOffset(glyphId, head));
    auto end = try$(loca.glyfOffset(glyphId + 1, head));
    return Ok(sub(glyf.bytes(), start, end));
}

// NOTE: A subset has no 'cmap', Parser::init() would turn it down.
static Res<Parser> _openSubset(Bytes data) {
    Parser font{data};
    auto directory = font.begin();
    try$(directory.skip(4)); // sfntVersion
    usize numTables = try$(directory.nextU16be());
    try$(directory.skip(6)); // searchRange, entrySelector, rangeShift
    font._tables = try$(directory.nextSlice<TableRecord>(numTables));
    return Ok(font);
}

// NOTE: See test-woff.cpp for what is in the font
test$("ttf-subset") {
    auto data = try$(Sys::readAll("bundle://karm-font.tests/woff/tiny.ttf"_url));
    auto font = try$(Parser::init(data));

    auto stairs = try$(font.glyph(U's'));
    auto a = try$(font.glyph(U'A'));
    auto acute = try$(font.glyph(U'´'));
    auto aacute = try$(font.glyph(U'Á'));
    auto aa = try$(font.glyph(U'Å'));

    Array glyphs = {stairs, aa};
    auto subset = try$(Ttf::subset(font, glyphs));

    // 'AA' is made of 'A' and 'Aacute', itself made of 'A' and 'acute'
    assertEq$(subset.glyphs.len(), 5uz);
    assertEq$(subset.glyphs[0].index, stairs.index);
    assertEq$(subset.glyphs[1].index, aa.index);
    assertEq$(subset.glyphs[2].index, a.index);
    assertEq$(subset.glyphs[3].index, aacute.index);
    assertEq$(subset.glyphs[4].index, acute.index);

    // A well formed font sums up to this
    assertEq$(checksum(subset.data), 0xB1B0AFBAu);

    auto out = try$(_openSubset(subset.data));
    assertEq$(try$(out.version()), 0x00010000u);

    Opt<Str> prev = NONE;
    for (auto const& table : out._tables) {
        if (prev)
            assert$(*prev < table.tag());
        prev = Some(table.tag());
    }

    assertEq$(try$(_table<Maxp>(out).numGlyphs()), 5);
    assertEq$(_table<Hhea>(out).numberOfHMetrics(), 5);

    assert$(try$(_glyphData(out, 0)) == try$(_glyphData(font, stairs.index)));
    assert$(try$(_glyphData(out, 2)) == try$(_glyphData(font, a.index)));

    Io::BScan composite{try$(_glyphData(out, 1))};
    try$(composite.skip(10));
    try$(composite.skip(2)); // flags
    assertEq$(try$(composite.nextU16be()), 2);

    auto hmtx = _table<Hmtx>(out);
    auto hhea = _table<Hhea>(out);
    for (usize i = 0; i < subset.glyphs.len(); i++) {
        auto expected = try$(font._hmtx.metrics(subset.glyphs[i].index, font._hhea));
        auto actual = try$(hmtx.metrics(i, hhea));
        assertEq$(actual.advanceWidth, expected.advanceWidth);
        assertEq$(actual.lsb, expected.lsb);
    }

    return Ok();
}

} // namespace Karm::Font::Ttf::Tests
