module;

#include <karm/macros>

export module Karm.Font.Ttf:parser;

import Karm.Logger;

import :cbdt;
import :cmap;
import :colr;
import :cpal;
import :glyf;
import :gpos;
import :gsub;
import :head;
import :hhea;
import :hmtx;
import :loca;
import :maxp;
import :name;
import :os2;
import :post;

// https://tchayen.github.io/posts/ttf-file-parsing
// http://stevehanov.ca/blog/?id=143
// https://www.microsoft.com/typography/otspec/otff.htm
// https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
// https://docs.microsoft.com/en-us/typography/opentype/spec/ttch01
// https://docs.microsoft.com/en-us/typography/opentype/spec/otff
// https://fontdrop.info/

namespace Karm::Font::Ttf {

static bool DEBUG_PARSER = false;

export struct GlyphMetrics {
    f64 x;
    f64 y;
    f64 width;
    f64 height;
    f64 lsb;
    f64 advance;
};

export struct Metrics {
    f64 ascend;
    f64 descend;
    f64 linegap;
    f64 maxWidth;
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff#table-directory
export struct [[gnu::packed]] TableRecord {
    Array<char, 4> _tag;
    u32be checkSum;
    u32be offset;
    u32be length;

    Str tag() const {
        return {_tag.buf(), _tag.len()};
    }
};

export struct Parser {
    Bytes _slice;
    Slice<TableRecord> _tables;

    Head _head;
    Cmap _cmap;
    Cmap::Table _cmapTable;
    Glyf _glyf;
    Loca _loca;
    Hhea _hhea;
    Hmtx _hmtx;
    Maxp _maxp;
    Gpos _gpos;
    Gsub _gsub;
    Name _name;
    Post _post;
    Os2 _os2;
    Colr _colr;
    Cpal _cpal;
    Cblc _cblc;
    Cbdt _cbdt;

    static Res<Cmap::Table> chooseCmap(Parser& font) {
        Opt<Cmap::Table> bestCmap = NONE;
        isize bestScore = 0;

        struct KnowCmap {
            isize platformId;
            isize encodingId;
            isize type;
            isize score;
        };

        Array<KnowCmap, 4> knowCmaps = {
            KnowCmap{0, 3, 4, 150},
            KnowCmap{3, 1, 4, 100},
            KnowCmap{0, 4, 12, 1050},
            KnowCmap{3, 10, 12, 1000},
        };

        for (auto const& record : try$(font._cmap.records())) {
            auto table = try$(font._cmap.table(record));
            for (auto& knowCmap : knowCmaps) {
                if (knowCmap.platformId == table.platformId and
                    knowCmap.encodingId == table.encodingId and
                    knowCmap.type == table.type and
                    knowCmap.score > bestScore) {

                    bestCmap = Some(table);
                    bestScore = knowCmap.score;
                }
            }
        }

        if (not bestCmap) {
            logError("ttf: no suitable cmap table found");
            return Error::other("no suitable cmap table found");
        }

        return Ok(*bestCmap);
    }

    static bool sniff(Bytes slice) {
        return slice.len() >= 4 and slice[0] == 0x00 and slice[1] == 0x01 and
               slice[2] == 0x00 and slice[3] == 0x00;
    }

    static Res<Parser> init(Bytes slice) {
        Parser font{slice};

        auto version = try$(font.version());
        if (version != 0x00010000 and
            version != 0x4F54544F) {
            logError("ttf: version {:x} is not supported", (u64)version);
            return Error::other("invalid version");
        }

        auto directory = font.begin();
        try$(directory.skip(4)); // sfntVersion
        usize numTables = try$(directory.nextU16be());
        try$(directory.skip(6)); // searchRange, entrySelector, rangeShift
        font._tables = try$(directory.nextSlice<TableRecord>(numTables));

        font._head = try$(font.requireTable<Head>());
        if (font._head.bytes().len() < Head::MIN_LEN)
            return Error::invalidData("truncated 'head' table");

        font._cmap = try$(font.requireTable<Cmap>());
        font._cmapTable = try$(chooseCmap(font));
        // NOTE: Bitmap-only fonts (e.g. CBDT color emoji) have no outlines.
        font._cblc = font.lookupTable<Cblc>();
        font._cbdt = font.lookupTable<Cbdt>();
        if (font._cblc.present() and font._cbdt.present()) {
            font._glyf = font.lookupTable<Glyf>();
            font._loca = font.lookupTable<Loca>();
        } else {
            font._glyf = try$(font.requireTable<Glyf>());
            font._loca = try$(font.requireTable<Loca>());
        }
        font._hhea = try$(font.requireTable<Hhea>());
        if (font._hhea.bytes().len() < Hhea::MIN_LEN)
            return Error::invalidData("truncated 'hhea' table");

        font._hmtx = try$(font.requireTable<Hmtx>());
        font._maxp = font.lookupTable<Maxp>();
        font._gpos = font.lookupTable<Gpos>();
        font._gsub = font.lookupTable<Gsub>();
        font._name = font.lookupTable<Name>();
        font._post = font.lookupTable<Post>();
        if (not font._post.present())
            logWarn("ttf: 'post' table not found");

        font._os2 = font.lookupTable<Os2>();
        if (not font._os2.present())
            logWarn("ttf: 'OS/2' table not found");

        font._colr = font.lookupTable<Colr>();
        font._cpal = font.lookupTable<Cpal>();
        if (font._colr.present() and not font._cpal.present()) {
            logWarn("ttf: 'COLR' table without 'CPAL' table, ignoring color glyphs");
            font._colr = {};
        }

        return Ok(font);
    }

