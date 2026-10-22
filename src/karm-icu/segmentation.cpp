module;

#include <karm/macros>

export module Karm.Icu:segmentation;

import Karm.Core;

import :base;
import :ctype;
import :ucd;

namespace Karm::Icu {

// Unicode text segmentation algorithm
// https://unicode.org/reports/tr29/

export enum struct TextEdge {
    START_OF_TEXT,
    IN_TEXT,
    END_OF_TEXT
};

// MARK: Grapheme Cluster Boundaries -------------------------------------------

export enum struct GraphemeCluster {
    LEGACY,
    EXTENDED,

    _LEN,
};

export enum struct GraphemeBoundary {
    YES,
    NO,

    _LEN,
};

export struct GraphemeSegmenter {
    enum struct IndicConjunctState {
        INITIAL,
        AFTER_CONSONANT,
        AFTER_LINKER,
    };

    enum struct EmojiSequenceState {
        INITIAL,
        AFTER_EXTENDED_PICTOGRAPHIC,
        AFTER_ZWJ
    };

    enum struct RegionalIndicatorSequenceState {
        INITIAL,
        RI,
        RIRI,
    };

    GraphemeCluster _type;
    TextEdge _text = TextEdge::START_OF_TEXT;
    GraphemeClusterBreak _previous = GraphemeClusterBreak::OTHER;
    IndicConjunctState _indicConjunctState = IndicConjunctState::INITIAL;
    EmojiSequenceState _emojiSequenceState = EmojiSequenceState::INITIAL;
    RegionalIndicatorSequenceState _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::INITIAL;

    GraphemeSegmenter(GraphemeCluster type = GraphemeCluster::EXTENDED)
        : _type(type) {}

