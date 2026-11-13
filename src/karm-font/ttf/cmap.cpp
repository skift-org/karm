module;

#include <karm/macros>

export module Karm.Font.Ttf:cmap;

import Karm.Core;
import Karm.Gfx;
import Karm.Logger;

namespace Karm::Font::Ttf {

export struct Cmap : Io::BChunk {
    static constexpr Str SIG = "cmap";

    struct Table {
        u16 platformId;
        u16 encodingId;
        u16 type;

        Bytes slice;

        Io::BScan begin() const {
            return slice;
        }

        Res<Vec<Tuple<u16, u16>>> _extractMappingForType12() {
            Vec<Tuple<u16, u16>> codeMappings;

            auto s = try$(begin().peek(12));
            u32 nGroups = try$(s.nextU32be());

            for (usize i = 0; i < nGroups; i++) {
                u32 startCode = try$(s.nextU32be());
                u32 endCode = try$(s.nextU32be());

                u32 glyphOffset = try$(s.nextU32be());

                for (usize r = startCode; r <= endCode; ++r) {
                    codeMappings.pushBack({(u16)r, (u16)((r - startCode) + glyphOffset)});
                }
            }
            return Ok(codeMappings);
        }

        Res<Gfx::Glyph> _glyphIdForType4(Rune r) const {
            auto header = try$(begin().peek(6));
            u16 segCountX2 = try$(header.nextU16be());
            u16 segCount = segCountX2 / 2;

            for (usize i = 0; i < segCount; i++) {
                auto s = try$(begin().peek(14));

                try$(s.skip(i * 2));
                u16 endCode = try$(s.peekU16be());

                if (r > endCode)
                    continue;

                // + 2 for reserved padding
                try$(s.skip(segCountX2 + 2));
                u16 startCode = try$(s.peekU16be());

                if (r < startCode)
                    break;

                try$(s.skip(segCountX2));
                u16 idDelta = try$(s.peekI16be());
                try$(s.skip(segCountX2));
                u16 idRangeOffset = try$(s.peekU16be());

                if (idRangeOffset == 0) {
                    return Ok(Gfx::Glyph((r + idDelta) & 0xFFFF));
                }

                auto offset = idRangeOffset + (r - startCode) * 2;
                try$(s.skip(offset));
                return Ok(Gfx::Glyph(try$(s.nextU16be())));
            }

            logWarn("ttf: glyph not found for rune {:x}", r);
            return Ok(Gfx::Glyph(0));
        }

        Res<Vec<Tuple<u16, u16>>> _extractMappingForType4() {
            Vec<Tuple<u16, u16>> codeMappings;

            auto header = try$(begin().peek(6));
            u16 segCountX2 = try$(header.nextU16be());
            u16 segCount = segCountX2 / 2;

            for (usize i = 0; i < segCount; i++) {
                auto s = try$(begin().peek(14));

                try$(s.skip(i * 2));
                u16 endCode = try$(s.peekU16be());

                // + 2 for reserved padding
                try$(s.skip(segCountX2 + 2));
                u16 startCode = try$(s.peekU16be());

                try$(s.skip(segCountX2));
                u16 idDelta = try$(s.peekI16be());
                try$(s.skip(segCountX2));
                u16 idRangeOffset = try$(s.peekU16be());

                if (idRangeOffset == 0) {
                    for (usize code = startCode; code <= endCode; code++) {
                        codeMappings.pushBack({(u16)code, (u16)((code + idDelta) & 0xFFFF)});
                    }
                } else {
                    for (usize code = startCode; code <= endCode; code++) {
                        auto offset = idRangeOffset + (code - startCode) * 2;
                        auto gs = try$(s.peek(offset));
                        codeMappings.pushBack({(u16)code, try$(gs.nextU16be())});
                    }
                }
            }

            return Ok(codeMappings);
        }

        Res<Gfx::Glyph> _glyphForType12(Rune r) const {
            auto s = try$(begin().peek(12));
            u32 nGroups = try$(s.nextU32be());

            for (usize i = 0; i < nGroups; i++) {
                u32 startCode = try$(s.nextU32be());
                u32 endCode = try$(s.nextU32be());
                u32 glyphOffset = try$(s.nextU32be());

                if (r < startCode)
                    break;

                if (r > endCode)
                    continue;

                if (r >= startCode and r <= endCode) {
                    return Ok(Gfx::Glyph((r - startCode) + glyphOffset));
                }
            }

            return Ok(Gfx::Glyph(0));
        }

        Res<Vec<Tuple<u16, u16>>> extractMapping() {
            if (type == 4) {
                return _extractMappingForType4();
            } else if (type == 12) {
                return _extractMappingForType12();
            } else {
                return Ok(Vec<Tuple<u16, u16>>{});
            }
        }

        Res<Gfx::Glyph> glyphIdFor(Rune r) const {
            if (type == 4) {
                return _glyphIdForType4(r);
            } else if (type == 12) {
                return _glyphForType12(r);
            } else {
                return Ok(Gfx::Glyph(0));
            }
        }
    };

    // https://learn.microsoft.com/en-us/typography/opentype/spec/cmap#encoding-records-and-encodings
    struct [[gnu::packed]] EncodingRecord {
        u16be platformId;
        u16be encodingId;
        u32be offset;
    };

    Res<Slice<EncodingRecord>> records() const {
        auto s = begin();
        try$(s.skip(2)); // version
        auto numTables = try$(s.nextU16be());
        return s.nextSlice<EncodingRecord>(numTables);
    }

    Res<Table> table(EncodingRecord const& record) const {
        Table t;
        t.platformId = record.platformId;
        t.encodingId = record.encodingId;
        t.slice = try$(begin(record.offset)).remBytes();
        t.type = try$(t.begin().nextU16be());
        return Ok(t);
    }
};

} // namespace Karm::Font::Ttf
