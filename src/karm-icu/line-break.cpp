module;

#include <karm/macros>

export module Karm.Icu:lineBreak;

import Karm.Core;

import :segmentation;
import :ucd;

namespace Karm::Icu {

// Unicode line breaking algorithm
// https://www.unicode.org/reports/tr14/tr14-55.html

export enum struct LineBoundary {
    MANDATORY,
    ALLOWED,
    NO,
    UNDECIDED,

    _LEN
};

export struct LineBoundaries {
    LineBoundary current = LineBoundary::NO;
    Opt<LineBoundary> undecided = NONE;
};

export struct LineToken {
    bool sot = true;
    Rune rune = 0;
    LineBreak lineBreak = LineBreak::UNKNOWN;
    GeneralCategory generalCategory = GeneralCategory::UNASSIGNED;
    EastAsianWidth eastAsianWidth = EastAsianWidth::NEUTRAL;
    bool extendedPictographic = false;

    static LineToken of(Rune r) {
        auto props = Properties::of(r);
        return {
            .sot = false,
            .rune = r,
            .lineBreak = props.lineBreak(),
            .generalCategory = props.generalCategory(),
            .eastAsianWidth = props.eastAsianWidth(),
            .extendedPictographic = props.extendedPictographic(),
        };
    }

    bool is(auto... lineBreaks) const {
        return not sot and oneOf(lineBreak, lineBreaks...);
    }

    bool letter() const {
        return not sot and oneOf(
                               generalCategory,
                               GeneralCategory::UPPERCASE_LETTER,
                               GeneralCategory::LOWERCASE_LETTER,
                               GeneralCategory::TITLECASE_LETTER,
                               GeneralCategory::MODIFIER_LETTER,
                               GeneralCategory::OTHER_LETTER
                           );
    }

    // https://www.unicode.org/reports/tr14/tr14-55.html#LB19a
    // $EastAsian = [\p{ea=F}\p{ea=W}\p{ea=H}]
    bool eastAsian() const {
        return not sot and oneOf(eastAsianWidth, EastAsianWidth::FULLWIDTH, EastAsianWidth::WIDE, EastAsianWidth::HALFWIDTH);
    }

    bool aksara() const {
        return is(LineBreak::AKSARA, LineBreak::AKSARA_START) or rune == 0x25CC;
    }
};

// https://drafts.csswg.org/css-text-3/#line-break-property
export enum struct LineStrictness {
    LOOSE,
    NORMAL,
    STRICT,
    ANYWHERE,
};

// https://drafts.csswg.org/css-text-3/#word-break-property
export enum struct WordBreaking {
    NORMAL,
    BREAK_ALL,
    KEEP_ALL,
};

// https://drafts.csswg.org/css-text-3/#line-breaking
export struct LineTailoring {
    LineStrictness strictness = LineStrictness::NORMAL;
    WordBreaking words = WordBreaking::NORMAL;
    bool hyphens = true;
    bool cjk = false;

