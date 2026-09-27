module;

#include <karm/macros>

export module Karm.Font.Ttf:cbdt;

import Karm.Core;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/cbdt
export struct Cbdt : Io::BChunk {
    static constexpr Str SIG = "CBDT";
};

// A color bitmap glyph, positioned in pixels of its strike.
export struct BitmapGlyph {
    Bytes png;
    u8 width;
    u8 height;
    i8 bearingX;
    i8 bearingY;
    u8 ppem;
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/cblc
export struct Cblc : Io::BChunk {
    static constexpr Str SIG = "CBLC";

    using NumSizes = Io::BField<u32be, 4>;

    static constexpr usize BITMAP_SIZES = 8;
    static constexpr usize BITMAP_SIZE_RECORD = 48;
    static constexpr usize INDEX_SUBTABLE_RECORD = 8;
    static constexpr usize BIG_GLYPH_METRICS = 8;

    struct Strike {
        usize indexSubTableArrayOffset;
        usize numIndexSubTables;
        u16 startGlyph;
        u16 endGlyph;
        u8 ppem;
    };

    struct Metrics {
        u8 height;
        u8 width;
        i8 bearingX;
        i8 bearingY;
    };

    struct Location {
        usize offset;
        usize len;
        u16 imageFormat;
        Opt<Metrics> metrics;
    };

    Strike strike(usize i) const {
        auto s = begin(BITMAP_SIZES + i * BITMAP_SIZE_RECORD);
        Strike strike{};
        strike.indexSubTableArrayOffset = s.nextU32be();
        /* indexTablesSize = */ s.nextU32be();
        strike.numIndexSubTables = s.nextU32be();
        /* colorRef = */ s.nextU32be();
        s.skip(24); // hori and vert SbitLineMetrics
        strike.startGlyph = s.nextU16be();
        strike.endGlyph = s.nextU16be();
        /* ppemX = */ s.nextU8be();
        strike.ppem = s.nextU8be();
        return strike;
    }

    // NOTE: We don't know the rendering size ahead of time and bitmaps
    //       get scaled anyway, so pick the largest strike covering the glyph.
    Opt<Strike> bestStrike(u16 glyph) const {
        Opt<Strike> best = NONE;
        for (usize i : urange::zeroTo((usize)get<NumSizes>())) {
            auto candidate = strike(i);
            if (glyph < candidate.startGlyph or glyph > candidate.endGlyph)
                continue;
            if (not best or candidate.ppem > best->ppem)
                best = Some(candidate);
        }
        return best;
    }

    static Metrics _readBigMetrics(Io::BScan s) {
        Metrics m{};
        m.height = s.nextU8be();
        m.width = s.nextU8be();
        m.bearingX = s.nextI8be();
        m.bearingY = s.nextI8be();
        return m;
    }

    Opt<Location> locate(Strike const& strike, u16 glyph) const {
        auto array = begin(strike.indexSubTableArrayOffset);

        for (usize i : urange::zeroTo(strike.numIndexSubTables)) {
            auto record = array;
            record.skip(i * INDEX_SUBTABLE_RECORD);
            u16 first = record.nextU16be();
            u16 last = record.nextU16be();
            usize additionalOffset = record.nextU32be();

            if (glyph < first or glyph > last)
                continue;

            auto sub = begin(strike.indexSubTableArrayOffset + additionalOffset);
            u16 indexFormat = sub.nextU16be();
            u16 imageFormat = sub.nextU16be();
            usize imageDataOffset = sub.nextU32be();
            usize index = glyph - first;

            switch (indexFormat) {
            case 1: {
                sub.skip(index * 4);
                usize start = sub.nextU32be();
                usize end = sub.nextU32be();
                return Some(Location{imageDataOffset + start, end - start, imageFormat, NONE});
            }

            case 2: {
                usize imageSize = sub.nextU32be();
                auto metrics = _readBigMetrics(sub);
                return Some(Location{imageDataOffset + index * imageSize, imageSize, imageFormat, Some(metrics)});
            }

            case 3: {
                sub.skip(index * 2);
                usize start = sub.nextU16be();
                usize end = sub.nextU16be();
                return Some(Location{imageDataOffset + start, end - start, imageFormat, NONE});
            }

            case 4: {
                usize numGlyphs = sub.nextU32be();
                for (usize j : urange::zeroTo(numGlyphs)) {
                    (void)j;
                    u16 id = sub.nextU16be();
                    usize start = sub.nextU16be();
                    if (id == glyph) {
                        sub.skip(2);
                        usize end = sub.nextU16be();
                        return Some(Location{imageDataOffset + start, end - start, imageFormat, NONE});
                    }
                }
                return NONE;
            }

            case 5: {
                usize imageSize = sub.nextU32be();
                auto metrics = _readBigMetrics(sub);
                sub.skip(BIG_GLYPH_METRICS);
                usize numGlyphs = sub.nextU32be();
                for (usize j : urange::zeroTo(numGlyphs)) {
                    if (sub.nextU16be() == glyph)
                        return Some(Location{imageDataOffset + j * imageSize, imageSize, imageFormat, Some(metrics)});
                }
                return NONE;
            }

            default:
                return NONE;
            }
        }

        return NONE;
    }

    Opt<BitmapGlyph> bitmap(Cbdt const& cbdt, u16 glyph) const {
        if (not present() or not cbdt.present())
            return NONE;

        auto strike = try$(bestStrike(glyph));
        auto location = try$(locate(strike, glyph));
        if (location.len == 0 or location.offset + location.len > cbdt.bytes().len())
            return NONE;

        auto s = cbdt.begin(location.offset);
        Metrics metrics{};

        switch (location.imageFormat) {
        case 17: // SmallGlyphMetrics + PNG
            metrics.height = s.nextU8be();
            metrics.width = s.nextU8be();
            metrics.bearingX = s.nextI8be();
            metrics.bearingY = s.nextI8be();
            /* advance = */ s.nextU8be();
            break;

        case 18: // BigGlyphMetrics + PNG
            metrics = _readBigMetrics(s);
            s.skip(BIG_GLYPH_METRICS);
            break;

        case 19: // Metrics in CBLC + PNG
            metrics = try$(location.metrics);
            break;

        default:
            return NONE;
        }

        usize dataLen = s.nextU32be();
        if (dataLen > s.rem())
            return NONE;

        return Some(BitmapGlyph{
            .png = s.nextBytes(dataLen),
            .width = metrics.width,
            .height = metrics.height,
            .bearingX = metrics.bearingX,
            .bearingY = metrics.bearingY,
            .ppem = strike.ppem,
        });
    }
};

} // namespace Karm::Font::Ttf
