module;

#include <karm/macros>

export module Karm.Font.Ttf:opentype;

import Karm.Core;
import Karm.Logger;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#script-list-table-and-script-record
export struct [[gnu::packed]] TagRecord {
    Array<char, 4> _tag;
    u16be offset;

    Str tag() const {
        return {_tag.buf(), _tag.len()};
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#language-system-table
export struct LangSys : Io::BChunk {
    using LookupOrderOffset = Io::BField<u16be, 0>;
    using ReqFeatureIndex = Io::BField<u16be, 2>;
    using FeatureCount = Io::BField<u16be, 4>;

    Str tag;

    LangSys(Str tag, Bytes bytes)
        : BChunk{bytes}, tag(tag) {}

    Res<Slice<u16be>> featureIndices() const {
        return try$(begin(6)).peekSlice<u16be>(try$(get<FeatureCount>()));
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#script-list-table-and-script-record
export struct ScriptTable : Io::BChunk {
    using DefaultLangSysOffset = Io::BField<u16be, 0>;
    using LangSysCount = Io::BField<u16be, 2>;

    Str tag;

    ScriptTable(Str tag, Bytes bytes)
        : BChunk{bytes}, tag(tag) {}

    Res<LangSys> defaultLangSys() const {
        return Ok(LangSys{"DFLT", try$(begin(try$(get<DefaultLangSysOffset>()))).remBytes()});
    }

    Res<Slice<TagRecord>> records() const {
        return try$(begin(4)).peekSlice<TagRecord>(try$(get<LangSysCount>()));
    }

    Res<LangSys> langSys(TagRecord const& record) const {
        return Ok(LangSys{record.tag(), try$(begin(record.offset)).remBytes()});
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#script-list-table-and-script-record
export struct ScriptList : Io::BChunk {
    using ScriptCount = Io::BField<u16be, 0>;

    Res<Slice<TagRecord>> records() const {
        return try$(begin(2)).peekSlice<TagRecord>(try$(get<ScriptCount>()));
    }

    Res<ScriptTable> script(TagRecord const& record) const {
        return Ok(ScriptTable{record.tag(), try$(begin(record.offset)).remBytes()});
    }

    Res<ScriptTable> lookup(Str tag) const {
        for (auto const& record : try$(records())) {
            if (record.tag() == tag)
                return script(record);
        }

        return Error::invalidInput("script tag not found");
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#feature-table
export struct FeatureTable : Io::BChunk {
    using FeatureParamsOffset = Io::BField<u16be, 0>;
    using LookupCount = Io::BField<u16be, 2>;

    Str tag;

    FeatureTable(Str tag, Bytes bytes)
        : BChunk{bytes}, tag(tag) {}

    Res<Slice<u16be>> lookupIndices() const {
        return try$(begin(4)).peekSlice<u16be>(try$(get<LookupCount>()));
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#feature-list-table
export struct FeatureList : Io::BChunk {
    using featureCount = Io::BField<u16be, 0>;

    Res<Slice<TagRecord>> records() const {
        return try$(begin(2)).peekSlice<TagRecord>(try$(get<featureCount>()));
    }

    Res<FeatureTable> feature(TagRecord const& record) const {
        return Ok(FeatureTable{record.tag(), try$(begin(record.offset)).remBytes()});
    }

    Res<FeatureTable> at(usize index) const {
        auto records = try$(this->records());
        if (index >= records.len())
            return Error::invalidData("feature index out of range");
        return feature(records[index]);
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#coverage-table
export struct CoverageTable : Io::BChunk {
    using Format = Io::BField<u16be, 0>;
    using GlyphCount = Io::BField<u16be, 2>;

    Res<usize> format() const { return Ok<usize>(try$(get<Format>())); }

    Res<usize> len() const { return Ok<usize>(try$(get<GlyphCount>())); }

    Res<Opt<usize>> coverageIndex(usize glyphId) {
        auto s = try$(begin(4));
        auto format = try$(this->format());

        if (format == 1) {
            for (auto i : urange::zeroTo(try$(len()))) {
                auto glyph = try$(s.nextU16be());
                if (glyph == glyphId) {
                    return Ok(Some(i));
                }
            }
        }

        if (format == 2) {
            for (auto i : urange::zeroTo(try$(len()))) {
                (void)i;
                auto start = try$(s.nextU16be());
                auto end = try$(s.nextU16be());
                auto index = try$(s.nextU16be());
                if (start <= glyphId and glyphId <= end) {
                    return Ok(Some<usize>(index + glyphId - start));
                }
            }
        }

        return Ok(NONE);
    }
};

export struct LookupSubtable : Io::BChunk {
    using Format = Io::BField<u16be, 0>;

    Res<u16> format() const { return Ok<u16>(try$(get<Format>())); }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#class-definition-table
export struct ClassDef : Io::BChunk {
    Res<Opt<usize>> classOf(usize glyphId) {
        auto s = begin();
        auto format = try$(s.nextU16be());

        if (format == 1) {
            auto startGlyph = try$(s.nextU16be());
            auto glyphCount = try$(s.nextU16be());
            if (startGlyph <= glyphId and glyphId < startGlyph + glyphCount) {
                try$(s.skip((glyphId - startGlyph) * 2));
                return Ok(Some<usize>(try$(s.nextU16be())));
            }
        }

        if (format == 2) {
            auto classRangeCount = try$(s.nextU16be());
            for (usize i : urange::zeroTo(classRangeCount)) {
                (void)i;
                auto startGlyph = try$(s.nextU16be());
                auto endGlyph = try$(s.nextU16be());
                auto glyphClass = try$(s.nextU16be());
                if (startGlyph <= glyphId and glyphId <= endGlyph) {
                    return Ok(Some<usize>(glyphClass));
                }
            }
        }

        return Ok(NONE);
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#lookup-table
export struct LookupTable : Io::BChunk {
    using LookupType = Io::BField<u16be, 0>;
    using LookupFlag = Io::BField<u16be, 2>;
    using SubTableCount = Io::BField<u16be, 4>;

    enum LookupFlags : u16 {
        RIGHT_TO_LEFT = 1 << 0,
        IGNORE_BASE_GLYPHS = 1 << 1,
        IGNORE_LIGATURES = 1 << 2,
        IGNORE_MARKS = 1 << 3,
        USE_MARK_FILTERING_SET = 1 << 4,
        MARK_ATTACHMENT_TYPE = 1 << 5,
    };

    Res<u16> lookupType() const { return Ok<u16>(try$(get<LookupType>())); }

    Res<u16> lookupFlag() const { return Ok<u16>(try$(get<LookupFlag>())); }

    Res<u16> markFilteringSet() const {
        if (not(try$(lookupFlag()) & USE_MARK_FILTERING_SET))
            return Ok<u16>(0);
        auto s = try$(begin(6 + try$(get<SubTableCount>()) * 2));
        return s.nextU16be();
    }

    Res<Slice<u16be>> subtableOffsets() const {
        return try$(begin(6)).peekSlice<u16be>(try$(get<SubTableCount>()));
    }

    template <typename T>
    Res<T> subtable(usize offset) const {
        auto subtable = try$(begin(offset));
        auto format = try$(subtable.peekU16be());

        return Ok(
            T::any(
                [&]<typename U>() -> Opt<T> {
                    if constexpr (requires { U::FORMAT; })
                        if (format == U::FORMAT)
                            return Some(U{subtable.remBytes()});
                    return NONE;
                }
            )
                .unwrapOr(LookupSubtable{subtable.remBytes()})
        );
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#lookup-list-table
export struct LookupList : Io::BChunk {
    using LookupCount = Io::BField<u16be, 0>;

    Res<Slice<u16be>> offsets() const {
        return try$(begin(2)).peekSlice<u16be>(try$(get<LookupCount>()));
    }

    Res<LookupTable> lookup(usize offset) const {
        return Ok(LookupTable{try$(begin(offset)).remBytes()});
    }

    Res<LookupTable> at(usize index) const {
        auto offsets = try$(this->offsets());
        if (index >= offsets.len())
            return Error::invalidData("lookup index out of range");
        return lookup(offsets[index]);
    }
};

} // namespace Karm::Font::Ttf
