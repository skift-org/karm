export module Karm.Icu:normal;

import :ucd;

namespace Karm::Icu {

// Unicode Normalization Forms
// https://unicode.org/reports/tr15/

// https://unicode.org/reports/tr15/#Norm_Forms
export struct NormForm {
    static const NormForm NFD;
    static const NormForm NFC;
    static const NormForm NFKD;
    static const NormForm NFKC;

    int _id;

    static Opt<NormForm> detect(Str str) {
        if (NFD.quickCheck(str) == QuickCheck::YES)
            return Some(NFD);
        if (NFC.quickCheck(str) == QuickCheck::YES)
            return Some(NFC);
        if (NFKD.quickCheck(str) == QuickCheck::YES)
            return Some(NFKD);
        if (NFKC.quickCheck(str) == QuickCheck::YES)
            return Some(NFKC);
        return NONE;
    }

    // https://unicode.org/reports/tr15/#Detecting_Normalization_Forms
    QuickCheck allowed(Rune ch) const {
        auto props = Properties::of(ch);
        if (*this == NFD) {
            return props.nfdQuickCheck();
        } else if (*this == NFC) {
            return props.nfcQuickCheck();
        } else if (*this == NFKD) {
            return props.nfkdQuickCheck();
        } else if (*this == NFKC) {
            return props.nfkcQuickCheck();
        } else {
            unreachable();
        }
    }

    // https://unicode.org/reports/tr15/#Detecting_Normalization_Forms
    QuickCheck quickCheck(Str str) const {
        u8 lastCanonicalClass = 0;
        QuickCheck result = QuickCheck::YES;
        for (auto r : iterRunes(str)) {
            auto canonicalClass = toUnderlyingType(Properties::of(r).canonicalCombiningClass());
            if (lastCanonicalClass > canonicalClass and canonicalClass != 0)
                return QuickCheck::NO;
            auto check = allowed(r);
            if (check == QuickCheck::NO)
                return QuickCheck::NO;
            if (check == QuickCheck::MAYBE)
                result = QuickCheck::MAYBE;
            lastCanonicalClass = canonicalClass;
        }
        return result;
    }

    bool operator==(NormForm const&) const = default;
};

constexpr NormForm NormForm::NFD{0};
constexpr NormForm NormForm::NFC{1};
constexpr NormForm NormForm::NFKD{2};
constexpr NormForm NormForm::NFKC{3};

} // namespace Karm::Icu