    // https://www.unicode.org/reports/tr14/tr14-55.html#Tailoring
    // Changing the line breaking class assignment for some characters
    LineBreak resolve(LineToken const& token) const {
        auto lineBreak = token.lineBreak;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB1
        // Resolved | Original     | General_Category
        // AL       | AI, SG, XX   | Any
        // CM       | SA           | Only Mn or Mc
        // AL       | SA           | Any except Mn and Mc
        // NS       | CJ           | Any
        if (oneOf(lineBreak, LineBreak::AMBIGUOUS, LineBreak::SURROGATE, LineBreak::UNKNOWN))
            lineBreak = LineBreak::ALPHABETIC;
        else if (lineBreak == LineBreak::COMPLEX_CONTEXT)
            lineBreak = oneOf(token.generalCategory, GeneralCategory::NONSPACING_MARK, GeneralCategory::SPACING_MARK)
                            ? LineBreak::COMBINING_MARK
                            : LineBreak::ALPHABETIC;
        else if (lineBreak == LineBreak::CONDITIONAL_JAPANESE_STARTER)
            lineBreak = LineBreak::NONSTARTER;

        // https://drafts.csswg.org/css-text-3/#valdef-hyphens-none
        // Words are not hyphenated, even if characters inside the word explicitly define hyphenation opportunities.
        if (not hyphens and token.rune == 0x00AD)
            return LineBreak::COMBINING_MARK;

        // https://drafts.csswg.org/css-text-3/#line-break-property
        // The following breaks are forbidden for normal and strict line breaking and allowed in loose:
        //   breaks before Japanese small kana or the Katakana-Hiragana prolonged sound mark,
        //   i.e. characters from the Unicode line breaking class CJ.
        // https://www.unicode.org/reports/tr14/tr14-55.html#Examples
        // Example 5. […] This customization can be handled by mapping class CJ to be handled as class ID in rule LB1.
        if (strictness == LineStrictness::LOOSE and token.lineBreak == LineBreak::CONDITIONAL_JAPANESE_STARTER)
            return LineBreak::IDEOGRAPHIC;

        // The following breaks are allowed for normal and loose line breaking if the writing system is Chinese or
        // Japanese, and are otherwise forbidden:
        //   breaks before certain CJK hyphen-like characters: 〜 U+301C, ゠ U+30A0
        if (
            cjk and oneOf(strictness, LineStrictness::NORMAL, LineStrictness::LOOSE) and
            oneOf(token.rune, 0x301Cu, 0x30A0u)
        )
            return LineBreak::IDEOGRAPHIC;

        // The following breaks are forbidden for normal and strict line breaking and allowed in loose:
        //   breaks before iteration marks: 々 U+3005, 〻 U+303B, ゝ U+309D, ゞ U+309E, ヽ U+30FD, ヾ U+30FE
        if (
            strictness == LineStrictness::LOOSE and
            oneOf(token.rune, 0x3005u, 0x303Bu, 0x309Du, 0x309Eu, 0x30FDu, 0x30FEu)
        )
            return LineBreak::IDEOGRAPHIC;

        // The following breaks are allowed for loose if the writing system is Chinese or Japanese and are otherwise
        // forbidden:
        //   breaks before certain centered punctuation marks: ・ U+30FB, ： U+FF1A, ； U+FF1B, ･ U+FF65, ‼ U+203C,
        //   ⁇ U+2047, ⁈ U+2048, ⁉ U+2049, ！ U+FF01, ？ U+FF1F
        //   breaks before suffixes: Characters with the Unicode line breaking class PO and the East Asian Width
        //   property Ambiguous, Fullwidth, or Wide.
        //   breaks after prefixes: Characters with the Unicode line breaking class PR and the East Asian Width
        //   property Ambiguous, Fullwidth, or Wide.
        if (cjk and strictness == LineStrictness::LOOSE) {
            if (oneOf(token.rune, 0x30FBu, 0xFF1Au, 0xFF1Bu, 0xFF65u, 0x203Cu, 0x2047u, 0x2048u, 0x2049u, 0xFF01u, 0xFF1Fu))
                return LineBreak::IDEOGRAPHIC;

            if (
                oneOf(lineBreak, LineBreak::POSTFIX_NUMERIC, LineBreak::PREFIX_NUMERIC) and
                oneOf(token.eastAsianWidth, EastAsianWidth::AMBIGUOUS, EastAsianWidth::FULLWIDTH, EastAsianWidth::WIDE)
            )
                return LineBreak::IDEOGRAPHIC;
        }

        // https://drafts.csswg.org/css-text-3/#valdef-word-break-break-all
        // Breaking is allowed within “words”: specifically, in addition to soft wrap opportunities allowed for normal,
        // any typographic letter units (and any typographic character units resolving to the NU (“numeric”),
        // AL (“alphabetic”), or SA (“Southeast Asian”) line breaking classes) are instead treated as ID
        // (“ideographic characters”) for the purpose of line-breaking.
        // NOTE: Letters in other classes keep them: Hangul jamo and Brahmic aksaras so that LB26 and LB28a still keep
        //       their syllables together, and small kana because they are controlled by line-break instead.
        if (
            words == WordBreaking::BREAK_ALL and
            oneOf(lineBreak, LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER, LineBreak::NUMERIC)
        )
            return LineBreak::IDEOGRAPHIC;

        return lineBreak;
    }