    // https://www.unicode.org/reports/tr29/tr29-47.html#Grapheme_Cluster_Boundary_Rules
    GraphemeBoundary advance(Rune r) {
        // My life for match expression in C++
        auto current = Properties::of(r);
        Defer _ = [&] {
            _previous = current.graphemeClusterBreak();
        };
        Defer _ = [&] {
            _text = TextEdge::IN_TEXT;
        };

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB9c
        // \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker} [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}
        auto indicConjunct = _indicConjunctState;
        if (current.indicConjunctBreak() == IndicConjunctBreak::CONSONANT) {
            _indicConjunctState = IndicConjunctState::AFTER_CONSONANT;
        } else if (_indicConjunctState != IndicConjunctState::INITIAL and
                   current.indicConjunctBreak() == IndicConjunctBreak::LINKER) {
            _indicConjunctState = IndicConjunctState::AFTER_LINKER;
        } else if (current.indicConjunctBreak() != IndicConjunctBreak::EXTEND) {
            _indicConjunctState = IndicConjunctState::INITIAL;
        }

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB11
        // \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
        auto emojiSequence = _emojiSequenceState;
        if (current.extendedPictographic()) {
            _emojiSequenceState = EmojiSequenceState::AFTER_EXTENDED_PICTOGRAPHIC;
        } else if (_emojiSequenceState == EmojiSequenceState::AFTER_EXTENDED_PICTOGRAPHIC and
                   current.graphemeClusterBreak() == GraphemeClusterBreak::EXTEND) {
            _emojiSequenceState = EmojiSequenceState::AFTER_EXTENDED_PICTOGRAPHIC;
        } else if (_emojiSequenceState == EmojiSequenceState::AFTER_EXTENDED_PICTOGRAPHIC and
                   current.graphemeClusterBreak() == GraphemeClusterBreak::ZWJ) {
            _emojiSequenceState = EmojiSequenceState::AFTER_ZWJ;
        } else {
            _emojiSequenceState = EmojiSequenceState::INITIAL;
        }

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB12
        // sot (RI RI)* RI × RI
        // https://www.unicode.org/reports/tr29/tr29-47.html#GB13
        // [^RI] (RI RI)* RI × RI
        auto regionalIndicatorSequence = _regionalIndicatorSequenceState;
        if (current.graphemeClusterBreak() == GraphemeClusterBreak::REGIONAL_INDICATOR) {
            switch (_regionalIndicatorSequenceState) {
            case RegionalIndicatorSequenceState::INITIAL:
                _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::RI;
                break;
            case RegionalIndicatorSequenceState::RI:
                _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::RIRI;
                break;
            case RegionalIndicatorSequenceState::RIRI:
                _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::RI;
                break;
            }
        } else {
            _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::INITIAL;
        }

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB1
        // sot ÷ Any
        if (_text == TextEdge::START_OF_TEXT)
            return GraphemeBoundary::YES;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB3
        // CR × LF
        if (_previous == GraphemeClusterBreak::CR and
            current.graphemeClusterBreak() == GraphemeClusterBreak::LF)
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB4
        // (Control | CR | LF) ÷
        if (oneOf(_previous, GraphemeClusterBreak::CONTROL, GraphemeClusterBreak::CR, GraphemeClusterBreak::LF))
            return GraphemeBoundary::YES;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB5
        // ÷ (Control | CR | LF)
        if (oneOf(current.graphemeClusterBreak(), GraphemeClusterBreak::CONTROL, GraphemeClusterBreak::CR, GraphemeClusterBreak::LF))
            return GraphemeBoundary::YES;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB6
        // L × (L | V | LV | LVT)
        if (
            _previous == GraphemeClusterBreak::L and
            oneOf(current.graphemeClusterBreak(), GraphemeClusterBreak::L, GraphemeClusterBreak::V, GraphemeClusterBreak::LV, GraphemeClusterBreak::LVT)
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB7
        // (LV | V) × (V | T)
        if (
            oneOf(_previous, GraphemeClusterBreak::LV, GraphemeClusterBreak::V) and
            oneOf(current.graphemeClusterBreak(), GraphemeClusterBreak::V, GraphemeClusterBreak::T)
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB8
        // (LVT | T) × T
        if (
            oneOf(_previous, GraphemeClusterBreak::LVT, GraphemeClusterBreak::T) and
            current.graphemeClusterBreak() == GraphemeClusterBreak::T
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB9
        // × (Extend | ZWJ)
        if (
            oneOf(current.graphemeClusterBreak(), GraphemeClusterBreak::EXTEND, GraphemeClusterBreak::ZWJ)
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB9a
        // × SpacingMark
        if (
            _type != GraphemeCluster::LEGACY and
            current.graphemeClusterBreak() == GraphemeClusterBreak::SPACINGMARK
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB9b
        // Prepend ×
        if (
            _type != GraphemeCluster::LEGACY and
            _previous == GraphemeClusterBreak::PREPEND
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB9c
        // \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker} [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}
        if (
            _type != GraphemeCluster::LEGACY and
            indicConjunct == IndicConjunctState::AFTER_LINKER and
            current.indicConjunctBreak() == IndicConjunctBreak::CONSONANT
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB11
        // \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
        if (
            emojiSequence == EmojiSequenceState::AFTER_ZWJ and
            current.extendedPictographic()
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB12
        // sot (RI RI)* RI × RI
        // https://www.unicode.org/reports/tr29/tr29-47.html#GB13
        // [^RI] (RI RI)* RI × RI
        if (
            regionalIndicatorSequence == RegionalIndicatorSequenceState::RI and
            current.graphemeClusterBreak() == GraphemeClusterBreak::REGIONAL_INDICATOR
        )
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB999
        // Any ÷ Any
        return GraphemeBoundary::YES;
    }

    GraphemeBoundary advance(TextEdge e) {
        Defer _ = [&] {
            _text = e;
        };
        if (_text == TextEdge::START_OF_TEXT and e == TextEdge::END_OF_TEXT)
            return GraphemeBoundary::NO;

        // https://www.unicode.org/reports/tr29/tr29-47.html#GB2
        // Any ÷ eot
        return GraphemeBoundary::YES;
    }
};

export usize countGrapheneClusters(Str s, GraphemeCluster cluster = GraphemeCluster::EXTENDED) {
    GraphemeSegmenter segmenter{cluster};
    usize n = 0;
    for (auto r : iterRunes(s))
        if (segmenter.advance(r) == GraphemeBoundary::YES)
            n++;
    return n;
}

// MARK: Word Boundaries -------------------------------------------------------

export enum struct WordBoundary {
    YES,
    NO,
    UNDECIDED,
};

export struct WordBoundaries {
    WordBoundary current = WordBoundary::NO;
    Opt<WordBoundary> undecided = NONE;
};

export struct WordSegmenter {
    enum struct RegionalIndicatorSequenceState {
        INITIAL,
        RI,
        RIRI,
    };

    TextEdge _text = TextEdge::START_OF_TEXT;
    WordBreak _previous = WordBreak::OTHER;
    WordBreak _left = WordBreak::OTHER;
    WordBreak _leftLeft = WordBreak::OTHER;
    bool _undecided = false;
    RegionalIndicatorSequenceState _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::INITIAL;

    WordSegmenter() {}

    // https://www.unicode.org/reports/tr29/tr29-47.html#WB_Rule_Macros
    // AHLetter (ALetter | Hebrew_Letter)
    static bool _ahLetter(WordBreak wb) {
        return oneOf(wb, WordBreak::ALETTER, WordBreak::HEBREW_LETTER);
    }

    // https://www.unicode.org/reports/tr29/tr29-47.html#WB_Rule_Macros
    // MidNumLetQ (MidNumLet | Single_Quote)
    static bool _midNumLetQ(WordBreak wb) {
        return oneOf(wb, WordBreak::MIDNUMLET, WordBreak::SINGLE_QUOTE);
    }

    // https://www.unicode.org/reports/tr29/tr29-47.html#Word_Boundary_Rules
    WordBoundaries advance(Rune r) {
        auto current = Properties::of(r);
        auto currentBreak = current.wordBreak();

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB4
        // X (Extend | Format | ZWJ)* → X
        bool ignored = _text != TextEdge::START_OF_TEXT and
                       not oneOf(_previous, WordBreak::NEWLINE, WordBreak::CR, WordBreak::LF) and
                       oneOf(currentBreak, WordBreak::EXTEND, WordBreak::FORMAT, WordBreak::ZWJ);

        Defer _ = [&] {
            _previous = currentBreak;
        };
        Defer _ = [&] {
            _text = TextEdge::IN_TEXT;
        };
        Defer _ = [&] {
            if (ignored)
                return;
            _leftLeft = _left;
            _left = currentBreak;
        };

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB15
        // sot (RI RI)* RI × RI
        // https://www.unicode.org/reports/tr29/tr29-47.html#WB16
        // [^RI] (RI RI)* RI × RI
        auto regionalIndicatorSequence = _regionalIndicatorSequenceState;
        if (not ignored) {
            if (currentBreak == WordBreak::REGIONAL_INDICATOR) {
                switch (_regionalIndicatorSequenceState) {
                case RegionalIndicatorSequenceState::INITIAL:
                    _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::RI;
                    break;
                case RegionalIndicatorSequenceState::RI:
                    _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::RIRI;
                    break;
                case RegionalIndicatorSequenceState::RIRI:
                    _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::RI;
                    break;
                }
            } else {
                _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::INITIAL;
            }
        }

        Opt<WordBoundary> resolved = NONE;
        if (_undecided and not ignored) {
            _undecided = false;
            auto resolution = WordBoundary::YES;

            // https://www.unicode.org/reports/tr29/tr29-47.html#WB6
            // AHLetter × (MidLetter | MidNumLetQ) AHLetter
            if (
                _ahLetter(_leftLeft) and
                (_left == WordBreak::MIDLETTER or _midNumLetQ(_left)) and
                _ahLetter(currentBreak)
            )
                resolution = WordBoundary::NO;

            // https://www.unicode.org/reports/tr29/tr29-47.html#WB7b
            // Hebrew_Letter × Double_Quote Hebrew_Letter
            if (
                _leftLeft == WordBreak::HEBREW_LETTER and
                _left == WordBreak::DOUBLE_QUOTE and
                currentBreak == WordBreak::HEBREW_LETTER
            )
                resolution = WordBoundary::NO;

            // https://www.unicode.org/reports/tr29/tr29-47.html#WB12
            // Numeric × (MidNum | MidNumLetQ) Numeric
            if (
                _leftLeft == WordBreak::NUMERIC and
                (_left == WordBreak::MIDNUM or _midNumLetQ(_left)) and
                currentBreak == WordBreak::NUMERIC
            )
                resolution = WordBoundary::NO;

            resolved = Some(resolution);
        }

        bool pending = false;

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB1
        // sot ÷ Any
        if (_text == TextEdge::START_OF_TEXT)
            return {WordBoundary::YES, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB3
        // CR × LF
        if (_previous == WordBreak::CR and currentBreak == WordBreak::LF)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB3a
        // (Newline | CR | LF) ÷
        if (oneOf(_previous, WordBreak::NEWLINE, WordBreak::CR, WordBreak::LF))
            return {WordBoundary::YES, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB3b
        // ÷ (Newline | CR | LF)
        if (oneOf(currentBreak, WordBreak::NEWLINE, WordBreak::CR, WordBreak::LF))
            return {WordBoundary::YES, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB3c
        // ZWJ × \p{Extended_Pictographic}
        if (_previous == WordBreak::ZWJ and current.extendedPictographic())
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB3d
        // WSegSpace × WSegSpace
        if (_previous == WordBreak::WSEGSPACE and currentBreak == WordBreak::WSEGSPACE)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB4
        // X (Extend | Format | ZWJ)* → X
        if (ignored)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB5
        // AHLetter × AHLetter
        if (_ahLetter(_left) and _ahLetter(currentBreak))
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB6
        // AHLetter × (MidLetter | MidNumLetQ) AHLetter
        if (_ahLetter(_left) and (currentBreak == WordBreak::MIDLETTER or _midNumLetQ(currentBreak)))
            pending = true;

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB7
        // AHLetter (MidLetter | MidNumLetQ) × AHLetter
        if (
            _ahLetter(_leftLeft) and
            (_left == WordBreak::MIDLETTER or _midNumLetQ(_left)) and
            _ahLetter(currentBreak)
        )
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB7a
        // Hebrew_Letter × Single_Quote
        if (_left == WordBreak::HEBREW_LETTER and currentBreak == WordBreak::SINGLE_QUOTE)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB7b
        // Hebrew_Letter × Double_Quote Hebrew_Letter
        if (_left == WordBreak::HEBREW_LETTER and currentBreak == WordBreak::DOUBLE_QUOTE)
            pending = true;

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB7c
        // Hebrew_Letter Double_Quote × Hebrew_Letter
        if (
            _leftLeft == WordBreak::HEBREW_LETTER and
            _left == WordBreak::DOUBLE_QUOTE and
            currentBreak == WordBreak::HEBREW_LETTER
        )
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB8
        // Numeric × Numeric
        if (_left == WordBreak::NUMERIC and currentBreak == WordBreak::NUMERIC)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB9
        // AHLetter × Numeric
        if (_ahLetter(_left) and currentBreak == WordBreak::NUMERIC)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB10
        // Numeric × AHLetter
        if (_left == WordBreak::NUMERIC and _ahLetter(currentBreak))
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB11
        // Numeric (MidNum | MidNumLetQ) × Numeric
        if (
            _leftLeft == WordBreak::NUMERIC and
            (_left == WordBreak::MIDNUM or _midNumLetQ(_left)) and
            currentBreak == WordBreak::NUMERIC
        )
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB12
        // Numeric × (MidNum | MidNumLetQ) Numeric
        if (_left == WordBreak::NUMERIC and (currentBreak == WordBreak::MIDNUM or _midNumLetQ(currentBreak)))
            pending = true;

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB13
        // Katakana × Katakana
        if (_left == WordBreak::KATAKANA and currentBreak == WordBreak::KATAKANA)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB13a
        // (AHLetter | Numeric | Katakana | ExtendNumLet) × ExtendNumLet
        if (
            (_ahLetter(_left) or oneOf(_left, WordBreak::NUMERIC, WordBreak::KATAKANA, WordBreak::EXTENDNUMLET)) and
            currentBreak == WordBreak::EXTENDNUMLET
        )
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB13b
        // ExtendNumLet × (AHLetter | Numeric | Katakana)
        if (
            _left == WordBreak::EXTENDNUMLET and
            (_ahLetter(currentBreak) or oneOf(currentBreak, WordBreak::NUMERIC, WordBreak::KATAKANA))
        )
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB15
        // sot (RI RI)* RI × RI
        // https://www.unicode.org/reports/tr29/tr29-47.html#WB16
        // [^RI] (RI RI)* RI × RI
        if (
            regionalIndicatorSequence == RegionalIndicatorSequenceState::RI and
            currentBreak == WordBreak::REGIONAL_INDICATOR
        )
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB999
        // Any ÷ Any
        if (pending) {
            _undecided = true;
            return {WordBoundary::UNDECIDED, resolved};
        }
        return {WordBoundary::YES, resolved};
    }

    WordBoundaries advance(TextEdge e) {
        Defer _ = [&] {
            _text = e;
        };

        Opt<WordBoundary> resolved = NONE;
        if (_undecided) {
            _undecided = false;
            resolved = Some(WordBoundary::YES);
        }

        if (_text == TextEdge::START_OF_TEXT and e == TextEdge::END_OF_TEXT)
            return {WordBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr29/tr29-47.html#WB2
        // Any ÷ eot
        return {WordBoundary::YES, resolved};
    }
};

// https://www.unicode.org/reports/tr29/tr29-47.html#Word_Boundaries
export bool wordLike(Str segment) {
    for (auto r : iterRunes(segment))
        if (isAlnum(r))
            return true;
    return false;
}

export usize countWords(Str s) {
    WordSegmenter segmenter;
    RuneIter<Utf8> iter{s};
    usize n = 0;
    usize start = 0;
    usize undecidedAt = 0;

    auto collect = [&](WordBoundaries b, usize at) {
        if (b.undecided == WordBoundary::YES) {
            if (wordLike(sub(s, start, undecidedAt)))
                n++;
            start = undecidedAt;
        }

        if (b.current == WordBoundary::YES) {
            if (wordLike(sub(s, start, at)))
                n++;
            start = at;
        } else if (b.current == WordBoundary::UNDECIDED) {
            undecidedAt = at;
        }
    };

    while (not iter.endOfText()) {
        auto at = iter.pos();
        collect(segmenter.advance(iter.next()), at);
    }
    collect(segmenter.advance(TextEdge::END_OF_TEXT), iter.pos());

    return n;
}

} // namespace Karm::Icu
