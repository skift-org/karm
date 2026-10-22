#include <karm/test>

import Karm.Sys;
import Karm.Core;
import Karm.Logger;
import Karm.Icu;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Icu {

struct TestCase {
    Vec<Rune> runes;
    usize paragraphLevel;
    Vec<Opt<usize>> levels;
    Vec<usize> ordering;

    void repr(Io::Emit& e) const {
        e("TestCase(runes: {}, paragraphLevel: {}, levels: {}, ordering: {})", runes, paragraphLevel, levels, ordering);
    }

    static Opt<TestCase> fromRow(Slice<char> row) {
        if (row.len() == 0 or row[0] == '#') {
            return NONE; // Skip empty lines and comments
        }

        auto noComment = split(row, '#').next();
        if (not noComment) {
            return NONE;
        }

        auto splitRow = split(*noComment, ';');

        TestCase testCase;

        // runes
        {
            auto textColumn = splitRow.next();
            if (not textColumn)
                return NONE;

            auto strippedColumn = trim(*textColumn, isAsciiBlank);

            for (auto piece : split(strippedColumn, ' ')) {
                Io::SScan scan{piece};
                auto rune = atoi(scan, {.base = 16});
                if (not rune)
                    return NONE;
                testCase.runes.pushBack(*rune);
            }
        }

        // skip
        (void)splitRow.next();

        // paragraph level
        {
            auto levelColumn = splitRow.next();
            if (not levelColumn)
                return NONE;

            auto strippedColumn = trim(*levelColumn, isAsciiBlank);

            Io::SScan scan{strippedColumn};
            auto level = atoi(scan, {.base = 10});
            if (not level)
                return NONE;

            testCase.paragraphLevel = *level;
        }

        // levels
        {
            auto expectedColumn = splitRow.next();
            if (not expectedColumn)
                return NONE;

            auto strippedColumn = trim(*expectedColumn, isAsciiBlank);
            for (auto piece : split(strippedColumn, ' ')) {
                if (piece == "x"s) {
                    testCase.levels.pushBack(NONE);
                } else {
                    Io::SScan scan{piece};
                    auto level = atoi(scan, {.base = 10});
                    if (not level)
                        return NONE;
                    testCase.levels.pushBack(Some(*level));
                }
            }
        }

        // ordering
        {
            auto orderingColumn = splitRow.next();
            if (not orderingColumn)
                return NONE;

            auto strippedColumn = trim(*orderingColumn, isAsciiBlank);
            for (auto piece : split(strippedColumn, ' ')) {
                Io::SScan scan{piece};
                auto id = atoi(scan, {.base = 10});
                if (not id)
                    return NONE;
                testCase.ordering.pushBack(*id);
            }
        }

        return Some(std::move(testCase));
    }
};

Yield<TestCase> testCasesFromFile(Sys::Mmap& file) {
    for (auto line : iterSplit(file.bytes(), '\n')) {
        auto testCase = TestCase::fromRow(line.cast<char>());
        if (not testCase)
            continue;
        co_yield *testCase;
    }
}

test$("bidi-test-file-levels") {
    auto file = try$(Sys::File::open("bundle://karm-icu.tests/BidiCharacterTest.txt"_url));
    auto mmap = try$(Sys::mmap(file));

    usize testCount = 0;
    [[maybe_unused]] usize correct = 0;
    for (auto testCase : testCasesFromFile(mmap)) {
        auto const& inputParagraph = testCase.runes;
        auto [levels, _] = Bidi::computeLevels(inputParagraph, Some(testCase.paragraphLevel));

        assertEq$(levels.len(), testCase.levels.len());
        bool isOk = true;
        for (usize i = 0; i < levels.len(); i++) {
            if (not testCase.levels[i])
                continue;

            if (levels[i] != *testCase.levels[i]) {
                isOk = false;
                logDebug("Test case {} failed at index {}: expected {}, got {}", testCount, i, *testCase.levels[i], levels[i]);
                logDebug("Test case: {}, Output: {}", testCase, levels);
                assertEq$(levels[i], *testCase.levels[i]);
                break;
            }
        }

        if (isOk)
            correct++;
        testCount++;
    }

    if (testCount != 91707) {
        logWarn("Expected 91707 test cases, but found {}", testCount);
        assertEq$(testCount, 91707u);
    }

    return Ok();
}

test$("bidi-test-file-reorder") {
    auto file = try$(Sys::File::open("bundle://karm-icu.tests/BidiCharacterTest.txt"_url));
    auto mmap = try$(Sys::mmap(file));

    usize testCount = 0;
    [[maybe_unused]] usize correct = 0;
    for (auto testCase : testCasesFromFile(mmap)) {
        Vec<Pair<usize, usize>> levels;
        for (usize i = 0; i < testCase.levels.len(); i++) {
            if (testCase.levels[i]) {
                levels.pushBack({i, *testCase.levels[i]});
            }
        }

        Bidi::reorderLine(testCase.runes, testCase.paragraphLevel, mutSub(levels), [&](auto& level) -> usize& {
            return level.v1;
        });

        bool isOk = true;
        for (usize i = 0; i < levels.len(); i++) {
            if (testCase.ordering[i] != levels[i].v0) {
                isOk = false;
                logDebug("Test case {} failed at index {}: expected {}, got {}", testCount, i, testCase.ordering[i], levels[i].v0);
                logDebug("Test case: {}, Output: {}", testCase, levels);
                assertEq$(levels[i].v0, testCase.ordering[i]);
                break;
            }
        }

        if (isOk)
            correct++;
        testCount++;
    }

    if (testCount != 91707) {
        logWarn("Expected 91707 test cases, but found {}", testCount);
        assertEq$(testCount, 91707u);
    }

    return Ok();
}

} // namespace Karm::Icu