    // https://www.unicode.org/reports/tr14/tr14-55.html#Tailoring
    // Changing the line breaking rules […] applied after rule LB12.
    Opt<LineBoundary> rule(LineToken const& left, LineToken const& current) const {
        // https://drafts.csswg.org/css-text-3/#valdef-word-break-keep-all
        // Breaking is forbidden within “words”: implicit soft wrap opportunities between typographic letter units
        // (or other typographic character units belonging to the NU, AL, AI, or ID Unicode line breaking classes)
        // are suppressed, i.e. breaks are prohibited between pairs of such characters (regardless of line-break
        // settings other than anywhere) except where opportunities exist due to dictionary-based breaking.
        auto wordLike = [](LineToken const& token) {
            return token.letter() or token.is(LineBreak::NUMERIC, LineBreak::ALPHABETIC, LineBreak::IDEOGRAPHIC);
        };
        if (words == WordBreaking::KEEP_ALL and wordLike(left) and wordLike(current))
            return Some(LineBoundary::NO);

        if (strictness == LineStrictness::LOOSE) {
            // https://drafts.csswg.org/css-text-3/#line-break-property
            // The following breaks are allowed for loose line breaking if the preceding character belongs to the
            // Unicode line breaking class ID (including when the preceding character is treated as ID due to
            // word-break: break-all), and are otherwise forbidden:
            //   breaks before hyphens: ‐ U+2010, – U+2013
            if (left.is(LineBreak::IDEOGRAPHIC) and not current.sot and oneOf(current.rune, 0x2010u, 0x2013u))
                return Some(LineBoundary::ALLOWED);

            // The following breaks are forbidden for normal and strict line breaking and allowed in loose:
            //   breaks between inseparable characters (such as ‥ U+2025, … U+2026) i.e. characters from the
            //   Unicode line breaking class IN.
            // NOTE: Read as a break between two consecutive IN characters, not before every IN character.
            if (left.is(LineBreak::INSEPARABLE) and current.is(LineBreak::INSEPARABLE))
                return Some(LineBoundary::ALLOWED);
        }

        return NONE;
    }
};

export struct LineSegmenter {
    enum struct Lookahead {
        NONE,
        FINAL_QUOTE,
        DECIMAL_MARK,
        QUOTE,
        NUMBER_OPEN,
        NUMBER_OPEN_INFIX,
        AKSARA,
    };

    enum struct NumberState {
        INITIAL,
        NUMBER,
        NUMBER_CLOSE,
    };

    enum struct RegionalIndicatorSequenceState {
        INITIAL,
        RI,
        RIRI,
    };

    LineTailoring _tailoring;
    GraphemeSegmenter _graphemes = {};
    TextEdge _text = TextEdge::START_OF_TEXT;
    LineToken _previous = {};
    LineToken _left = {};
    LineToken _leftLeft = {};
    LineToken _beforeSpaces = {};
    LineToken _beforeSpacesLeft = {};
    NumberState _numberState = NumberState::INITIAL;
    RegionalIndicatorSequenceState _regionalIndicatorSequenceState = RegionalIndicatorSequenceState::INITIAL;
    Lookahead _lookahead = Lookahead::NONE;
    LineBoundary _lookaheadFallback = LineBoundary::NO;

    LineSegmenter(LineTailoring tailoring = {}) : _tailoring(tailoring) {}

    static LineBoundary _lookaheadMatch(Lookahead l) {
        if (l == Lookahead::DECIMAL_MARK)
            return LineBoundary::ALLOWED;
        return LineBoundary::NO;
    }

