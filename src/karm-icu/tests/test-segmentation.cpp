#include <karm/test>

import Karm.Sys;
import Karm.Core;
import Karm.Logger;
import Karm.Icu;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Icu {

struct SegmentTestCase {
    usize line = 0;
    Vec<Rune> runes;
    Vec<usize> boundaries;

    void repr(Io::Emit& e) const {
        e("SegmentTestCase(line: {}, runes: {}, boundaries: {})", line, runes, boundaries);
    }

    static Opt<SegmentTestCase> fromRow(Slice<char> row) {
        if (row.len() == 0 or row[0] == '#')
            return NONE;

        auto noComment = split(row, '#').next();
        if (not noComment)
            return NONE;

        SegmentTestCase testCase;
        for (auto piece : split(trim(*noComment, isAsciiBlank), ' ')) {
            if (piece.len() == 0 or piece == "×"s)
                continue;

            if (piece == "÷"s) {
                testCase.boundaries.pushBack(testCase.runes.len());
                continue;
            }

            Io::SScan scan{piece};
            auto rune = atoi(scan, {.base = 16});
            if (not rune)
                return NONE;
            testCase.runes.pushBack(static_cast<Rune>(*rune));
        }

        if (testCase.runes.len() == 0)
            return NONE;
        return Some(std::move(testCase));
    }
};

Yield<SegmentTestCase> segmentTestCasesFromFile(Sys::Mmap& file) {
    usize lineNo = 0;
    for (auto line : iterSplit(file.bytes(), '\n')) {
        lineNo++;
        auto testCase = SegmentTestCase::fromRow(line.cast<char>());
        if (not testCase)
            continue;
        testCase->line = lineNo;
        co_yield *testCase;
    }
}

Res<> checkTestFile(auto& _driver, Ref::Url url, usize expectedCount, auto segment) {
    auto file = try$(Sys::File::open(url));
    auto mmap = try$(Sys::mmap(file));

    usize testCount = 0;
    for (auto testCase : segmentTestCasesFromFile(mmap)) {
        Vec<usize> boundaries = segment(testCase.runes);

        bool same = boundaries.len() == testCase.boundaries.len();
        for (usize i = 0; same and i < boundaries.len(); i++)
            same = boundaries[i] == testCase.boundaries[i];

        if (not same) {
            logDebug("Line {} failed: expected {}, got {}", testCase.line, testCase.boundaries, boundaries);
            logDebug("Test case: {}", testCase);
            assert$(same);
        }

        testCount++;
    }

    if (testCount != expectedCount) {
        logWarn("Expected {} test cases, but found {}", expectedCount, testCount);
        assertEq$(testCount, expectedCount);
    }

    return Ok();
}

test$("segmentation-grapheme-test-file") {
    return checkTestFile(_driver, "bundle://karm-icu.tests/GraphemeBreakTest.txt"_url, 766, [](Vec<Rune> const& runes) {
        GraphemeSegmenter segmenter;
        Vec<usize> boundaries;
        for (usize i = 0; i < runes.len(); i++)
            if (segmenter.advance(runes[i]) == GraphemeBoundary::YES)
                boundaries.pushBack(i);
        if (segmenter.advance(TextEdge::END_OF_TEXT) == GraphemeBoundary::YES)
            boundaries.pushBack(runes.len());
        return boundaries;
    });
}

test$("segmentation-word-test-file") {
    return checkTestFile(_driver, "bundle://karm-icu.tests/WordBreakTest.txt"_url, 1944, [](Vec<Rune> const& runes) {
        WordSegmenter segmenter;
        Vec<usize> boundaries;
        usize undecidedAt = 0;

        auto collect = [&](WordBoundaries b, usize at) {
            if (b.undecided == WordBoundary::YES)
                boundaries.pushBack(undecidedAt);
            if (b.current == WordBoundary::YES)
                boundaries.pushBack(at);
            else if (b.current == WordBoundary::UNDECIDED)
                undecidedAt = at;
        };

        for (usize i = 0; i < runes.len(); i++)
            collect(segmenter.advance(runes[i]), i);
        collect(segmenter.advance(TextEdge::END_OF_TEXT), runes.len());
        return boundaries;
    });
}

test$("segmentation-line-test-file") {
    return checkTestFile(_driver, "bundle://karm-icu.tests/LineBreakTest.txt"_url, 19338, [](Vec<Rune> const& runes) {
        LineSegmenter segmenter;
        Vec<usize> boundaries;
        usize undecidedAt = 0;

        auto collect = [&](LineBoundaries b, usize at) {
            if (b.undecided == LineBoundary::ALLOWED)
                boundaries.pushBack(undecidedAt);
            if (oneOf(b.current, LineBoundary::MANDATORY, LineBoundary::ALLOWED))
                boundaries.pushBack(at);
            else if (b.current == LineBoundary::UNDECIDED)
                undecidedAt = at;
        };

        for (usize i = 0; i < runes.len(); i++)
            collect(segmenter.advance(runes[i]), i);
        collect(segmenter.advance(TextEdge::END_OF_TEXT), runes.len());
        return boundaries;
    });
}

test$("segmentation-line-css") {
    auto check = [](Str text, LineTailoring tailoring, Slice<usize> expected) {
        LineSegmenter segmenter{tailoring};
        Vec<usize> boundaries;
        usize undecidedAt = 0;
        usize at = 0;

        auto collect = [&](LineBoundaries b) {
            if (b.undecided == LineBoundary::ALLOWED)
                boundaries.pushBack(undecidedAt);
            if (oneOf(b.current, LineBoundary::MANDATORY, LineBoundary::ALLOWED))
                boundaries.pushBack(at);
            else if (b.current == LineBoundary::UNDECIDED)
                undecidedAt = at;
        };

        for (auto r : iterRunes(text)) {
            collect(segmenter.advance(r));
            at++;
        }
        collect(segmenter.advance(TextEdge::END_OF_TEXT));

        if (boundaries.len() != expected.len())
            return false;
        for (usize i = 0; i < boundaries.len(); i++)
            if (boundaries[i] != expected[i])
                return false;
        return true;
    };

    LineTailoring normal = {};
    LineTailoring loose = {.strictness = LineStrictness::LOOSE};
    LineTailoring looseCjk = {.strictness = LineStrictness::LOOSE, .cjk = true};
    LineTailoring normalCjk = {.cjk = true};

    assert$(check("アァ"s, normal, Array{2uz}));
    assert$(check("アァ"s, loose, Array{1uz, 2uz}));

    assert$(check("あ〜"s, normal, Array{2uz}));
    assert$(check("あ〜"s, normalCjk, Array{1uz, 2uz}));

    assert$(check("人々"s, normal, Array{2uz}));
    assert$(check("人々"s, loose, Array{1uz, 2uz}));

    assert$(check("日‐"s, normal, Array{2uz}));
    assert$(check("日‐"s, loose, Array{1uz, 2uz}));

    assert$(check("……"s, normal, Array{2uz}));
    assert$(check("……"s, loose, Array{1uz, 2uz}));

    assert$(check("日！"s, loose, Array{2uz}));
    assert$(check("日！"s, looseCjk, Array{1uz, 2uz}));

    assert$(check("日％"s, loose, Array{2uz}));
    assert$(check("日％"s, looseCjk, Array{1uz, 2uz}));

    LineTailoring breakAll = {.words = WordBreaking::BREAK_ALL};
    assert$(check("ab"s, normal, Array{2uz}));
    assert$(check("ab"s, breakAll, Array{1uz, 2uz}));
    assert$(check("12"s, breakAll, Array{1uz, 2uz}));

    LineTailoring keepAll = {.words = WordBreaking::KEEP_ALL};
    assert$(check("日本"s, normal, Array{1uz, 2uz}));
    assert$(check("日本"s, keepAll, Array{2uz}));
    assert$(check("각줄"s, keepAll, Array{2uz}));
    assert$(check("日 本"s, keepAll, Array{2uz, 3uz}));

    LineTailoring noHyphens = {.hyphens = false};
    assert$(check("a­b"s, normal, Array{2uz, 3uz}));
    assert$(check("a­b"s, noHyphens, Array{3uz}));

    LineTailoring anywhere = {.strictness = LineStrictness::ANYWHERE};
    assert$(check("a b"s, normal, Array{2uz, 3uz}));
    assert$(check("a b"s, anywhere, Array{1uz, 2uz, 3uz}));
    assert$(check("éx"s, anywhere, Array{2uz, 3uz}));
    assert$(check("a\nb"s, anywhere, Array{1uz, 2uz, 3uz}));

    return Ok();
}

test$("segmentation-count-grapheme-clusters") {
    assertEq$(countGrapheneClusters(""s), 0uz);
    assertEq$(countGrapheneClusters("abc"s), 3uz);
    assertEq$(countGrapheneClusters("é"s), 1uz);
    assertEq$(countGrapheneClusters("\r\n"s), 1uz);
    assertEq$(countGrapheneClusters("🇫🇷🇧🇪"s), 2uz);
    assertEq$(countGrapheneClusters("👨‍👩‍👧"s), 1uz);
    return Ok();
}

test$("segmentation-word-like") {
    assert$(wordLike("hello"s));
    assert$(wordLike("42"s));
    assert$(wordLike("日"s));
    assertNot$(wordLike(""s));
    assertNot$(wordLike(" "s));
    assertNot$(wordLike("!?"s));
    assertNot$(wordLike("👍"s));
    return Ok();
}

test$("segmentation-count-words") {
    assertEq$(countWords(""s), 0uz);
    assertEq$(countWords("   "s), 0uz);
    assertEq$(countWords("Hello, world!"s), 2uz);
    assertEq$(countWords("can't stop"s), 2uz);
    assertEq$(countWords("e.g. example.com"s), 2uz);
    assertEq$(countWords("3.14 apples"s), 2uz);
    assertEq$(countWords("a:1"s), 2uz);
    assertEq$(countWords("a:"s), 1uz);
    assertEq$(countWords("snake_case 42"s), 2uz);
    assertEq$(countWords("👍 ok"s), 1uz);
    return Ok();
}

} // namespace Karm::Icu
