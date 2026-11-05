module;

#include <karm/macros>

export module Karm.Font.Ttf:subset;

import Karm.Core;
import Karm.Gfx;
import Karm.Ref;

import :container;
import :glyf;
import :hhea;
import :maxp;
import :parser;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff
// https://learn.microsoft.com/en-us/typography/opentype/spec/glyf#composite-glyph-description

static Res<> _visitComponents(Bytes glyph, auto fn) {
    Io::BScan s{glyph};
    if (s.rem() < 10 or s.nextI16be() >= 0)
        return Ok();

    s.seek(10);
    while (s.rem() >= 4) {
        u16 flags = s.nextU16be();
        usize at = s.tell();
        try$(fn(at, s.nextU16be()));

        s.skip((flags & Glyf::ARG_1_AND_2_ARE_WORDS) ? 4 : 2);
        if (flags & Glyf::WE_HAVE_A_SCALE)
            s.skip(2);
        else if (flags & Glyf::WE_HAVE_AN_X_AND_Y_SCALE)
            s.skip(4);
        else if (flags & Glyf::WE_HAVE_A_TWO_BY_TWO)
            s.skip(8);

        if (not(flags & Glyf::MORE_COMPONENTS))
            break;
    }
    return Ok();
}

static Res<Vec<u8>> _patchU16(Bytes table, usize at, u16 v) {
    Io::BufferWriter buf;
    Io::BEmit e{buf};
    try$(e.writeBytes(sub(table, 0, at)));
    try$(e.writeU16be(v));
    try$(e.writeBytes(sub(table, at + 2, table.len())));
    return Ok(buf.take());
}

export Res<Gfx::FontSubset> subset(Parser const& parser, Slice<Gfx::Glyph> glyphs) {
    if (not parser._glyf.present() or not parser._loca.present())
        return Error::notImplemented("font has no glyf outlines");

    auto maxp = parser._maxp;
    Bytes head = parser._head.bytes();
    Bytes hhea = parser._hhea.bytes();
    if (head.len() < 54 or hhea.len() < 36 or maxp.bytes().len() < 6)
        return Error::invalidData("truncated font tables");

    u16 numGlyphs = maxp.numGlyphs();
    auto glyf = parser._glyf.bytes();

    auto glyphData = [&](u16 gid) -> Bytes {
        if (gid >= numGlyphs)
            return {};
        auto start = parser._loca.glyfOffset(gid, parser._head);
        auto end = parser._loca.glyfOffset(gid + 1, parser._head);
        if (end <= start or end > glyf.len())
            return {};
        return sub(glyf, start, end);
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
        try$(_visitComponents(glyphData(parent.index), [&](usize, u16 component) -> Res<> {
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

    Io::BufferWriter glyfBuf, locaBuf, hmtxBuf;
    Io::BEmit glyfOut{glyfBuf}, locaOut{locaBuf}, hmtxOut{hmtxBuf};
    for (auto glyph : result.glyphs) {
        try$(locaOut.writeU32be(glyfOut.tell()));

        auto data = glyphData(glyph.index);
        usize from = 0;
        try$(_visitComponents(data, [&](usize at, u16 component) -> Res<> {
            try$(glyfOut.writeBytes(sub(data, from, at)));
            try$(glyfOut.writeU16be(*newIds.lookup(component)));
            from = at + 2;
            return Ok();
        }));
        try$(glyfOut.writeBytes(sub(data, from, data.len())));
        try$(glyfOut.align(4));

        auto m = parser._hmtx.metrics(glyph.index, parser._hhea);
        try$(hmtxOut.writeU16be(m.advanceWidth));
        try$(hmtxOut.writeI16be(m.lsb));
    }
    try$(locaOut.writeU32be(glyfOut.tell()));

    Io::BufferWriter postBuf;
    Io::BEmit postOut{postBuf};
    try$(postOut.writeU32be(0x00030000));
    if (auto post = parser._post.bytes(); post.len() >= 32)
        try$(postOut.writeBytes(sub(post, 4, 32)));
    else
        for (usize i = 4; i < 32; i++)
            try$(postOut.writeU8be(0));

    auto newHead = try$(_patchU16(head, 50, 1));
    auto newHhea = try$(_patchU16(hhea, Hhea::NumberOfHMetrics::offset, count));
    auto newMaxp = try$(_patchU16(maxp.bytes(), 4, count));

    FontContainer container;
    for (Str tag : {"OS/2", "cvt ", "fpgm", "prep"})
        if (auto t = parser.lookupTable(tag))
            container.add(tag, *t);
    container.add("glyf", glyfBuf.bytes());
    container.add("head", newHead);
    container.add("hhea", newHhea);
    container.add("hmtx", hmtxBuf.bytes());
    container.add("loca", locaBuf.bytes());
    container.add("maxp", newMaxp);
    container.add("post", postBuf.bytes());

    Io::BufferWriter outBuf;
    try$(container.write(outBuf));
    result.data = outBuf.take();
    return Ok(std::move(result));
}

} // namespace Karm::Font::Ttf
