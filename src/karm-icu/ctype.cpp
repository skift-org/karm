export module Karm.Icu:ctype;

import Karm.Core;

import :ucd;

namespace Karm::Icu {

// https://www.unicode.org/reports/tr18/#Compatibility_Properties

// https://www.unicode.org/reports/tr18/#alpha
// \p{Alphabetic}
export bool isAlpha(Rune rune) {
    return Properties::of(rune).alphabetic();
}

// https://www.unicode.org/reports/tr18/#lower
// \p{Lowercase}
export bool isLower(Rune rune) {
    return Properties::of(rune).lowercase();
}

// https://www.unicode.org/reports/tr18/#upper
// \p{Uppercase}
export bool isUpper(Rune rune) {
    return Properties::of(rune).uppercase();
}

// https://www.unicode.org/reports/tr18/#punct
// \p{gc=Punctuation}
export bool isPunct(Rune rune) {
    return oneOf(
        Properties::of(rune).generalCategory(),
        GeneralCategory::CONNECTOR_PUNCTUATION,
        GeneralCategory::DASH_PUNCTUATION,
        GeneralCategory::OPEN_PUNCTUATION,
        GeneralCategory::CLOSE_PUNCTUATION,
        GeneralCategory::INITIAL_PUNCTUATION,
        GeneralCategory::FINAL_PUNCTUATION,
        GeneralCategory::OTHER_PUNCTUATION
    );
}

// https://www.unicode.org/reports/tr18/#digit
// \p{gc=Decimal_Number}
export bool isDigit(Rune rune) {
    return Properties::of(rune).generalCategory() == GeneralCategory::DECIMAL_NUMBER;
}

// https://www.unicode.org/reports/tr18/#xdigit
// \p{gc=Decimal_Number} \p{Hex_Digit}
export bool isXdigit(Rune rune) {
    return isDigit(rune) or Properties::of(rune).hexDigit();
}

// https://www.unicode.org/reports/tr18/#alnum
// \p{alpha} \p{digit}
export bool isAlnum(Rune rune) {
    return isAlpha(rune) or isDigit(rune);
}

// https://www.unicode.org/reports/tr18/#space
// \p{Whitespace}
export bool isSpace(Rune rune) {
    return Properties::of(rune).whiteSpace();
}

// https://www.unicode.org/reports/tr18/#blank
// \p{gc=Space_Separator} \N{CHARACTER TABULATION}
export bool isBlank(Rune rune) {
    return Properties::of(rune).generalCategory() == GeneralCategory::SPACE_SEPARATOR or rune == '\t';
}

// https://www.unicode.org/reports/tr18/#cntrl
// \p{gc=Control}
export bool isCntrl(Rune rune) {
    return Properties::of(rune).generalCategory() == GeneralCategory::CONTROL;
}

// https://www.unicode.org/reports/tr18/#graph
// [^ \p{space} \p{gc=Control} \p{gc=Surrogate} \p{gc=Unassigned}]
export bool isGraph(Rune rune) {
    return not isSpace(rune) and
           not oneOf(
               Properties::of(rune).generalCategory(),
               GeneralCategory::CONTROL,
               GeneralCategory::SURROGATE,
               GeneralCategory::UNASSIGNED
           );
}

// https://www.unicode.org/reports/tr18/#Compatibility_Properties
// \p{graph} \p{blank} -- \p{cntrl}
export bool isPrint(Rune rune) {
    return (isGraph(rune) or isBlank(rune)) and not isCntrl(rune);
}

// https://www.unicode.org/reports/tr18/#word
// \p{alpha} \p{gc=Mark} \p{digit} \p{gc=Connector_Punctuation} \p{Join_Control}
export bool isWord(Rune rune) {
    auto props = Properties::of(rune);
    return isAlpha(rune) or
           oneOf(
               props.generalCategory(),
               GeneralCategory::NONSPACING_MARK,
               GeneralCategory::SPACING_MARK,
               GeneralCategory::ENCLOSING_MARK,
               GeneralCategory::CONNECTOR_PUNCTUATION
           ) or
           isDigit(rune) or
           props.joinControl();
}

// NOSPEC: Basic Multilingual Plane, U+0000..U+FFFF
export constexpr bool isBmp(Rune rune) {
    return rune <= 0xFFFF;
}

// NOSPEC: Supplementary planes, U+10000..U+10FFFF
export constexpr bool isSupplementary(Rune rune) {
    return rune >= 0x10000 and rune <= 0x10FFFF;
}

} // namespace Karm::Icu