    LineBoundary _resolve(LineToken const& current) {
        switch (_lookahead) {
        case Lookahead::NONE:
            return LineBoundary::UNDECIDED;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB15b
        // × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)
        case Lookahead::FINAL_QUOTE:
            if (current.is(
                    LineBreak::SPACE, LineBreak::GLUE, LineBreak::WORD_JOINER, LineBreak::CLOSE_PUNCTUATION,
                    LineBreak::QUOTATION, LineBreak::CLOSE_PARENTHESIS, LineBreak::EXCLAMATION,
                    LineBreak::INFIX_NUMERIC, LineBreak::BREAK_SYMBOLS, LineBreak::MANDATORY_BREAK,
                    LineBreak::CARRIAGE_RETURN, LineBreak::LINE_FEED, LineBreak::NEXT_LINE, LineBreak::ZWSPACE
                ))
                return LineBoundary::NO;
            return _lookaheadFallback;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB15c
        // SP ÷ IS NU
        case Lookahead::DECIMAL_MARK:
            if (current.is(LineBreak::NUMERIC))
                return LineBoundary::ALLOWED;
            return _lookaheadFallback;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB19a
        // × QU ( [^$EastAsian] | eot )
        case Lookahead::QUOTE:
            if (not current.eastAsian())
                return LineBoundary::NO;
            return _lookaheadFallback;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB25
        // PO × OP NU
        // PO × OP IS NU
        // PR × OP NU
        // PR × OP IS NU
        case Lookahead::NUMBER_OPEN:
            if (current.is(LineBreak::NUMERIC))
                return LineBoundary::NO;
            if (current.is(LineBreak::INFIX_NUMERIC)) {
                _lookahead = Lookahead::NUMBER_OPEN_INFIX;
                return LineBoundary::UNDECIDED;
            }
            return _lookaheadFallback;

        case Lookahead::NUMBER_OPEN_INFIX:
            if (current.is(LineBreak::NUMERIC))
                return LineBoundary::NO;
            return _lookaheadFallback;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB28a
        // (AK | [◌] | AS) × (AK | [◌] | AS) VF
        case Lookahead::AKSARA:
            if (current.is(LineBreak::VIRAMA_FINAL))
                return LineBoundary::NO;
            return _lookaheadFallback;
        }
    }

    LineBoundary _resolveEndOfText() {
        switch (_lookahead) {
        case Lookahead::NONE:
            return LineBoundary::UNDECIDED;

        case Lookahead::FINAL_QUOTE:
        case Lookahead::QUOTE:
            return LineBoundary::NO;

        case Lookahead::DECIMAL_MARK:
        case Lookahead::NUMBER_OPEN:
        case Lookahead::NUMBER_OPEN_INFIX:
        case Lookahead::AKSARA:
            return _lookaheadFallback;
        }
    }

