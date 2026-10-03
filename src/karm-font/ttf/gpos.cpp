module;

#include <karm/macros>

export module Karm.Font.Ttf:gpos;

import Karm.Math;
import Karm.Logger;

import :opentype;

namespace Karm::Font::Ttf {

export enum struct GposLookupType : u16 {
    SINGLE_ADJUSTMENT = 1,
    PAIR_ADJUSTMENT = 2,
    CURSIVE_ATTACHMENT = 3,
    MARK_TO_BASE_ATTACHMENT = 4,
    MARK_TO_LIGATURE_ATTACHMENT = 5,
    MARK_TO_MARK_ATTACHMENT = 6,
    CONTEXT_POSITIONING = 7,
    CHAIN_CONTEXT_POSITIONING = 8,
    EXTENSION_POSITIONING = 9,
};

export struct GposValueRecord {
    i16 xPlacement;
    i16 yPlacement;
    i16 xAdvance;
    i16 yAdvance;
    i16 xPlacementDeviceOffset;
    i16 yPlacementDeviceOffset;
    i16 xAdvanceDeviceOffset;
    i16 yAdvanceDeviceOffset;

    enum Format : u16 {
        X_PLACEMENT = 1 << 0,
        Y_PLACEMENT = 1 << 1,
        X_ADVANCE = 1 << 2,
        Y_ADVANCE = 1 << 3,
        X_PLACEMENT_DEVICE = 1 << 4,
        Y_PLACEMENT_DEVICE = 1 << 5,
        X_ADVANCE_DEVICE = 1 << 6,
        Y_ADVANCE_DEVICE = 1 << 7,
    };

    static GposValueRecord read(Io::BScan& s, u16 format) {
        GposValueRecord r = {};

        if (format & X_PLACEMENT)
            r.xPlacement = s.nextI16be();

        if (format & Y_PLACEMENT)
            r.yPlacement = s.nextI16be();

        if (format & X_ADVANCE)
            r.xAdvance = s.nextI16be();

        if (format & Y_ADVANCE)
            r.yAdvance = s.nextI16be();

        if (format & X_PLACEMENT_DEVICE)
            r.xPlacementDeviceOffset = s.nextI16be();

        if (format & Y_PLACEMENT_DEVICE)
            r.yPlacementDeviceOffset = s.nextI16be();

        if (format & X_ADVANCE_DEVICE)
            r.xAdvanceDeviceOffset = s.nextI16be();

        if (format & Y_ADVANCE_DEVICE)
            r.yAdvanceDeviceOffset = s.nextI16be();

        return r;
    }

