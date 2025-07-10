#include <karm-io/text.h>
#include <karm-sys/file.h>
#include <karm-test/macros.h>

import Karm.Icu;

namespace Karm::Icu {

Slice<char> strip(Slice<char> row) {
    usize l = 0;
    usize r = row.len();
    while (l < r and (row[l] == ' ' or row[l] == '\t'))
        l++;
    while (r > l and (row[r - 1] == ' ' or row[r - 1] == '\t'))
        r--;
    return sub(row, l, r);
}

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

            auto strippedColumn = strip(*textColumn);

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

            auto strippedColumn = strip(*levelColumn);

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

            auto strippedColumn = strip(*expectedColumn);
            for (auto piece : split(strippedColumn, ' ')) {
                if (piece == "x"s) {
                    testCase.levels.pushBack(NONE);
                } else {
                    Io::SScan scan{piece};
                    auto level = atoi(scan, {.base = 10});
                    if (not level)
                        return NONE;
                    testCase.levels.pushBack(*level);
                }
            }
        }

        // ordering
        {
            auto orderingColumn = splitRow.next();
            if (not orderingColumn)
                return NONE;

            auto strippedColumn = strip(*orderingColumn);
            for (auto piece : split(strippedColumn, ' ')) {
                Io::SScan scan{piece};
                auto id = atoi(scan, {.base = 10});
                if (not id)
                    return NONE;
                testCase.ordering.pushBack(*id);
            }
        }

        return testCase;
    }
};

Generator<TestCase> testCasesFromFile(Sys::FileReader& file) {
    while (true) {
        Io::BufferWriter bw;
        auto maybeLine = Io::readLine(file, bw, "\n"_bytes);
        if (not maybeLine)
            continue;

        auto [read, reachedDelim] = maybeLine.take();

        if (read == 0) {
            if (reachedDelim)
                continue;
            else
                co_return;
        }

        auto testCase = TestCase::fromRow(bw.bytes().cast<char>());
        if (not testCase)
            continue;

        co_yield *testCase;
    }
}

test$("bidiTestFileLevels") {
    auto file = try$(Sys::File::open("bundle://karm-icu.tests/BidiCharacterTest.txt"_url));
    usize testCount = 0;
    usize correct = 0;
    for (auto testCase : testCasesFromFile(file)) {
        auto const& inputParagraph = testCase.runes;
        auto [levels, _] = Bidi::computeLevels(inputParagraph, testCase.paragraphLevel);

        expectEq$(levels.len(), testCase.levels.len());
        bool isOk = true;
        for (usize i = 0; i < levels.len(); i++) {
            if (not testCase.levels[i])
                continue;

            if (levels[i] != *testCase.levels[i]) {
                isOk = false;
                logDebug("Test case {} failed at index {}: expected {}, got {}", testCount, i, *testCase.levels[i], levels[i]);
                logDebug("Test case: {}, Output: {}", testCase, levels);
                expectEq$(levels[i], *testCase.levels[i]);
                break;
            }
        }

        if (isOk)
            correct++;
        testCount++;
    }

    if(testCount != 91707) {
        logWarn("Expected 91707 test cases, but found {}", testCount);
        expectEq$(testCount, 91707u);
    }

    logDebug("Passed {} tests from {} for Bidi level computation", correct, testCount);

    return Ok();
}

test$("bidiTestFileReorder") {
    auto file = try$(Sys::File::open("bundle://karm-icu.tests/BidiCharacterTest.txt"_url));
    usize testCount = 0;
    usize correct = 0;
    for (auto testCase : testCasesFromFile(file)) {
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
                expectEq$(levels[i].v0, testCase.ordering[i]);
                break;
            }
        }

        if (isOk)
            correct++;
        testCount++;
    }

    if(testCount != 91707) {
        logWarn("Expected 91707 test cases, but found {}", testCount);
        expectEq$(testCount, 91707u);
    }

    logDebug("Passed {} tests from {} for Bidi line reordering", correct, testCount);

    return Ok();
}

} // namespace Karm::Icu