    LineBoundary _decide(LineToken const& current, Lookahead& pending) {
        auto lookaheadFor = [&](Lookahead l) {
            if (pending == Lookahead::NONE)
                pending = l;
        };

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB11
        // × WJ
        // WJ ×
        if (current.is(LineBreak::WORD_JOINER) or _left.is(LineBreak::WORD_JOINER))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB12
        // GL ×
        if (_left.is(LineBreak::GLUE))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#Tailoring
        // Changing the line breaking rules […] applied after rule LB12.
        if (auto tailored = _tailoring.rule(_left, current))
            return *tailored;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB12a
        // [^SP BA HY HH] × GL
        if (
            not _left.is(LineBreak::SPACE, LineBreak::BREAK_AFTER, LineBreak::HYPHEN, LineBreak::UNAMBIGUOUS_HYPHEN) and
            current.is(LineBreak::GLUE)
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB13
        // × CL
        // × CP
        // × EX
        // × SY
        if (current.is(LineBreak::CLOSE_PUNCTUATION, LineBreak::CLOSE_PARENTHESIS, LineBreak::EXCLAMATION, LineBreak::BREAK_SYMBOLS))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB14
        // OP SP* ×
        if (_beforeSpaces.is(LineBreak::OPEN_PUNCTUATION))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB15a
        // (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW) [\p{Pi}&QU] SP* ×
        if (
            _beforeSpaces.is(LineBreak::QUOTATION) and
            _beforeSpaces.generalCategory == GeneralCategory::INITIAL_PUNCTUATION and
            (_beforeSpacesLeft.sot or
             _beforeSpacesLeft.is(
                 LineBreak::MANDATORY_BREAK, LineBreak::CARRIAGE_RETURN, LineBreak::LINE_FEED, LineBreak::NEXT_LINE,
                 LineBreak::OPEN_PUNCTUATION, LineBreak::QUOTATION, LineBreak::GLUE, LineBreak::SPACE, LineBreak::ZWSPACE
             ))
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB15b
        // × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)
        if (current.is(LineBreak::QUOTATION) and current.generalCategory == GeneralCategory::FINAL_PUNCTUATION)
            lookaheadFor(Lookahead::FINAL_QUOTE);

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB15c
        // SP ÷ IS NU
        if (_left.is(LineBreak::SPACE) and current.is(LineBreak::INFIX_NUMERIC))
            lookaheadFor(Lookahead::DECIMAL_MARK);

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB15d
        // × IS
        if (current.is(LineBreak::INFIX_NUMERIC))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB16
        // (CL | CP) SP* × NS
        if (_beforeSpaces.is(LineBreak::CLOSE_PUNCTUATION, LineBreak::CLOSE_PARENTHESIS) and current.is(LineBreak::NONSTARTER))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB17
        // B2 SP* × B2
        if (_beforeSpaces.is(LineBreak::BREAK_BOTH) and current.is(LineBreak::BREAK_BOTH))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB18
        // SP ÷
        if (_left.is(LineBreak::SPACE))
            return LineBoundary::ALLOWED;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB19
        // × [ QU - \p{Pi} ]
        // [ QU - \p{Pf} ] ×
        if (current.is(LineBreak::QUOTATION) and current.generalCategory != GeneralCategory::INITIAL_PUNCTUATION)
            return LineBoundary::NO;
        if (_left.is(LineBreak::QUOTATION) and _left.generalCategory != GeneralCategory::FINAL_PUNCTUATION)
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB19a
        // [^$EastAsian] × QU
        // × QU ( [^$EastAsian] | eot )
        // QU × [^$EastAsian]
        // ( sot | [^$EastAsian] ) QU ×
        if (current.is(LineBreak::QUOTATION) and not _left.eastAsian())
            return LineBoundary::NO;
        if (current.is(LineBreak::QUOTATION))
            lookaheadFor(Lookahead::QUOTE);
        if (_left.is(LineBreak::QUOTATION) and not current.eastAsian())
            return LineBoundary::NO;
        if (_left.is(LineBreak::QUOTATION) and (_leftLeft.sot or not _leftLeft.eastAsian()))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB20
        // ÷ CB
        // CB ÷
        if (current.is(LineBreak::CONTINGENT_BREAK) or _left.is(LineBreak::CONTINGENT_BREAK))
            return LineBoundary::ALLOWED;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB20a
        // ( sot | BK | CR | LF | NL | SP | ZW | CB | GL ) ( HY | HH ) × ( AL | HL )
        if (
            (_leftLeft.sot or
             _leftLeft.is(
                 LineBreak::MANDATORY_BREAK, LineBreak::CARRIAGE_RETURN, LineBreak::LINE_FEED, LineBreak::NEXT_LINE,
                 LineBreak::SPACE, LineBreak::ZWSPACE, LineBreak::CONTINGENT_BREAK, LineBreak::GLUE
             )) and
            _left.is(LineBreak::HYPHEN, LineBreak::UNAMBIGUOUS_HYPHEN) and
            current.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER)
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB21
        // × BA
        // × HH
        // × HY
        // × NS
        // BB ×
        if (current.is(LineBreak::BREAK_AFTER, LineBreak::UNAMBIGUOUS_HYPHEN, LineBreak::HYPHEN, LineBreak::NONSTARTER))
            return LineBoundary::NO;
        if (_left.is(LineBreak::BREAK_BEFORE))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB21a
        // HL (HY | HH) × [^HL]
        if (
            _leftLeft.is(LineBreak::HEBREW_LETTER) and
            _left.is(LineBreak::HYPHEN, LineBreak::UNAMBIGUOUS_HYPHEN) and
            not current.is(LineBreak::HEBREW_LETTER)
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB21b
        // SY × HL
        if (_left.is(LineBreak::BREAK_SYMBOLS) and current.is(LineBreak::HEBREW_LETTER))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB22
        // × IN
        if (current.is(LineBreak::INSEPARABLE))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB23
        // (AL | HL) × NU
        // NU × (AL | HL)
        if (_left.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER) and current.is(LineBreak::NUMERIC))
            return LineBoundary::NO;
        if (_left.is(LineBreak::NUMERIC) and current.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB23a
        // PR × (ID | EB | EM)
        // (ID | EB | EM) × PO
        if (_left.is(LineBreak::PREFIX_NUMERIC) and current.is(LineBreak::IDEOGRAPHIC, LineBreak::E_BASE, LineBreak::E_MODIFIER))
            return LineBoundary::NO;
        if (_left.is(LineBreak::IDEOGRAPHIC, LineBreak::E_BASE, LineBreak::E_MODIFIER) and current.is(LineBreak::POSTFIX_NUMERIC))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB24
        // (PR | PO) × (AL | HL)
        // (AL | HL) × (PR | PO)
        if (_left.is(LineBreak::PREFIX_NUMERIC, LineBreak::POSTFIX_NUMERIC) and current.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER))
            return LineBoundary::NO;
        if (_left.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER) and current.is(LineBreak::PREFIX_NUMERIC, LineBreak::POSTFIX_NUMERIC))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB25
        // NU ( SY | IS )* CL × PO
        // NU ( SY | IS )* CP × PO
        // NU ( SY | IS )* CL × PR
        // NU ( SY | IS )* CP × PR
        // NU ( SY | IS )* × PO
        // NU ( SY | IS )* × PR
        // PO × OP NU
        // PO × OP IS NU
        // PO × NU
        // PR × OP NU
        // PR × OP IS NU
        // PR × NU
        // HY × NU
        // IS × NU
        // NU ( SY | IS )* × NU
        if (
            oneOf(_numberState, NumberState::NUMBER, NumberState::NUMBER_CLOSE) and
            current.is(LineBreak::POSTFIX_NUMERIC, LineBreak::PREFIX_NUMERIC)
        )
            return LineBoundary::NO;
        if (_left.is(LineBreak::POSTFIX_NUMERIC, LineBreak::PREFIX_NUMERIC) and current.is(LineBreak::OPEN_PUNCTUATION))
            lookaheadFor(Lookahead::NUMBER_OPEN);
        if (
            _left.is(LineBreak::POSTFIX_NUMERIC, LineBreak::PREFIX_NUMERIC, LineBreak::HYPHEN, LineBreak::INFIX_NUMERIC) and
            current.is(LineBreak::NUMERIC)
        )
            return LineBoundary::NO;
        if (_numberState == NumberState::NUMBER and current.is(LineBreak::NUMERIC))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB26
        // JL × (JL | JV | H2 | H3)
        // (JV | H2) × (JV | JT)
        // (JT | H3) × JT
        if (_left.is(LineBreak::JL) and current.is(LineBreak::JL, LineBreak::JV, LineBreak::H2, LineBreak::H3))
            return LineBoundary::NO;
        if (_left.is(LineBreak::JV, LineBreak::H2) and current.is(LineBreak::JV, LineBreak::JT))
            return LineBoundary::NO;
        if (_left.is(LineBreak::JT, LineBreak::H3) and current.is(LineBreak::JT))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB27
        // (JL | JV | JT | H2 | H3) × PO
        // PR × (JL | JV | JT | H2 | H3)
        if (_left.is(LineBreak::JL, LineBreak::JV, LineBreak::JT, LineBreak::H2, LineBreak::H3) and current.is(LineBreak::POSTFIX_NUMERIC))
            return LineBoundary::NO;
        if (_left.is(LineBreak::PREFIX_NUMERIC) and current.is(LineBreak::JL, LineBreak::JV, LineBreak::JT, LineBreak::H2, LineBreak::H3))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB28
        // (AL | HL) × (AL | HL)
        if (_left.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER) and current.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB28a
        // AP × (AK | [◌] | AS)
        // (AK | [◌] | AS) × (VF | VI)
        // (AK | [◌] | AS) VI × (AK | [◌])
        // (AK | [◌] | AS) × (AK | [◌] | AS) VF
        if (_left.is(LineBreak::AKSARA_PREBASE) and current.aksara())
            return LineBoundary::NO;
        if (_left.aksara() and current.is(LineBreak::VIRAMA_FINAL, LineBreak::VIRAMA))
            return LineBoundary::NO;
        if (_leftLeft.aksara() and _left.is(LineBreak::VIRAMA) and (current.is(LineBreak::AKSARA) or current.rune == 0x25CC))
            return LineBoundary::NO;
        if (_left.aksara() and current.aksara())
            lookaheadFor(Lookahead::AKSARA);

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB29
        // IS × (AL | HL)
        if (_left.is(LineBreak::INFIX_NUMERIC) and current.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER))
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB30
        // (AL | HL | NU) × [OP-$EastAsian]
        // [CP-$EastAsian] × (AL | HL | NU)
        if (
            _left.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER, LineBreak::NUMERIC) and
            current.is(LineBreak::OPEN_PUNCTUATION) and not current.eastAsian()
        )
            return LineBoundary::NO;
        if (
            _left.is(LineBreak::CLOSE_PARENTHESIS) and not _left.eastAsian() and
            current.is(LineBreak::ALPHABETIC, LineBreak::HEBREW_LETTER, LineBreak::NUMERIC)
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB30a
        // sot (RI RI)* RI × RI
        // [^RI] (RI RI)* RI × RI
        if (
            _regionalIndicatorSequenceState == RegionalIndicatorSequenceState::RI and
            current.is(LineBreak::REGIONAL_INDICATOR)
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB30b
        // EB × EM
        // [\p{Extended_Pictographic}&\p{Cn}] × EM
        if (_left.is(LineBreak::E_BASE) and current.is(LineBreak::E_MODIFIER))
            return LineBoundary::NO;
        if (
            not _left.sot and _left.extendedPictographic and _left.generalCategory == GeneralCategory::UNASSIGNED and
            current.is(LineBreak::E_MODIFIER)
        )
            return LineBoundary::NO;

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB31
        // ALL ÷
        // ÷ ALL
        return LineBoundary::ALLOWED;
    }

    LineBoundaries advance(Rune r) {
        auto raw = LineToken::of(r);
        raw.lineBreak = _tailoring.resolve(raw);
        auto grapheme = _graphemes.advance(r);

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB9
        // Treat X (CM | ZWJ)* as if it were X.
        // where X is any line break class except BK, CR, LF, NL, SP, or ZW.
        bool ignored =
            not _left.sot and
            not _left.is(
                LineBreak::MANDATORY_BREAK, LineBreak::CARRIAGE_RETURN, LineBreak::LINE_FEED,
                LineBreak::NEXT_LINE, LineBreak::SPACE, LineBreak::ZWSPACE
            ) and
            raw.is(LineBreak::COMBINING_MARK, LineBreak::ZWJ);

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB10
        // Treat any remaining CM or ZWJ as if it had the properties of U+0041 A LATIN CAPITAL LETTER A,
        // that is, Line_Break=AL, General_Category=Lu, East_Asian_Width=Na, Extended_Pictographic=N.
        auto current = raw;
        if (not ignored and raw.is(LineBreak::COMBINING_MARK, LineBreak::ZWJ)) {
            current = LineToken::of('A');
            current.lineBreak = _tailoring.resolve(current);
        }

        Defer _ = [&] {
            _previous = raw;
        };
        Defer _ = [&] {
            _text = TextEdge::IN_TEXT;
        };
        Defer _ = [&] {
            if (ignored)
                return;

            if (not current.is(LineBreak::SPACE)) {
                _beforeSpacesLeft = _left;
                _beforeSpaces = current;
            }
            _leftLeft = _left;
            _left = current;

            if (current.is(LineBreak::NUMERIC))
                _numberState = NumberState::NUMBER;
            else if (_numberState == NumberState::NUMBER and current.is(LineBreak::BREAK_SYMBOLS, LineBreak::INFIX_NUMERIC))
                _numberState = NumberState::NUMBER;
            else if (_numberState == NumberState::NUMBER and current.is(LineBreak::CLOSE_PUNCTUATION, LineBreak::CLOSE_PARENTHESIS))
                _numberState = NumberState::NUMBER_CLOSE;
            else
                _numberState = NumberState::INITIAL;

            if (current.is(LineBreak::REGIONAL_INDICATOR)) {
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
        };

        Opt<LineBoundary> resolved = NONE;
        if (_lookahead != Lookahead::NONE and not ignored) {
            auto resolution = _resolve(current);
            if (resolution != LineBoundary::UNDECIDED) {
                _lookahead = Lookahead::NONE;
                resolved = Some(resolution);
            }
        }

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB2
        // sot ×
        if (_text == TextEdge::START_OF_TEXT)
            return {LineBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB4
        // BK !
        if (_previous.is(LineBreak::MANDATORY_BREAK))
            return {LineBoundary::MANDATORY, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB5
        // CR × LF
        // CR !
        // LF !
        // NL !
        if (_previous.is(LineBreak::CARRIAGE_RETURN) and raw.is(LineBreak::LINE_FEED))
            return {LineBoundary::NO, resolved};
        if (_previous.is(LineBreak::CARRIAGE_RETURN, LineBreak::LINE_FEED, LineBreak::NEXT_LINE))
            return {LineBoundary::MANDATORY, resolved};

        // https://drafts.csswg.org/css-text-3/#valdef-line-break-anywhere
        // There is a soft wrap opportunity around every typographic character unit, including around any
        // punctuation character or preserved white spaces, or in the middle of words, disregarding any prohibition
        // against line breaks, even those introduced by characters with the GL, WJ, or ZWJ line breaking classes or
        // mandated by the word-break property.
        // https://www.unicode.org/reports/tr14/tr14-55.html#Examples
        // Example 2. To implement terminal style line breaks […] Such a modification essentially disregards the
        // output of the line breaking algorithm, and is therefore not a conformant tailoring.
        if (_tailoring.strictness == LineStrictness::ANYWHERE)
            return {grapheme == GraphemeBoundary::YES ? LineBoundary::ALLOWED : LineBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB6
        // × ( BK | CR | LF | NL )
        if (raw.is(LineBreak::MANDATORY_BREAK, LineBreak::CARRIAGE_RETURN, LineBreak::LINE_FEED, LineBreak::NEXT_LINE))
            return {LineBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB7
        // × SP
        // × ZW
        if (raw.is(LineBreak::SPACE, LineBreak::ZWSPACE))
            return {LineBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB8
        // ZW SP* ÷
        if (_beforeSpaces.is(LineBreak::ZWSPACE))
            return {LineBoundary::ALLOWED, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB8a
        // ZWJ ×
        if (_previous.is(LineBreak::ZWJ))
            return {LineBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB9
        // Treat X (CM | ZWJ)* as if it were X.
        if (ignored)
            return {LineBoundary::NO, resolved};

        Lookahead pending = Lookahead::NONE;
        auto decision = _decide(current, pending);
        if (pending != Lookahead::NONE and decision != _lookaheadMatch(pending)) {
            _lookahead = pending;
            _lookaheadFallback = decision;
            return {LineBoundary::UNDECIDED, resolved};
        }

        return {decision, resolved};
    }

    LineBoundaries advance(TextEdge e) {
        Defer _ = [&] {
            _text = e;
        };

        Opt<LineBoundary> resolved = NONE;
        if (_lookahead != Lookahead::NONE) {
            resolved = Some(_resolveEndOfText());
            _lookahead = Lookahead::NONE;
        }

        if (_text == TextEdge::START_OF_TEXT and e == TextEdge::END_OF_TEXT)
            return {LineBoundary::NO, resolved};

        // https://www.unicode.org/reports/tr14/tr14-55.html#LB3
        // ! eot
        return {LineBoundary::MANDATORY, resolved};
    }
};

} // namespace Karm::Icu
