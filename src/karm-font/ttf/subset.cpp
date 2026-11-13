module;

#include <karm/macros>

export module Karm.Font.Ttf:subset;

import Karm.Core;
import Karm.Gfx;
import Karm.Ref;

import :glyf;
import :hhea;
import :maxp;
import :parser;
import :sfnt;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff
// https://learn.microsoft.com/en-us/typography/opentype/spec/glyf#composite-glyph-description

static Res<> _visitComponents(Bytes glyph, auto fn) {
    Io::BScan s{glyph};
    if (s.rem() < 10 or try$(s.nextI16be()) >= 0)
        return Ok();

    try$(s.seek(10));
    while (s.rem() >= 4) {
        u16 flags = try$(s.nextU16be());
        usize at = s.tell();
        try$(fn(at, try$(s.nextU16be())));

        try$(s.skip((flags & Glyf::ARG_1_AND_2_ARE_WORDS) ? 4 : 2));
        if (flags & Glyf::WE_HAVE_A_SCALE)
            try$(s.skip(2));
        else if (flags & Glyf::WE_HAVE_AN_X_AND_Y_SCALE)
            try$(s.skip(4));
        else if (flags & Glyf::WE_HAVE_A_TWO_BY_TWO)
            try$(s.skip(8));

        if (not(flags & Glyf::MORE_COMPONENTS))
            break;
    }
    return Ok();
}

export Res<Gfx::FontSubset> subset(Parser const& parser, Slice<Gfx::Glyph> glyphs) {
    if (not parser._glyf.present() or not parser._loca.present())
        return Error::notImplemented("font has no glyf outlines");

    auto maxp = parser._maxp;
    Bytes head = parser._head.bytes();
    Bytes hhea = parser._hhea.bytes();
    if (head.len() < 54 or hhea.len() < 36 or maxp.bytes().len() < 6)
        return Error::invalidData("truncated font tables");

    u16 numGlyphs = try$(maxp.numGlyphs());
    auto glyf = parser._glyf.bytes();

    auto glyphData = [&](u16 gid) -> Res<Bytes> {
        if (gid >= numGlyphs)
            return Ok(Bytes{});
        auto start = try$(parser._loca.glyfOffset(gid, parser._head));
        auto end = try$(parser._loca.glyfOffset(gid + 1, parser._head));
        if (end <= start or end > glyf.len())
            return Ok(Bytes{});
        return Ok(sub(glyf, start, end));
    };

    Gfx::FontSubset result{.format = Ref::Uti::PUBLIC_TTF};
    Map<u16, u16> newIds;
    for (auto glyph : glyphs) {
        if (not newIds.contains(glyph.index))
            newIds.put(glyph.index, result.glyphs.len());
        result.glyphs.pushBack(glyph);
    }

    for (usize i = 0; i < result.glyphs.len(); i++) {
        auto parent = result.glyphs[i];
        try$(_visitComponents(try$(glyphData(parent.index)), [&](usize, u16 component) -> Res<> {
            if (not newIds.contains(component)) {
                newIds.put(component, result.glyphs.len());
                result.glyphs.pushBack({component, parent.font});
            }
            return Ok();
        }));
    }

    if (result.glyphs.len() > 0xffff)
        return Error::invalidData("too many glyphs");
    u16 count = result.glyphs.len();

    BufferBuilder glyfBuf, locaBuf, hmtxBuf;
    for (auto glyph : result.glyphs) {
        locaBuf.appendU32be(glyfBuf.len());

        auto data = try$(glyphData(glyph.index));
        usize from = 0;
        try$(_visitComponents(data, [&](usize at, u16 component) -> Res<> {
            glyfBuf.append(sub(data, from, at));
            glyfBuf.appendU16be(*newIds.lookup(component));
            from = at + 2;
            return Ok();
        }));
        glyfBuf.append(sub(data, from, data.len()));
        glyfBuf.align(4);

        auto m = try$(parser._hmtx.metrics(glyph.index, parser._hhea));
        hmtxBuf.appendU16be(m.advanceWidth);
        hmtxBuf.appendI16be(m.lsb);
    }
    locaBuf.appendU32be(glyfBuf.len());

    BufferBuilder postBuf;
    postBuf.appendU32be(0x00030000);
    if (auto post = parser._post.bytes(); post.len() >= 32)
        postBuf.append(sub(post, 4, 32));
    else
        postBuf.appendFill(28);

    BufferBuilder newHead{head};
    newHead.patchU16be(50, 1); // indexToLocFormat, 'loca' is written long

    BufferBuilder newHhea{hhea};
    newHhea.patchU16be(Hhea::NumberOfHMetrics::offset, count);

    BufferBuilder newMaxp{maxp.bytes()};
    newMaxp.patchU16be(4, count); // numGlyphs

    // NOTE: Tables are laid out in the order they are added, which is kept
    //       the same as the one of their tags.
    SfntBuilder sfnt;
    for (Str tag : {"OS/2", "cvt ", "fpgm"})
        if (auto t = parser.lookupTable(tag))
            try$(sfnt.add(tag, *t));
    try$(sfnt.add("glyf", glyfBuf.bytes()));
    try$(sfnt.add("head", newHead.bytes()));
    try$(sfnt.add("hhea", newHhea.bytes()));
    try$(sfnt.add("hmtx", hmtxBuf.bytes()));
    try$(sfnt.add("loca", locaBuf.bytes()));
    try$(sfnt.add("maxp", newMaxp.bytes()));
    try$(sfnt.add("post", postBuf.bytes()));
    if (auto t = parser.lookupTable("prep"))
        try$(sfnt.add("prep", *t));

    result.data = try$(sfnt.finish());
    return Ok(std::move(result));
}

} // namespace Karm::Font::Ttf