    static usize len(usize format) {
        return popcount(format & 0xff) * sizeof(i16);
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/gpos#lookup-type-2-subtable-pair-adjustment-positioning
export struct GposGlyphPairAdjustment : LookupSubtable {
    static constexpr auto LOOKUP_TYPE = GposLookupType::PAIR_ADJUSTMENT;
    static constexpr int FORMAT = 1;

    Opt<Pair<GposValueRecord>> adjustments(usize prev, usize curr) {
        auto s = begin();

        // Read the table header
        /* format = */ s.nextU16be();
        auto coverageOffset = s.nextU16be();
        auto valueFormat1 = s.nextU16be();
        auto valueFormat2 = s.nextU16be();
        auto pairSetCount = s.nextU16be();

        // Lookup the coverage index for the first glyph
        CoverageTable coverage{begin().skip(coverageOffset).remBytes()};
        auto coverageIndex = coverage.coverageIndex(prev);

        if (not coverageIndex or (*coverageIndex >= pairSetCount))
            return NONE;

        auto value1len = GposValueRecord::len(valueFormat1);
        auto value2len = GposValueRecord::len(valueFormat2);
        auto pairSetOffset = s.skip(coverageIndex.expect() * 2).nextU16be();

        // Lookup the PairSet table for the second glyph
        auto pairSetTable = begin(pairSetOffset);
        auto pairValueCount = pairSetTable.nextU16be();

        for (usize _ : urange::zeroTo(pairValueCount)) {
            auto secondGlyph = pairSetTable.nextU16be();
            if (secondGlyph == curr) {
                GposValueRecord value1 = GposValueRecord::read(pairSetTable, valueFormat1);
                GposValueRecord value2 = GposValueRecord::read(pairSetTable, valueFormat2);
                return Some(Pair<GposValueRecord>{value1, value2});
            }

            pairSetTable.skip(value1len + value2len);
        }

        return NONE;
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/gpos#pair-adjustment-positioning-format-2-class-pair-adjustment
export struct GposClassPairAdjustment : LookupSubtable {
    static constexpr auto LOOKUP_TYPE = GposLookupType::PAIR_ADJUSTMENT;
    static constexpr int FORMAT = 2;

    Opt<Pair<GposValueRecord>> adjustments(usize prev, usize curr) {
        auto s = begin();

        // Read the table header
        /* format = */ s.nextU16be();
        /* coverageOffset = */ s.nextU16be();
        auto valueFormat1 = s.nextU16be();
        auto valueFormat2 = s.nextU16be();
        auto classDef1Offset = s.nextU16be();
        auto classDef2Offset = s.nextU16be();
        /* class1Count = */ s.nextU16be();
        auto class2Count = s.nextU16be();

        ClassDef prevClassDef{begin().skip(classDef1Offset).remBytes()};
        ClassDef currClassDef{begin().skip(classDef2Offset).remBytes()};

        auto prevClass = try$(prevClassDef.classOf(prev));
        auto currClass = try$(currClassDef.classOf(curr));

        auto value1len = GposValueRecord::len(valueFormat1);
        auto value2len = GposValueRecord::len(valueFormat2);

        auto class2Size = value1len + value2len;
        auto class1Size = class2Count * class2Size;
        s.skip((prevClass * class1Size) + (currClass * class2Size));

        GposValueRecord value1 = GposValueRecord::read(s, valueFormat1);
        GposValueRecord value2 = GposValueRecord::read(s, valueFormat2);

        return Some(Pair{value1, value2});
    }
};

export using GposLookupSubtable = Union<
    GposGlyphPairAdjustment,
    GposClassPairAdjustment,
    LookupSubtable>;

// https://learn.microsoft.com/en-us/typography/opentype/spec/gpos
export struct Gpos : Io::BChunk {
    static constexpr Str SIG = "GPOS";

    using ScriptListOffset = Io::BField<u16be, 4>;
    using FeatureListOffset = Io::BField<u16be, 6>;
    using LookupListOffset = Io::BField<u16be, 8>;

    ScriptList scriptList() const {
        return ScriptList{begin().skip(get<ScriptListOffset>()).remBytes()};
    }

    FeatureList featureList() const {
        return FeatureList{begin().skip(get<FeatureListOffset>()).remBytes()};
    }

    LookupList lookupList() const {
        return LookupList{begin().skip(get<LookupListOffset>()).remBytes()};
    }

    Res<Pair<GposValueRecord>> adjustments(usize prev, usize curr) const {
        // 1. Locate the current script in the GPOS ScriptList table.

        // FIXME: We assume that the script is always "latn".
        auto scriptTable = try$(scriptList().lookup("latn"));

        // 2. If the language system is known, search the script for the correct
        //    LangSys table; otherwise, use the script’s default LangSys table.

        // FIXME: We assume that the language system is always "dflt".
        auto langSys = scriptTable.defaultLangSys();

        // 3. The LangSys table provides index numbers into the GPOS FeatureList
        //    table to access a required feature and a number of additional features.
        Opt<FeatureTable> kernFeatureTable;
        for (auto featureIndex : langSys.iterFeatures()) {
            auto featureTable = featureList().at(featureIndex);

            // 4. Inspect the featureTag of each feature, and select the feature
            //    tables to apply to an input glyph string.
            if (featureTable.tag == "kern") {
                kernFeatureTable = Some(featureTable);
                break;
            }
        }

        if (not kernFeatureTable)
            return Ok(Pair<GposValueRecord>{});

        // 5. If a Feature Variation table is present, evaluate conditions in
        //    the Feature Variation table to determine if any of the initially-
        //    selected feature tables should be substituted by an alternate
        //    feature table.

        // FIXME: We don't support feature variations.

        // 6. Each feature provides an array of index numbers into the GPOS
        //    LookupList table. Assemble all lookups from the set of chosen
        //    feature tables, and apply the lookups in the order given in the
        //    LookupList table.
        for (auto lookupIndex : kernFeatureTable->iterLookups()) {
            auto lookupTable = lookupList().at(lookupIndex);

            // FIXME: We only support pair adjustment lookups.
            if (lookupTable.lookupType() != (u16)GposLookupType::PAIR_ADJUSTMENT)
                continue;

            for (auto lookupSubtable : lookupTable.iter<GposLookupSubtable>()) {
                if (auto glyphPair = lookupSubtable.is<GposGlyphPairAdjustment>()) {
                    auto pair = glyphPair->adjustments(prev, curr);
                    if (pair)
                        return Ok(*pair);
                } else if (auto classPair = lookupSubtable.is<GposClassPairAdjustment>()) {
                    auto pair = classPair->adjustments(prev, curr);
                    if (pair)
                        return Ok(*pair);
                } else {
                    logWarn("ttf: unsupported GPOS lookup subtable");
                }
            }
        }

        return Ok(Pair<GposValueRecord>{});
    }
};

} // namespace Karm::Font::Ttf
