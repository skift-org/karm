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

    static Res<GposValueRecord> read(Io::BScan& s, u16 format) {
        GposValueRecord r = {};

        if (format & X_PLACEMENT)
            r.xPlacement = try$(s.nextI16be());

        if (format & Y_PLACEMENT)
            r.yPlacement = try$(s.nextI16be());

        if (format & X_ADVANCE)
            r.xAdvance = try$(s.nextI16be());

        if (format & Y_ADVANCE)
            r.yAdvance = try$(s.nextI16be());

        if (format & X_PLACEMENT_DEVICE)
            r.xPlacementDeviceOffset = try$(s.nextI16be());

        if (format & Y_PLACEMENT_DEVICE)
            r.yPlacementDeviceOffset = try$(s.nextI16be());

        if (format & X_ADVANCE_DEVICE)
            r.xAdvanceDeviceOffset = try$(s.nextI16be());

        if (format & Y_ADVANCE_DEVICE)
            r.yAdvanceDeviceOffset = try$(s.nextI16be());

        return Ok(r);
    }

    static usize len(usize format) {
        return popcount(format & 0xff) * sizeof(i16);
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/gpos#lookup-type-2-subtable-pair-adjustment-positioning
export struct GposGlyphPairAdjustment : LookupSubtable {
    static constexpr auto LOOKUP_TYPE = GposLookupType::PAIR_ADJUSTMENT;
    static constexpr int FORMAT = 1;

    Res<Opt<Pair<GposValueRecord>>> adjustments(usize prev, usize curr) {
        auto s = begin();

        // Read the table header
        try$(s.skip(2)); // format
        auto coverageOffset = try$(s.nextU16be());
        auto valueFormat1 = try$(s.nextU16be());
        auto valueFormat2 = try$(s.nextU16be());
        auto pairSetCount = try$(s.nextU16be());

        // Lookup the coverage index for the first glyph
        CoverageTable coverage{try$(begin(coverageOffset)).remBytes()};
        auto coverageIndex = try$(coverage.coverageIndex(prev));

        if (not coverageIndex or (*coverageIndex >= pairSetCount))
            return Ok(NONE);

        auto value1len = GposValueRecord::len(valueFormat1);
        auto value2len = GposValueRecord::len(valueFormat2);
        try$(s.skip(*coverageIndex * 2));
        auto pairSetOffset = try$(s.nextU16be());

        // Lookup the PairSet table for the second glyph
        auto pairSetTable = try$(begin(pairSetOffset));
        auto pairValueCount = try$(pairSetTable.nextU16be());

        for (usize _ : urange::zeroTo(pairValueCount)) {
            auto secondGlyph = try$(pairSetTable.nextU16be());
            if (secondGlyph == curr) {
                GposValueRecord value1 = try$(GposValueRecord::read(pairSetTable, valueFormat1));
                GposValueRecord value2 = try$(GposValueRecord::read(pairSetTable, valueFormat2));
                return Ok(Some(Pair<GposValueRecord>{value1, value2}));
            }

            try$(pairSetTable.skip(value1len + value2len));
        }

        return Ok(NONE);
    }
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/gpos#pair-adjustment-positioning-format-2-class-pair-adjustment
export struct GposClassPairAdjustment : LookupSubtable {
    static constexpr auto LOOKUP_TYPE = GposLookupType::PAIR_ADJUSTMENT;
    static constexpr int FORMAT = 2;

    Res<Opt<Pair<GposValueRecord>>> adjustments(usize prev, usize curr) {
        auto s = begin();

        // Read the table header
        try$(s.skip(2)); // format
        try$(s.skip(2)); // coverageOffset
        auto valueFormat1 = try$(s.nextU16be());
        auto valueFormat2 = try$(s.nextU16be());
        auto classDef1Offset = try$(s.nextU16be());
        auto classDef2Offset = try$(s.nextU16be());
        try$(s.skip(2)); // class1Count
        auto class2Count = try$(s.nextU16be());

        ClassDef prevClassDef{try$(begin(classDef1Offset)).remBytes()};
        ClassDef currClassDef{try$(begin(classDef2Offset)).remBytes()};

        auto prevClass = try$(prevClassDef.classOf(prev));
        auto currClass = try$(currClassDef.classOf(curr));
        if (not prevClass or not currClass)
            return Ok(NONE);

        auto value1len = GposValueRecord::len(valueFormat1);
        auto value2len = GposValueRecord::len(valueFormat2);

        auto class2Size = value1len + value2len;
        auto class1Size = class2Count * class2Size;
        try$(s.skip((*prevClass * class1Size) + (*currClass * class2Size)));

        GposValueRecord value1 = try$(GposValueRecord::read(s, valueFormat1));
        GposValueRecord value2 = try$(GposValueRecord::read(s, valueFormat2));

        return Ok(Some(Pair{value1, value2}));
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

    Res<ScriptList> scriptList() const {
        return Ok(ScriptList{try$(begin(try$(get<ScriptListOffset>()))).remBytes()});
    }

    Res<FeatureList> featureList() const {
        return Ok(FeatureList{try$(begin(try$(get<FeatureListOffset>()))).remBytes()});
    }

    Res<LookupList> lookupList() const {
        return Ok(LookupList{try$(begin(try$(get<LookupListOffset>()))).remBytes()});
    }

    Res<Pair<GposValueRecord>> adjustments(usize prev, usize curr) const {
        // 1. Locate the current script in the GPOS ScriptList table.

        // FIXME: We assume that the script is always "latn".
        auto scriptTable = try$(try$(scriptList()).lookup("latn"));

        // 2. If the language system is known, search the script for the correct
        //    LangSys table; otherwise, use the script’s default LangSys table.

        // FIXME: We assume that the language system is always "dflt".
        auto langSys = try$(scriptTable.defaultLangSys());

        // 3. The LangSys table provides index numbers into the GPOS FeatureList
        //    table to access a required feature and a number of additional features.
        Opt<FeatureTable> kernFeatureTable;
        auto features = try$(featureList());
        for (u16 featureIndex : try$(langSys.featureIndices())) {
            auto featureTable = try$(features.at(featureIndex));

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
        auto lookups = try$(lookupList());
        for (u16 lookupIndex : try$(kernFeatureTable->lookupIndices())) {
            auto lookupTable = try$(lookups.at(lookupIndex));

            // FIXME: We only support pair adjustment lookups.
            if (try$(lookupTable.lookupType()) != (u16)GposLookupType::PAIR_ADJUSTMENT)
                continue;

            for (u16 offset : try$(lookupTable.subtableOffsets())) {
                auto lookupSubtable = try$(lookupTable.subtable<GposLookupSubtable>(offset));
                if (auto glyphPair = lookupSubtable.is<GposGlyphPairAdjustment>()) {
                    auto pair = try$(glyphPair->adjustments(prev, curr));
                    if (pair)
                        return Ok(*pair);
                } else if (auto classPair = lookupSubtable.is<GposClassPairAdjustment>()) {
                    auto pair = try$(classPair->adjustments(prev, curr));
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
