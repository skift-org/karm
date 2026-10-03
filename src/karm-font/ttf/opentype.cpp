module;

#include <karm/macros>

export module Karm.Font.Ttf:opentype;

import Karm.Core;
import Karm.Logger;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#language-system-table
export struct LangSys : Io::BChunk {
    using LookupOrderOffset = Io::BField<u16be, 0>;
    using ReqFeatureIndex = Io::BField<u16be, 2>;
    using FeatureCount = Io::BField<u16be, 4>;

    Str tag;

    LangSys(Str tag, Bytes bytes)
        : BChunk{bytes}, tag(tag) {}

    auto iterFeatures() const {
        auto s = begin().skip(6);
        return urange::zeroTo((usize)get<FeatureCount>()) | Select([s](auto) mutable {
                   return s.nextU16be();
               });
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#script-list-table-and-script-record
export struct ScriptTable : Io::BChunk {
    using DefaultLangSysOffset = Io::BField<u16be, 0>;
    using LangSysCount = Io::BField<u16be, 2>;

    Str tag;

    ScriptTable(Str tag, Bytes bytes)
        : BChunk{bytes}, tag(tag) {}

    LangSys defaultLangSys() const {
        return {"DFLT", begin().skip(get<DefaultLangSysOffset>()).remBytes()};
    }

    usize len() const {
        return get<LangSysCount>();
    }

    LangSys at(usize i) const {
        auto s = begin().skip(4 + i * 6);
        return {s.nextStr(4), begin().skip(s.nextU16be()).remBytes()};
    }

    auto iter() const {
        return urange::zeroTo(len()) | Select([this](auto i) {
                   return at(i);
               });
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#script-list-table-and-script-record
export struct ScriptList : Io::BChunk {
    using ScriptCount = Io::BField<u16be, 0>;

    usize len() const {
        return get<ScriptCount>();
    }

    ScriptTable at(usize i) const {
        auto s = begin().skip(2 + i * 6);
        auto tag = s.nextStr(4);
        auto off = s.nextU16be();
        return ScriptTable{tag, begin().skip(off).remBytes()};
    }

    auto iter() const {
        return urange::zeroTo(len()) | Select([this](auto i) {
                   return at(i);
               });
    }

    Res<ScriptTable> lookup(Str tag) {
        for (auto const& script : iter()) {
            if (script.tag == tag) {
                return Ok(script);
            }
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

    auto iterLookups() const {
        auto s = begin().skip(4);
        return urange::zeroTo((usize)get<LookupCount>()) | Select([s](auto) mutable {
                   return s.nextU16be();
               });
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#feature-list-table
export struct FeatureList : Io::BChunk {
    using featureCount = Io::BField<u16be, 0>;

    usize len() const {
        return get<featureCount>();
    }

    FeatureTable at(usize i) const {
        auto s = begin().skip(2 + i * 6);
        auto tag = s.nextStr(4);
        auto off = s.nextU16be();
        return FeatureTable{tag, begin().skip(off).remBytes()};
    }

    auto iter() const {
        return urange::zeroTo(len()) | Select([this](auto i) {
                   return at(i);
               });
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#coverage-table
export struct CoverageTable : Io::BChunk {
    using Format = Io::BField<u16be, 0>;
    using GlyphCount = Io::BField<u16be, 2>;

    usize format() const { return get<Format>(); }

    usize len() const { return get<GlyphCount>(); }

    Opt<usize> coverageIndex(usize glyphId) {
        auto s = begin().skip(4);

        if (format() == 1) {
            for (auto i : urange::zeroTo(len())) {
                auto glyph = s.nextU16be();
                if (glyph == glyphId) {
                    return Some(i);
                }
            }
        }

        if (format() == 2) {
            for (auto i : urange::zeroTo(len())) {
                (void)i;
                auto start = s.nextU16be();
                auto end = s.nextU16be();
                auto index = s.nextU16be();
                if (start <= glyphId and glyphId <= end) {
                    return Some(index + glyphId - start);
                }
            }
        }

        return NONE;
    }
};

export struct LookupSubtable : Io::BChunk {
    using Format = Io::BField<u16be, 0>;

    u16 format() const { return get<Format>(); }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#class-definition-table
export struct ClassDef : Io::BChunk {
    Opt<usize> classOf(usize glyphId) {
        auto s = begin();
        auto format = s.nextU16be();

        if (format == 1) {
            auto startGlyph = s.nextU16be();
            auto glyphCount = s.nextU16be();
            if (startGlyph <= glyphId and glyphId < startGlyph + glyphCount) {
                return Some(s.skip((glyphId - startGlyph) * 2).nextU16be());
            }
        }

        if (format == 2) {
            auto classRangeCount = s.nextU16be();
            for (usize i : urange::zeroTo(classRangeCount)) {
                (void)i;
                auto startGlyph = s.nextU16be();
                auto endGlyph = s.nextU16be();
                auto glyphClass = s.nextU16be();
                if (startGlyph <= glyphId and glyphId <= endGlyph) {
                    return Some(glyphClass);
                }
            }
        }

        return NONE;
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

    u16 lookupType() const { return get<LookupType>(); }

    u16 lookupFlag() const { return get<LookupFlag>(); }

    u16 markFilteringSet() const {
        return lookupFlag() & USE_MARK_FILTERING_SET
                   ? begin().skip(6 + get<SubTableCount>() * 2).nextU16be()
                   : 0;
    }

    template <typename T>
    T at(usize i) const {
        auto off = begin().skip(6 + i * 2).nextU16be();
        auto subtable = begin().skip(off);
        auto format = subtable.peekU16be();

        return T::any(
                   [&]<typename U>() -> Opt<T> {
                       if constexpr (requires { U::FORMAT; })
                           if (format == U::FORMAT)
                               return Some(U{subtable.remBytes()});
                       return NONE;
                   }
        )
            .unwrapOr(LookupSubtable{subtable.remBytes()});
    }

    usize len() const { return get<SubTableCount>(); }

    template <typename T>
    auto iter() const {
        return urange::zeroTo(len()) | Select([&](auto i) {
                   return at<T>(i);
               });
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#lookup-list-table
export struct LookupList : Io::BChunk {
    using LookupCount = Io::BField<u16be, 0>;

    usize len() const { return get<LookupCount>(); }

    LookupTable at(usize i) const {
        auto s = begin().skip(2 + i * 2);
        auto off = s.nextU16be();
        return LookupTable{begin().skip(off).remBytes()};
    }

    auto iter() const {
        return urange::zeroTo(len()) | Select([&](auto i) {
                   return at(i);
               });
    }
};

} // namespace Karm::Font::Ttf
