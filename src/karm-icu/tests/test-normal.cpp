#include <karm/test>

import Karm.Sys;
import Karm.Core;
import Karm.Logger;
import Karm.Icu;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Icu {

// Helpers live in an anonymous namespace so they don't collide (ODR)
// with the ones in the bidi tests when linked into the same binary.
namespace {

Slice<char> strip(Slice<char> row) {
    usize l = 0;
    usize r = row.len();
    while (l < r and (row[l] == ' ' or row[l] == '\t'))
        l++;
    while (r > l and (row[r - 1] == ' ' or row[r - 1] == '\t'))
        r--;
    return sub(row, l, r);
}

Opt<Vec<Rune>> parseRunes(Slice<char> column) {
    Vec<Rune> runes;
    for (auto piece : split(strip(column), ' ')) {
        if (piece.len() == 0)
            continue;
        Io::SScan scan{piece};
        auto rune = atoi(scan, {.base = 16});
        if (not rune)
            return NONE;
        runes.pushBack(static_cast<Rune>(*rune));
    }
    if (runes.len() == 0)
        return NONE;
    return Some(std::move(runes));
}

bool sameRunes(Vec<Rune> const& a, Vec<Rune> const& b) {
    if (a.len() != b.len())
        return false;
    for (usize i = 0; i < a.len(); i++)
        if (a[i] != b[i])
            return false;
    return true;
}

String encode(Vec<Rune> const& runes) {
    StringBuilder sb;
    for (auto r : runes)
        sb.append(r);
    return sb.take();
}

// One line of NormalizationTest.txt: c1;c2;c3;c4;c5; # comment
struct NormTestCase {
    usize part = 0;
    usize line = 0;
    Vec<Vec<Rune>> columns; // c1..c5

    void repr(Io::Emit& e) const {
        e("NormTestCase(part: {}, line: {}, columns: {})", part, line, columns);
    }

    static Opt<NormTestCase> fromRow(Slice<char> row) {
        if (row.len() == 0 or row[0] == '#' or row[0] == '@')
            return NONE;

        auto noComment = split(row, '#').next();
        if (not noComment)
            return NONE;

        NormTestCase testCase;
        auto cols = split(*noComment, ';');
        for (usize i = 0; i < 5; i++) {
            auto col = cols.next();
            if (not col)
                return NONE;
            auto runes = parseRunes(*col);
            if (not runes)
                return NONE;
            testCase.columns.pushBack(std::move(*runes));
        }
        return Some(std::move(testCase));
    }
};

Yield<NormTestCase> normTestCasesFromFile(Sys::Mmap& file) {
    usize part = 0;
    usize lineNo = 0;
    for (auto line : iterSplit(file.bytes(), '\n')) {
        lineNo++;
        auto row = line.cast<char>();

        // "@PartN # ..."
        if (row.len() > 5 and row[0] == '@') {
            part = row[5] - '0';
            continue;
        }

        auto testCase = NormTestCase::fromRow(row);
        if (not testCase)
            continue;
        testCase->part = part;
        testCase->line = lineNo;
        co_yield *testCase;
    }
}

// UAX #15 conformance: X(c[i]) == c[expected[i]]  (columns c1..c5, 0-indexed)
//   NFC:  c2 == NFC(c1)  == NFC(c2)  == NFC(c3),  c4 == NFC(c4)  == NFC(c5)
//   NFD:  c3 == NFD(c1)  == NFD(c2)  == NFD(c3),  c5 == NFD(c4)  == NFD(c5)
//   NFKC: c4 == NFKC(c1..c5)
//   NFKD: c5 == NFKD(c1..c5)
struct FormSpec {
    NormForm form;
    char const* name;
    usize expected[5];
};

FormSpec const FORMS[] = {
    {NormForm::NFD, "NFD", {2, 2, 2, 4, 4}},
    {NormForm::NFC, "NFC", {1, 1, 1, 3, 3}},
    {NormForm::NFKD, "NFKD", {4, 4, 4, 4, 4}},
    {NormForm::NFKC, "NFKC", {3, 3, 3, 3, 3}},
};

// Quick check contract:
//   YES   => X(s) == s
//   NO    => X(s) != s
//   MAYBE => anything
Res<> checkFormAgainstTestFile(auto& _driver, FormSpec const& spec) {
    auto file = try$(Sys::File::open("bundle://karm-icu.tests/NormalizationTest.txt"_url));
    auto mmap = try$(Sys::mmap(file));

    usize testCount = 0;
    for (auto testCase : normTestCasesFromFile(mmap)) {
        for (usize i = 0; i < 5; i++) {
            auto const& input = testCase.columns[i];
            auto const& output = testCase.columns[spec.expected[i]];
            auto str = encode(input);
            auto qc = spec.form.quickCheck(str);

            if (sameRunes(input, output)) {
                // Already normalized: a NO here is a false negative.
                if (qc == QuickCheck::NO) {
                    logDebug("{} quickCheck said NO for normalized c{} at line {}", spec.name, i + 1, testCase.line);
                    logDebug("Test case: {}", testCase);
                    assertNe$(qc, QuickCheck::NO);
                }
            } else {
                // Not normalized: a YES here is a false positive.
                if (qc == QuickCheck::YES) {
                    logDebug("{} quickCheck said YES for unnormalized c{} at line {}", spec.name, i + 1, testCase.line);
                    logDebug("Test case: {}", testCase);
                    assertNe$(qc, QuickCheck::YES);
                }
            }
        }
        testCount++;
    }

    if (testCount == 0) {
        logWarn("No test cases found in NormalizationTest.txt");
        assertNe$(testCount, usize{0});
    }

    return Ok();
}

} // namespace

test$("norm-quick-check-nfd") {
    return checkFormAgainstTestFile(_driver, FORMS[0]);
}

test$("norm-quick-check-nfc") {
    return checkFormAgainstTestFile(_driver, FORMS[1]);
}

test$("norm-quick-check-nfkd") {
    return checkFormAgainstTestFile(_driver, FORMS[2]);
}

test$("norm-quick-check-nfkc") {
    return checkFormAgainstTestFile(_driver, FORMS[3]);
}

// UAX #15: every code point NOT listed in Part 1 is invariant under all four
// forms, so quickCheck must never return NO for it as a single-rune string.
test$("norm-quick-check-unlisted-codepoints") {
    auto file = try$(Sys::File::open("bundle://karm-icu.tests/NormalizationTest.txt"_url));
    auto mmap = try$(Sys::mmap(file));

    Vec<u64> listed;
    for (usize i = 0; i < 0x110000 / 64; i++)
        listed.pushBack(0);

    usize listedCount = 0;
    for (auto testCase : normTestCasesFromFile(mmap)) {
        if (testCase.part != 1)
            continue;
        Rune r = testCase.columns[0][0];
        listed[r / 64] |= u64{1} << (r % 64);
        listedCount++;
    }

    if (listedCount == 0) {
        logWarn("No Part 1 entries found in NormalizationTest.txt");
        assertNe$(listedCount, usize{0});
    }

    for (Rune r = 0; r <= 0x10FFFF; r++) {
        if (r >= 0xD800 and r <= 0xDFFF)
            continue; // surrogates can't be encoded
        if (listed[r / 64] & (u64{1} << (r % 64)))
            continue;

        Vec<Rune> runes;
        runes.pushBack(r);
        auto str = encode(runes);

        for (auto const& spec : FORMS) {
            auto qc = spec.form.quickCheck(str);
            if (qc == QuickCheck::NO) {
                logDebug("{} quickCheck said NO for unlisted code point {}", spec.name, r);
                assertNe$(qc, QuickCheck::NO);
            }
        }
    }

    return Ok();
}

} // namespace Karm::Icu
