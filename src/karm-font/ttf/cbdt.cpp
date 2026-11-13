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

    Res<Strike> strike(usize i) const {
        auto s = try$(begin(BITMAP_SIZES + i * BITMAP_SIZE_RECORD));
        Strike strike{};
        strike.indexSubTableArrayOffset = try$(s.nextU32be());
        try$(s.skip(4)); // indexTablesSize
        strike.numIndexSubTables = try$(s.nextU32be());
        try$(s.skip(4)); // colorRef
        try$(s.skip(24)); // hori and vert SbitLineMetrics
        strike.startGlyph = try$(s.nextU16be());
        strike.endGlyph = try$(s.nextU16be());
        try$(s.skip(1)); // ppemX
        strike.ppem = try$(s.nextU8be());
        return Ok(strike);
    }

    // NOTE: We don't know the rendering size ahead of time and bitmaps
    //       get scaled anyway, so pick the largest strike covering the glyph.
    Res<Opt<Strike>> bestStrike(u16 glyph) const {
        Opt<Strike> best = NONE;
        for (usize i : urange::zeroTo((usize)try$(get<NumSizes>()))) {
            auto candidate = try$(strike(i));
            if (glyph < candidate.startGlyph or glyph > candidate.endGlyph)
                continue;
            if (not best or candidate.ppem > best->ppem)
                best = Some(candidate);
        }
        return Ok(best);
    }

    static Res<Metrics> _readBigMetrics(Io::BScan s) {
        Metrics m{};
        m.height = try$(s.nextU8be());
        m.width = try$(s.nextU8be());
        m.bearingX = try$(s.nextI8be());
        m.bearingY = try$(s.nextI8be());
        return Ok(m);
    }

    Res<Opt<Location>> locate(Strike const& strike, u16 glyph) const {
        auto array = try$(begin(strike.indexSubTableArrayOffset));

        for (usize i : urange::zeroTo(strike.numIndexSubTables)) {
            auto record = try$(array.peek(i * INDEX_SUBTABLE_RECORD));
            u16 first = try$(record.nextU16be());
            u16 last = try$(record.nextU16be());
            usize additionalOffset = try$(record.nextU32be());

            if (glyph < first or glyph > last)
                continue;

            auto sub = try$(begin(strike.indexSubTableArrayOffset + additionalOffset));
            u16 indexFormat = try$(sub.nextU16be());
            u16 imageFormat = try$(sub.nextU16be());
            usize imageDataOffset = try$(sub.nextU32be());
            usize index = glyph - first;

            switch (indexFormat) {
            case 1: {
                try$(sub.skip(index * 4));
                usize start = try$(sub.nextU32be());
                usize end = try$(sub.nextU32be());
                return Ok(Some(Location{imageDataOffset + start, end - start, imageFormat, NONE}));
            }

            case 2: {
                usize imageSize = try$(sub.nextU32be());
                auto metrics = try$(_readBigMetrics(sub));
                return Ok(Some(Location{imageDataOffset + index * imageSize, imageSize, imageFormat, Some(metrics)}));
            }

            case 3: {
                try$(sub.skip(index * 2));
                usize start = try$(sub.nextU16be());
                usize end = try$(sub.nextU16be());
                return Ok(Some(Location{imageDataOffset + start, end - start, imageFormat, NONE}));
            }

            case 4: {
                usize numGlyphs = try$(sub.nextU32be());
                for (usize j : urange::zeroTo(numGlyphs)) {
                    (void)j;
                    u16 id = try$(sub.nextU16be());
                    usize start = try$(sub.nextU16be());
                    if (id == glyph) {
                        try$(sub.skip(2));
                        usize end = try$(sub.nextU16be());
                        return Ok(Some(Location{imageDataOffset + start, end - start, imageFormat, NONE}));
                    }
                }
                return Ok(NONE);
            }

            case 5: {
                usize imageSize = try$(sub.nextU32be());
                auto metrics = try$(_readBigMetrics(sub));
                try$(sub.skip(BIG_GLYPH_METRICS));
                usize numGlyphs = try$(sub.nextU32be());
                for (usize j : urange::zeroTo(numGlyphs)) {
                    if (try$(sub.nextU16be()) == glyph)
                        return Ok(Some(Location{imageDataOffset + j * imageSize, imageSize, imageFormat, Some(metrics)}));
                }
                return Ok(NONE);
            }

            default:
                return Ok(NONE);
            }
        }

        return Ok(NONE);
    }

    Res<Opt<BitmapGlyph>> bitmap(Cbdt const& cbdt, u16 glyph) const {
        if (not present() or not cbdt.present())
            return Ok(NONE);

        auto maybeStrike = try$(bestStrike(glyph));
        if (not maybeStrike)
            return Ok(NONE);
        auto strike = *maybeStrike;

        auto maybeLocation = try$(locate(strike, glyph));
        if (not maybeLocation)
            return Ok(NONE);
        auto location = *maybeLocation;

        if (location.len == 0 or location.offset + location.len > cbdt.bytes().len())
            return Ok(NONE);

        auto s = try$(cbdt.begin(location.offset));
        Metrics metrics{};

        switch (location.imageFormat) {
        case 17: // SmallGlyphMetrics + PNG
            metrics.height = try$(s.nextU8be());
            metrics.width = try$(s.nextU8be());
            metrics.bearingX = try$(s.nextI8be());
            metrics.bearingY = try$(s.nextI8be());
            try$(s.skip(1)); // advance
            break;

        case 18: // BigGlyphMetrics + PNG
            metrics = try$(_readBigMetrics(s));
            try$(s.skip(BIG_GLYPH_METRICS));
            break;

        case 19: // Metrics in CBLC + PNG
            if (not location.metrics)
                return Ok(NONE);
            metrics = *location.metrics;
            break;

        default:
            return Ok(NONE);
        }

        usize dataLen = try$(s.nextU32be());
        if (dataLen > s.rem())
            return Ok(NONE);

        return Ok(Some(BitmapGlyph{
            .png = try$(s.nextBytes(dataLen)),
            .width = metrics.width,
            .height = metrics.height,
            .bearingX = metrics.bearingX,
            .bearingY = metrics.bearingY,
            .ppem = strike.ppem,
        }));
    }
};

} // namespace Karm::Font::Ttf