    Parser(Bytes slice) : _slice(slice) {}

    // MARK: Parsing Api -------------------------------------------------------

    Io::BScan begin() const {
        return _slice;
    }

    Res<u32> version() {
        return begin().nextU32be();
    }

    // NOTE: Tables extending past the end of the file are truncated.
    Opt<Bytes> lookupTable(Str tag) const {
        for (auto const& table : _tables) {
            if (table.tag() == tag) {
                usize offset = table.offset;
                return Some(sub(_slice, offset, offset + table.length));
            }
        }
        return NONE;
    }

    template <typename T>
    T lookupTable() {
        if (auto bytes = lookupTable(T::SIG))
            return T{*bytes};

        logDebugIf(DEBUG_PARSER, "ttf: '{}' table not found", T::SIG);
        return T{};
    }

    template <typename T>
    Res<T> requireTable() {
        if (auto bytes = lookupTable(T::SIG))
            return Ok(T{*bytes});

        logError("ttf: '{}' table not found", T::SIG);
        return Error::other("table not found");
    }

    Res<Gfx::Glyph> glyph(Rune rune) const {
        return _cmapTable.glyphIdFor(rune);
    }

    Res<GlyphMetrics> glyphMetrics(Gfx::Glyph glyph) const {
        auto hmtx = try$(_hmtx.metrics(glyph.index, _hhea));
        if (not _glyf.present() or not _loca.present())
            return Ok(GlyphMetrics{0, 0, 0, 0, (f64)hmtx.lsb, (f64)hmtx.advanceWidth});

        auto glyfOffset = try$(_loca.glyfOffset(glyph.index, _head));

        // NOTE: Glyphs without an outline (e.g. space) have no glyf entry.
        if (glyfOffset == try$(_loca.glyfOffset(glyph.index + 1, _head)))
            return Ok(GlyphMetrics{0, 0, 0, 0, (f64)hmtx.lsb, (f64)hmtx.advanceWidth});

        auto glyf = try$(_glyf.metrics(glyfOffset));

        return Ok(GlyphMetrics{
            (f64)glyf.xMin,
            (f64)-glyf.yMax,
            (f64)glyf.xMax - glyf.xMin,
            (f64)glyf.yMax - glyf.yMin,
            (f64)hmtx.lsb,
            (f64)hmtx.advanceWidth,
        });
    }

    f64 glyphKern(Gfx::Glyph prev, Gfx::Glyph curr) const {
        if (not _gpos.present())
            return 0;

        auto positioning = _gpos.adjustments(prev.index, curr.index);

        if (not positioning)
            return 0;

        return positioning.expect().v0.xAdvance;
    }

    Res<> glyphContour(Gfx::Canvas& g, Gfx::Glyph glyph) const {
        if (not _glyf.present() or not _loca.present())
            return Ok();

        auto glyfOffset = try$(_loca.glyfOffset(glyph.index, _head));

        if (glyfOffset == try$(_loca.glyfOffset(glyph.index + 1, _head)))
            return Ok();

        return _glyf.contour(g, glyfOffset, _loca, _head);
    }

    // MARK: Color Glyphs ----------------------------------------------------

    Res<Opt<BitmapGlyph>> glyphBitmap(Gfx::Glyph glyph) const {
        return _cblc.bitmap(_cbdt, glyph.index);
    }

    Res<bool> glyphHasColor(Gfx::Glyph glyph) const {
        return Ok(try$(_colr.layers(glyph.index)).has());
    }

    Res<> glyphPaint(Gfx::Canvas& g, Gfx::Glyph glyph) const {
        return _colr.paint(g, glyph.index, _cpal, [&](Gfx::Canvas& g, u16 layerGlyph) {
            return glyphContour(g, {layerGlyph, glyph.font});
        });
    }

    Metrics metrics() const {
        return {
            (f64)_hhea.ascender(),
            (f64)_hhea.descender(),
            (f64)_hhea.lineGap(),
            (f64)_hhea.advanceWidthMax(),
        };
    }

    f64 unitPerEm() const {
        return _head.unitPerEm();
    }
};

} // namespace Karm::Font::Ttf
