module;

#include <karm-base/string.h>
#include <karm-base/vec.h>
#include <karm-logger/logger.h>

export module Karm.Icu:bidi;

import :base;
import :ucd;

static constexpr bool DEBUG_BIDI = false;

namespace Karm::Icu {

// Implementation of unicode bidi algorithm
// https://unicode.org/reports/tr9/
// https://www.unicode.org/Public/PROGRAMS/BidiReferenceJava/BidiReference.java

bool isIsolateInitiator(BidiType type) {
    return type == BidiType::LRI or type == BidiType::RLI or type == BidiType::FSI;
}

// https://unicode.org/reports/tr9/#NI
bool isNI(BidiType type) {
    return isIsolateInitiator(type) or type == BidiType::PDI or type == BidiType::B or type == BidiType::S or type == BidiType::WS or type == BidiType::ON;
}

Rune getPairedBracket(Rune r) {
    switch (r) {
#define BRACKET(A, B, _) \
    case A:              \
        return B;
#include "defs/unicode-brackets.inc"
#undef BRACKET
    }

    // FIXME
    panic("");
}

bool bracketsMatch(Rune a, Rune b) {
    if (getPairedBracket(a) == b)
        return true;

    // Note that although bracket pairs are defined under canonical equivalence, canonical equivalents only exist
    // between U+3008/U+3009, and U+2329/U+232A, and the Unicode Consortium will not add more such pairs.
    if (a == 0x232A)
        a = 0x3009;
    if (b == 0x2329)
        b = 0x3008;

    return getPairedBracket(a) == b;
}

enum struct BidiPairedBracketType {
    NONE = 0, //< No paired bracket
    OPEN = 1, //< Opening paired bracket
    CLOSE = 2 //< Closing paired bracket
};

BidiPairedBracketType getBidiPairedBracketType(Rune r) {
    switch (r) {
#define BRACKET(A, _, C) \
    case A:              \
        return C == 'o' ? BidiPairedBracketType::OPEN : BidiPairedBracketType::CLOSE;
#include "defs/unicode-brackets.inc"
#undef BRACKET
    }
    return BidiPairedBracketType::NONE;
}

namespace Bidi {
// MARK: Resolving Embedding Levels
// https://unicode.org/reports/tr9/#Resolving_Embedding_Levels

// 3.3 Resolving Embedding Levels - https://unicode.org/reports/tr9/#Resolving_Embedding_Levels
// Applying rules P2 and P3 to determine the paragraph level.
usize determineParagraphLevel(Slice<BidiType> paragraph) {
    // P2. In each paragraph, find the first character of type L, AL, or R while skipping over any characters
    // between an isolate initiator and its matching PDI or, if it has no matching PDI, the end of the paragraph.

    // P3. If a character is found in P2 and it is of type AL or R, then set the paragraph embedding level to one;
    // otherwise, set it to zero.
    usize isolateCount = 0;
    for (auto const& c : paragraph) {
        if (isIsolateInitiator(c)) {
            isolateCount++;
        } else if (c == BidiType::PDI) {
            isolateCount--;
        } else if (isolateCount > 0) {
            continue;
        } else if (c == BidiType::AL or c == BidiType::R) {
            return 1;
        } else if (c == BidiType::L) {
            return 0;
        }
    }

    return 0;
}

// https://unicode.org/reports/tr9/#BD6
// BD6. The directional override status determines whether the bidirectional type of characters is to
// be reset. The directional override status is set by using explicit directional formatting characters.
// This status has three states, as shown in Table 2.
enum struct DirectionOverrideStatus : u8 {
    NEUTRAL = 0, //< Neutral
    LTR = 1,     //< Left-to-Right
    RTL = 2      //< Right-to-Left
};

struct DirectionalStatusStackEntry {
    usize embeddingLevel;
    DirectionOverrideStatus directionOverrideStatus;

    // https://unicode.org/reports/tr9/#BD12
    // BD12. The directional isolate status is a Boolean value set by using isolate formatting characters:
    // it is true when the current embedding level was started by an isolate initiator.
    bool directionIsolateStatus;
};

using DirectionalStatusStack = Vec<DirectionalStatusStackEntry>;

// https://unicode.org/reports/tr9/#Explicit_Levels_and_Directions
// https://unicode.org/reports/tr9/#Retaining_Explicit_Formatting_Characters
Vec<usize> explicitLevelsAndDirections(
    MutSlice<BidiType> paragraph, usize paragraphLevel, Vec<Opt<usize>> const& matchingPDIIndices
) {
    Vec<usize> levels{Buf<usize>::init(paragraph.len(), 0)};

    static int const MAX_DEPTH = 125;
    // X1. At the beginning of a paragraph, perform the following steps:
    // Set the stack to empty.
    DirectionalStatusStack directionalStatusStack;

    // Push onto the stack an entry consisting of the paragraph embedding level, a neutral directional override
    // status, and a false directional isolate status.
    directionalStatusStack.pushBack(
        {.embeddingLevel = paragraphLevel,
         .directionOverrideStatus = DirectionOverrideStatus::NEUTRAL,
         .directionIsolateStatus = false}
    );

    // Set the overflow isolate count to zero.
    usize overflowIsolateCount = 0;

    // Set the overflow embedding count to zero.
    usize overflowEmbeddingCount = 0;

    // Set the valid isolate count to zero.
    usize validIsolateCount = 0;

    auto leastOddGreaterThan = [](usize level) {
        if (level % 2 == 0) {
            return level + 1;
        }
        return level + 2;
    };

    auto leastEvenGreaterThan = [](usize level) {
        if (level % 2 == 1) {
            return level + 1;
        }
        return level + 2;
    };

    auto incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero = [&]() {
        // If the overflow isolate count is zero, increment the overflow embedding count by one.
        // Leave all other variables unchanged.
        if (overflowIsolateCount == 0) {
            overflowEmbeddingCount++;
        }
    };

    auto checkIfValidNewLevelAndCounts = [&](usize newEmbeddingLevel) {
        // If this new level would be valid, and the overflow isolate count and overflow embedding count
        // are both zero, then (...) is valid
        return newEmbeddingLevel <= MAX_DEPTH &&
               overflowIsolateCount == 0 &&
               overflowEmbeddingCount == 0;
    };

    auto overrideCharIfNotNeutral = [&](BidiType c) {
        // Whenever the directional override status of the last entry on the directional status stack is not
        // neutral, reset the current character type according to the directional override status of the last
        // entry on the directional status stack.
        if (last(directionalStatusStack).directionOverrideStatus == DirectionOverrideStatus::LTR) {
            return BidiType::L;
        } else if (last(directionalStatusStack).directionOverrideStatus == DirectionOverrideStatus::RTL) {
            return BidiType::R;
        }
        return c;
    };

    // In rules X2 through X5, insert an initial step setting the explicit embedding or
    // override character's embedding level to the embedding level of the last entry on the
    // directional status stack. This applies to RLE, LRE, RLO, and LRO.
    usize matchingPDIIndicesIndex = 0;
    for (usize i = 0; i < paragraph.len(); i++) {
        auto& c = paragraph[i];
        if (c == BidiType::RLE) {
            // X2. With each RLE, perform the following steps:
            levels[i] = last(directionalStatusStack).embeddingLevel;

            // Compute the least odd embedding level greater than the embedding level of the last entry on the
            // directional status stack.
            usize newEmbeddingLevel = leastOddGreaterThan(last(directionalStatusStack).embeddingLevel);

            if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                // ..., then this RLE is valid.
                // Push an entry consisting of the new embedding level, neutral directional override status, and
                // false directional isolate status onto the directional status stack.
                directionalStatusStack.pushBack({newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, false});
            } else {
                incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
            }

        } else if (c == BidiType::LRE) {
            // X3. With each LRE, perform the following steps:
            levels[i] = last(directionalStatusStack).embeddingLevel;

            // Compute the least even embedding level greater than the embedding level of the last entry on
            // the directional status stack.
            usize newEmbeddingLevel = leastEvenGreaterThan(last(directionalStatusStack).embeddingLevel);

            if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                // ..., then this LRE is valid.
                // Push an entry consisting of the new embedding level, neutral directional override status,
                // and false directional isolate status onto the directional status stack.
                directionalStatusStack.pushBack({newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, false});
            } else {
                incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
            }

        } else if (c == BidiType::RLO) {
            // X4. With each RLO, perform the following steps:
            levels[i] = last(directionalStatusStack).embeddingLevel;

            // Compute the least odd embedding level greater than the embedding level of the last entry
            // on the directional status stack.
            usize newEmbeddingLevel = leastOddGreaterThan(last(directionalStatusStack).embeddingLevel);

            if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                // , then this RLO is valid.
                // Push an entry consisting of the new embedding level, right-to-left directional override
                // status, and false directional isolate status onto the directional status stack.
                directionalStatusStack.pushBack({newEmbeddingLevel, DirectionOverrideStatus::RTL, false});
            } else {
                incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
            }
        } else if (c == BidiType::LRO) {
            // X5. With each LRO, perform the following steps:
            levels[i] = last(directionalStatusStack).embeddingLevel;

            // Compute the least even embedding level greater than the embedding level of the last entry
            // on the directional status stack.
            usize newEmbeddingLevel = leastEvenGreaterThan(last(directionalStatusStack).embeddingLevel);

            if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                // , then this LRO is valid.
                // Push an entry consisting of the new embedding level, left-to-right directional override
                // status, and false directional isolate status onto the directional status stack.
                directionalStatusStack.pushBack({newEmbeddingLevel, DirectionOverrideStatus::LTR, false});
            } else {
                incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
            }
        } else if (isIsolateInitiator(c)) {
            if (c == BidiType::FSI) {
                // apply rules P2 and P3 to the sequence of characters between the FSI and its matching PDI
                // or if there is no matching PDI, the end of the paragraph
                // as if this sequence of characters were a paragraph

                auto isolateParagraphLevel = determineParagraphLevel(
                    sub(paragraph, i + 1, matchingPDIIndices[matchingPDIIndicesIndex].unwrapOr(paragraph.len()))
                );

                // If these rules decide on paragraph embedding level 1, treat the FSI as an RLI in rule X5a.
                // Otherwise, treat it as an LRI in rule X5b.
                c = isolateParagraphLevel == 1 ? BidiType::RLI : BidiType::LRI;
            }

            if (c == BidiType::RLI) {
                // X5a. With each RLI, perform the following steps:

                // Set the RLI's embedding level to the embedding level of the last entry on the directional status stack.
                levels[i] = last(directionalStatusStack).embeddingLevel;

                c = overrideCharIfNotNeutral(c);

                // Compute the least odd embedding level greater than the embedding level of the last entry on
                // the directional status stack.
                usize newEmbeddingLevel = leastOddGreaterThan(last(directionalStatusStack).embeddingLevel);

                // If this new level would be valid and the overflow isolate count and the overflow embedding count
                // are both zero,
                if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                    // then this RLI is valid.
                    // Increment the valid isolate count by one,
                    validIsolateCount++;
                    // and push an entry consisting of the new embedding level, neutral directional override status,
                    // and true directional isolate status onto the directional status stack.
                    directionalStatusStack.pushBack({newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, true});
                } else {
                    // Otherwise, this is an overflow RLI. Increment the overflow isolate count by one,
                    // and leave all other variables unchanged.
                    overflowIsolateCount++;
                }

            } else {
                // X5b. With each LRI, perform the following steps:
                // Set the LRI's embedding level to the embedding level of the last entry on
                // the directional status stack.
                levels[i] = last(directionalStatusStack).embeddingLevel;

                c = overrideCharIfNotNeutral(c);

                // Compute the least even embedding level greater than the embedding level of the last entry on the
                // directional status stack.
                usize newEmbeddingLevel = leastEvenGreaterThan(last(directionalStatusStack).embeddingLevel);

                // If this new level would be valid and the overflow isolate count and the overflow embedding count
                // are both zero,
                if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                    // then this LRI is valid.
                    // Increment the valid isolate count by one,
                    validIsolateCount++;
                    // and push an entry consisting of the new embedding level, neutral directional override status,
                    //  and true directional isolate status onto the directional status stack.
                    directionalStatusStack.pushBack({newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, true});
                } else {
                    // Otherwise, this is an overflow LRI. Increment the overflow isolate count by one, and leave
                    // all other variables unchanged.
                    overflowIsolateCount++;
                }
            }
        } else if (c != BidiType::B and c != BidiType::PDF and c != BidiType::PDI) {
            // X6. For all types besides B, BN, RLE, LRE, RLO, LRO, PDF, RLI, LRI, FSI, and PDI:
            // In rule X6, remove the exclusion of BN characters for the purposes of setting embedding levels.

            // Set the current character’s embedding level to the embedding level of the last entry on
            // the directional status stack.
            levels[i] = last(directionalStatusStack).embeddingLevel;

            // Continue not updating the character types of these characters.
            if (c != BidiType::BN)
                c = overrideCharIfNotNeutral(c);
        } else if (c == BidiType::PDI) {
            // If the overflow isolate count is greater than zero,
            if (overflowIsolateCount > 0) {
                // this PDI matches an overflow isolate initiator.
                // Decrement the overflow isolate count by one.
                overflowIsolateCount--;
            }
            if (validIsolateCount == 0) {
                // Otherwise, if the valid isolate count is zero, this PDI does not match any isolate initiator,
                // valid or overflow.
                // Do nothing.
            } else {
                // Otherwise, this PDI matches a valid isolate initiator. Perform the following steps:
                // Reset the overflow embedding count to zero.
                overflowEmbeddingCount = 0;

                // While the directional isolate status of the last entry on the stack is false,
                // pop the last entry from the directional status stack.
                while (not last(directionalStatusStack).directionIsolateStatus)
                    directionalStatusStack.popBack();

                // Pop the last entry from the directional status stack and decrement the valid isolate count by one.
                directionalStatusStack.popBack();
                validIsolateCount--;
            }

            // Set the PDI’s level to the entry's embedding level.
            levels[i] = last(directionalStatusStack).embeddingLevel;
            c = overrideCharIfNotNeutral(c);
        } else if (c == BidiType::PDF) {
            // If the overflow isolate count is greater than zero,
            if (overflowIsolateCount > 0) {
                // do nothing.
            } else if (overflowEmbeddingCount > 0) {
                // Otherwise, if the overflow embedding count is greater than zero, decrement it by one.
                overflowEmbeddingCount--;
            } else if (not last(directionalStatusStack).directionIsolateStatus and directionalStatusStack.len() >= 2) {
                // Otherwise, if the directional isolate status of the last entry on the directional status stack
                // is false, and the directional status stack contains at least two entries,
                // pop the last entry from the directional status stack.
                directionalStatusStack.popBack();
            } else {
                // Otherwise, do nothing.
            }

            // In rule X7, add a final step setting the embedding level of the PDF to the embedding level of the
            // last entry on the directional status stack, in all cases.
            levels[i] = last(directionalStatusStack).embeddingLevel;
        } else if (c == BidiType::B) {
            // Explicit paragraph separators (bidirectional character type B) indicate the end of a paragraph.
            // As such, they are not included in any embedding, override or isolate.
            // They are simply assigned the paragraph embedding level.
            levels[i] = paragraphLevel;
        }

        if (isIsolateInitiator(c))
            matchingPDIIndicesIndex++;
    }
    return levels;
}

inline bool removeDueToX9(BidiType c) {
    // X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.
    return c == BidiType::BN or c == BidiType::LRE or c == BidiType::RLO or
           c == BidiType::LRO or c == BidiType::PDF or c == BidiType::RLE;
}

// https://unicode.org/reports/tr9/#BD13
struct IsolatingRunSequence {
    Vec<urange> levelRuns;
    BidiType sos = BidiType::L; //< Start-of-sequence type (L or R)
    BidiType eos = BidiType::L; //< End-of-sequence type (L or R)

    void repr(Io::Emit& e) const {
        e("IsolatingRunSequence (sos {} eos {} ", sos, eos);
        e("levelRuns {}", levelRuns);
        e(")");
    }
};

// https://unicode.org/reports/tr9/#BD13
Vec<IsolatingRunSequence> computeIsolatingRunSequence(
    MutSlice<BidiType> paragraph, Vec<usize>& levels, Vec<Opt<usize>> const& matchingPDIIndices
) {
    Vec<IsolatingRunSequence> isolatingRunSequences;

    // this is a stack keeping next matching PDI index and related isolating sequence index
    // every time we pass by a valid/matched isolate initiator, we push to this stack
    // every time we pass by a matching PDI, we pop from this stack (this matching PDI should be the top)
    Vec<Pair<usize>> nextMatchingPDIIndices;

    usize matchingPDIIndicesIndex = 0;
    for (usize l = 0; l < paragraph.len();) {
        // Finding level run in interval [l; r)
        usize r = l + 1;
        while (r < paragraph.len() and (levels[r] == levels[l] or removeDueToX9(paragraph[r]))) {
            r++;
        }

        // updating matchingPDIIndicesIndex to point to the latest isolate initiator not including the last one in
        // the level run
        // the isolate initiators in this range should be ones without match
        for (usize i = l; i < r - 1; i++) {
            matchingPDIIndicesIndex += isIsolateInitiator(paragraph[i]) ? 1 : 0;
        }

        logDebugIf(DEBUG_BIDI, "Level run at ({}, {}) with level {}", l, r, levels[l]);
        // Finding if this level run belongs to an isolating sequence, that should be pointed by isolatingSequenceIndex
        usize isolatingSequenceIndex;
        if (paragraph[l] == BidiType::PDI and last(nextMatchingPDIIndices).v0 == l) {
            isolatingSequenceIndex = last(nextMatchingPDIIndices).v1;
            isolatingRunSequences[isolatingSequenceIndex].levelRuns.pushBack(urange::fromStartEnd(l, r));
            nextMatchingPDIIndices.popBack();
            // we should be continuing a sequence run from before if PDI is matching
        } else {
            isolatingSequenceIndex = isolatingRunSequences.len();
            isolatingRunSequences.pushBack({
                .levelRuns = {urange::fromStartEnd(l, r)},
            });
        }

        if (isIsolateInitiator(paragraph[r - 1]) and matchingPDIIndices[matchingPDIIndicesIndex]) {
            nextMatchingPDIIndices.pushBack({*matchingPDIIndices[matchingPDIIndicesIndex], isolatingSequenceIndex});

            // updating the matchingPDIIndicesIndex to pass the matching isolate initiator from this level run
            matchingPDIIndicesIndex++;
        }
        l = r;
    }
    return isolatingRunSequences;
}

struct BracketPairStackEntry {
    bool bracketChar;
    usize textPosition;

    void repr(Io::Emit& e) const {
        e("BracketPairStackEntry (bracketChar {}, textPosition {})", bracketChar, textPosition);
    }
};

using BracketPairStack = Vec<BracketPairStackEntry>;

// https://unicode.org/reports/tr9/#BD16
Vec<Pair<usize>> findBracketPairs(
    Slice<Rune> input, MutSlice<BidiType> paragraph, IsolatingRunSequence const& isolatingRunSequence
) {
    static usize const MAX_DEPTH = 63;
    Vec<Pair<usize>> bracketPairs;
    BracketPairStack bracketPairStack;

    // BD14. An opening paired bracket is a character whose Bidi_Paired_Bracket_Type property value is Open and
    // whose current bidirectional character type is ON.
    auto isOpeningPairedBracket = [&](usize i) {
        return getBidiPairedBracketType(input[i]) == BidiPairedBracketType::OPEN and
               paragraph[i] == BidiType::ON;
    };

    // BD15. A closing paired bracket is a character whose Bidi_Paired_Bracket_Type property value is Close and
    // whose current bidirectional character type is ON.
    auto isClosingPairedBracket = [&](usize i) {
        return getBidiPairedBracketType(input[i]) == BidiPairedBracketType::CLOSE and
               paragraph[i] == BidiType::ON;
    };

    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            // If an opening paired bracket is found and there is room in the stack,
            // push its Bidi_Paired_Bracket property value and its text position onto the stack.
            if (isOpeningPairedBracket(i)) {
                if (bracketPairStack.len() < MAX_DEPTH) {
                    bracketPairStack.pushBack({.bracketChar = true, .textPosition = i});
                } else {
                    // If an opening paired bracket is found and there is no room in the stack,
                    // stop processing BD16 for the remainder of the isolating run sequence and return an empty list.
                    return {};
                }
            } else if (isClosingPairedBracket(i)) {
                // If a closing paired bracket is found, do the following:
                // 1. Declare a variable that holds a reference to the current stack element and initialize it with
                // the top element of the stack. If the stack is empty, skip to step 5.
                if (bracketPairStack.len() == 0)
                    continue;

                usize currentStackIndex = bracketPairStack.len() - 1;
                while (currentStackIndex != usize(-1)) {
                    // 2. Compare the closing paired bracket being inspected to the bracket in
                    // the current stack element, where U+3009 and U+232A are treated as equivalent.
                    if (bracketsMatch(input[i], input[bracketPairStack[currentStackIndex].textPosition])) {
                        // 3. If the values match, meaning the two characters form a bracket pair, then

                        // Append the text position in the current stack element together with the text position of the
                        // closing paired bracket to the list of resulting bracket pairs.
                        bracketPairs.pushBack({bracketPairStack[currentStackIndex].textPosition, i});

                        // Pop the stack through the current stack element inclusively.
                        while (bracketPairStack.len() >= currentStackIndex + 1) {
                            bracketPairStack.popBack();
                        }
                        break;
                    } else {
                        // 4. Else, if the current stack element is not at the bottom of the stack, advance it to
                        // the next element deeper in the stack and go back to step 2.
                        currentStackIndex--;
                    }
                }
                // 5. Else, continue with inspecting the next character without popping the stack.
            }
        }
    }

    sort(bracketPairs);
    return bracketPairs;
}

void changeTypeGivenAdjacentAndTargetPreds(
    MutSlice<BidiType> paragraph, IsolatingRunSequence const& isolatingRunSequence,
    auto predForTarget, auto predForAdjacent, BidiType newType
) {
    // If both the adjacent character and the target character satisfy the given predicates,
    // change the type of the target character to the new type.
    Opt<usize> prevValidIndex;

    auto transformAdjacent = [&](usize i) {
        if (not prevValidIndex) {
            prevValidIndex = i;
            return;
        }

        if (predForAdjacent(paragraph, *prevValidIndex) and predForTarget(paragraph, i)) {
            paragraph[i] = newType;
        }

        prevValidIndex = i;
    };

    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            transformAdjacent(i);
        }
    }

    prevValidIndex = NONE;
    for (usize j = isolatingRunSequence.levelRuns.len() - 1; j != usize(-1); j--) {
        auto const& levelRun = isolatingRunSequence.levelRuns[j];
        for (usize i = usize(levelRun.end() - 1); i != usize(levelRun.start - 1); i--) {
            transformAdjacent(i);
        }
    }
}

// https://unicode.org/reports/tr9/#Resolving_Weak_Types
void resolveWeakTypes(MutSlice<BidiType> paragraph, IsolatingRunSequence const& isolatingRunSequence) {
    // W1. Examine each nonspacing mark (NSM) in the isolating run sequence, ...
    // could be an OPT
    Opt<usize> prevValidIndex = NONE;
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (paragraph[i] != BidiType::NSM) {
                prevValidIndex = i;
                continue;
            }

            if (not prevValidIndex) {
                // If the NSM is at the start of the isolating run sequence, it will get the type of sos.
                paragraph[i] = isolatingRunSequence.sos;
            } else if (isIsolateInitiator(paragraph[*prevValidIndex]) or paragraph[*prevValidIndex] == BidiType::PDI) {
                // (...) change the type of the NSM to Other Neutral if the previous character is
                // an isolate initiator or PDI
                paragraph[i] = BidiType::ON;
            } else {
                // (...) to the type of the previous character otherwise
                paragraph[i] = paragraph[*prevValidIndex];
            }

            prevValidIndex = i;
        }
    }

    // W2. Search backward from each instance of a European number until
    // the first strong type (R, L, AL, or sos) is found.
    // If an AL is found, change the type of the European number to Arabic number.
    Opt<usize> lastIndexOfInterest = NONE;
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (paragraph[i] == BidiType::EN) {
                if (lastIndexOfInterest and paragraph[*lastIndexOfInterest] == BidiType::AL)
                    paragraph[i] = BidiType::AN;
            } else if (
                paragraph[i] == BidiType::R or paragraph[i] == BidiType::L or paragraph[i] == BidiType::AL
            ) {
                lastIndexOfInterest = i;
            }
        }
    }

    // W3. Change all ALs to R.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (paragraph[i] == BidiType::AL) {
                paragraph[i] = BidiType::R;
            }
        }
    }

    // W4. A single European separator between two European numbers changes to a European number.
    // A single common separator between two numbers of the same type changes to that type.
    prevValidIndex = NONE;
    Opt<usize> prevPrevValidIndex = NONE;
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (not prevPrevValidIndex) {
                prevPrevValidIndex = prevValidIndex;
                prevValidIndex = i;
                continue;
            }

            if (
                paragraph[*prevPrevValidIndex] == BidiType::EN and
                (paragraph[*prevValidIndex] == BidiType::ES or paragraph[*prevValidIndex] == BidiType::CS) and
                paragraph[i] == BidiType::EN
            ) {
                paragraph[*prevValidIndex] = BidiType::EN;
            } else if (
                paragraph[*prevPrevValidIndex] == BidiType::AN and
                paragraph[*prevValidIndex] == BidiType::CS and
                paragraph[i] == BidiType::AN
            ) {
                paragraph[*prevValidIndex] = BidiType::AN;
            }

            prevPrevValidIndex = prevValidIndex;
            prevValidIndex = i;
        }
    }

    // W5. A sequence of European terminators adjacent to European numbers changes to all European numbers.
    changeTypeGivenAdjacentAndTargetPreds(paragraph, isolatingRunSequence, [](MutSlice<BidiType> paragraph, usize i) {
        return paragraph[i] == BidiType::EN;
    },
                                          [](MutSlice<BidiType> paragraph, usize i) {
                                              return paragraph[i] == BidiType::ET or paragraph[i] == BidiType::BN;
                                          },
                                          BidiType::EN);

    // W6. All remaining separators and terminators (after the application of W4 and W5) change to Other Neutral.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (paragraph[i] == BidiType::ES or
                paragraph[i] == BidiType::ET or
                paragraph[i] == BidiType::CS) {
                paragraph[i] = BidiType::ON;
            }
        }
    }

    // In rule W6, change all BN types adjacent to ET, ES, or CS to ON as well.
    changeTypeGivenAdjacentAndTargetPreds(paragraph, isolatingRunSequence, [](MutSlice<BidiType> paragraph, usize i) {
        return paragraph[i] == BidiType::ES or
               paragraph[i] == BidiType::ET or
               paragraph[i] == BidiType::CS;
    },
                                          [](MutSlice<BidiType> paragraph, usize i) {
                                              return paragraph[i] == BidiType::BN;
                                          },
                                          BidiType::ON);

    // W7. Search backward from each instance of a European number until the first strong type (R, L, or sos) is
    // found. If an L is found, then change the type of the European number to L.
    lastIndexOfInterest = NONE;
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (paragraph[i] == BidiType::EN) {
                if (not lastIndexOfInterest) {
                    if (isolatingRunSequence.sos == BidiType::L)
                        paragraph[i] = BidiType::L;
                } else if (paragraph[*lastIndexOfInterest] == BidiType::L)
                    paragraph[i] = BidiType::L;
            } else if (
                paragraph[i] == BidiType::R or paragraph[i] == BidiType::L
            ) {
                lastIndexOfInterest = i;
            }
        }
    }
}

BidiType applyNeutralTypesScopeChanges(BidiType type) {
    // Within this scope, bidirectional types EN and AN are treated as R.
    if (type == BidiType::R or type == BidiType::AN or type == BidiType::EN)
        return BidiType::R;
    return type;
}

// https://unicode.org/reports/tr9/#N0
void runN0ForBracketPair(
    Slice<Rune> input, MutSlice<BidiType> paragraph, IsolatingRunSequence const& isolatingRunSequence,
    Pair<usize> const& bracketPair, usize startLevelRun, BidiType lastStrongBeforeBracket
) {
    // Any number of characters that had original bidirectional character type NSM prior to the application of W1
    // that immediately follow a paired bracket which changed to L or R under N0 should change to match the type of
    // their preceding bracket.
    auto changeFollowingNSM = [&](usize i, BidiType newType) {
        while (
            i + 1 < paragraph.len() and
            ((
                 getBidiType(input[i + 1]) == BidiType::NSM and paragraph[i + 1] == BidiType::ON
             ) or
             getBidiType(input[i + 1]) == BidiType::BN
            )
        ) {
            paragraph[i + 1] = newType;
            i++;
        }
        return i;
    };

    auto embeddingDirection = isolatingRunSequence.sos;
    bool hasStrongMatchingEmbedding = false;
    bool hasStrongNotMatchingEmbedding = false;

    for (usize j = startLevelRun; j < isolatingRunSequence.levelRuns.len(); j++) {
        auto const& levelRun = isolatingRunSequence.levelRuns[j];
        for (usize i = j == startLevelRun ? bracketPair.v0 : levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            auto pi = applyNeutralTypesScopeChanges(paragraph[i]);

            bool isMatchingEmbedding = embeddingDirection == pi;
            bool isStrong = pi == BidiType::R or pi == BidiType::L;

            if (isStrong) {
                if (isMatchingEmbedding) {
                    hasStrongMatchingEmbedding = true;
                } else {
                    hasStrongNotMatchingEmbedding = true;
                }
            } else if (bracketPair.v1 == i) {
                // If any strong type (either L or R) matching the embedding direction is found, set the type for
                // both brackets in the pair to match the embedding direction
                if (hasStrongMatchingEmbedding) {
                    paragraph[bracketPair.v0] = embeddingDirection;
                    paragraph[bracketPair.v1] = embeddingDirection;

                    changeFollowingNSM(bracketPair.v0, embeddingDirection);
                    changeFollowingNSM(i, embeddingDirection);
                } else if (hasStrongNotMatchingEmbedding) {
                    // Otherwise, if there is a strong type it must be opposite the embedding direction.

                    // If the preceding strong type is also opposite the embedding direction, context is
                    // established, so set the type for both brackets in the pair to that direction.
                    if (lastStrongBeforeBracket != embeddingDirection) {
                        paragraph[bracketPair.v0] = lastStrongBeforeBracket;
                        paragraph[bracketPair.v1] = lastStrongBeforeBracket;

                        changeFollowingNSM(bracketPair.v0, lastStrongBeforeBracket);
                        changeFollowingNSM(i, lastStrongBeforeBracket);
                    } else {
                        // Otherwise set the type for both brackets in the pair to the embedding direction.
                        paragraph[bracketPair.v0] = embeddingDirection;
                        paragraph[bracketPair.v1] = embeddingDirection;

                        changeFollowingNSM(bracketPair.v0, embeddingDirection);
                        changeFollowingNSM(i, embeddingDirection);
                    }
                } else {
                    // Otherwise, there are no strong types within the bracket pair. Therefore, do not set the type
                    // for that bracket pair.
                }
            }
        }
    }
}

// https://unicode.org/reports/tr9/#Resolving_Neutral_Types
void resolveNeutralTypes(
    Slice<Rune> input, MutSlice<BidiType> paragraph, IsolatingRunSequence const& isolatingRunSequence
) {
    // N0. Process bracket pairs in an isolating run sequence sequentially in the logical order of
    // the text positions of the opening paired brackets using the logic given below.

    // Identify the bracket pairs in the current isolating run sequence according to BD16.
    auto bracketPairs = findBracketPairs(input, paragraph, isolatingRunSequence);

    logDebugIf(DEBUG_BIDI, "Bracket pairs found: {}", bracketPairs);

    // For each bracket-pair element in the list of pairs of text positions
    BidiType lastStrongBeforeCurrentBracket = isolatingRunSequence.sos;
    usize bracketPairIndex = 0;

    // Inspect the bidirectional types of the characters enclosed within the bracket pair.
    if (bracketPairs.len()) {
        for (usize j = 0; j < isolatingRunSequence.levelRuns.len(); j++) {
            auto const& levelRun = isolatingRunSequence.levelRuns[j];
            for (usize i = levelRun.start; i < levelRun.end(); i++) {
                if (removeDueToX9(paragraph[i]))
                    continue;

                if (bracketPairs[bracketPairIndex].v0 == i) {
                    runN0ForBracketPair(input, paragraph, isolatingRunSequence, bracketPairs[bracketPairIndex], j, lastStrongBeforeCurrentBracket);

                    bracketPairIndex++;
                    if (bracketPairIndex == bracketPairs.len())
                        break;
                }

                auto pi = applyNeutralTypesScopeChanges(paragraph[i]);
                if (pi == BidiType::R or pi == BidiType::L)
                    lastStrongBeforeCurrentBracket = pi;
            }

            if (bracketPairIndex == bracketPairs.len())
                break;
        }
    }

    // N1. A sequence of NIs takes the direction of the surrounding strong text if the text on both sides has
    // the same direction.
    // European and Arabic numbers act as if they were R in terms of their influence on NIs.
    // The start-of-sequence (sos) and end-of-sequence (eos) types are used at isolating run sequence boundaries.
    auto applySurroundingDirectionToSubseq = [&](BidiType before, BidiType after, Pair<usize> startSeq, Pair<usize> endSeq) {
        if (before != after)
            return;

        for (usize j = startSeq.v1; j <= endSeq.v1; j++) {
            usize start = j == startSeq.v1 ? startSeq.v0 : isolatingRunSequence.levelRuns[j].start;
            usize end = j == endSeq.v1 ? endSeq.v0 : isolatingRunSequence.levelRuns[j].end();

            for (usize i = start; i < end; i++) {
                paragraph[i] = before;
            }
        }
    };

    bool insideNIseq = false;
    BidiType strongBeforeNIseq = isolatingRunSequence.sos;
    Pair<usize> startOfNIseq{};
    for (usize j = 0; j < isolatingRunSequence.levelRuns.len(); j++) {
        auto const& levelRun = isolatingRunSequence.levelRuns[j];
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (isNI(paragraph[i])) {
                if (not insideNIseq) {
                    startOfNIseq = {i, j};
                    insideNIseq = true;
                }
            } else {
                if (insideNIseq)
                    applySurroundingDirectionToSubseq(strongBeforeNIseq, applyNeutralTypesScopeChanges(paragraph[i]), startOfNIseq, {i, j});
                insideNIseq = false;
                strongBeforeNIseq = applyNeutralTypesScopeChanges(paragraph[i]);
            }
        }
    }
    if (insideNIseq) {
        applySurroundingDirectionToSubseq(
            strongBeforeNIseq,
            isolatingRunSequence.eos,
            startOfNIseq,
            {last(isolatingRunSequence.levelRuns).end(),
             isolatingRunSequence.levelRuns.len() - 1}
        );
    }

    // N2. Any remaining NIs take the embedding direction.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (isNI(paragraph[i])) {
                paragraph[i] = isolatingRunSequence.sos;
            }
        }
    }
}

// https://unicode.org/reports/tr9/#Resolving_Implicit_Levels
void resolveImplicitLevels(MutSlice<BidiType> paragraph, IsolatingRunSequence const& isolatingRunSequence, Vec<usize>& levels) {
    // I1. For all characters with an even (left-to-right) embedding level, those of type R go up one level
    // and those of type AN or EN go up two levels.
    // I2. For all characters with an odd (right-to-left) embedding level, those of type L, EN or AN go up one level.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (levels[i] % 2 == 0) {
                if (paragraph[i] == BidiType::R) {
                    levels[i]++;
                } else if (paragraph[i] == BidiType::AN or paragraph[i] == BidiType::EN) {
                    levels[i] += 2;
                }
            } else {
                if (paragraph[i] == BidiType::L or paragraph[i] == BidiType::EN or paragraph[i] == BidiType::AN) {
                    levels[i]++;
                }
            }
        }
    }
}

// https://unicode.org/reports/tr9/#Preparations_for_Implicit_Processing
Vec<IsolatingRunSequence> implicitLevelsAndDirections(MutSlice<BidiType> paragraph, usize paragraphLevel, Vec<usize>& levels, Vec<Opt<usize>> const& matchingPDIIndices) {
    // X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.
    // In rule X9, do not remove any characters, but turn all RLE, LRE, RLO, LRO, and PDF characters into BN.
    for (auto& c : paragraph) {
        if (removeDueToX9(c))
            c = BidiType::BN;
    }

    // X10. Perform the following steps:
    // Compute the set of isolating run sequences as specified by BD13,
    Vec<IsolatingRunSequence> isolatingRunSequences = computeIsolatingRunSequence(paragraph, levels, matchingPDIIndices);

    // Determine the start-of-sequence (sos) and end-of-sequence (eos) types, either L or R, for each isolating run sequence.
    for (auto& isolatingRunSequence : isolatingRunSequences) {
        {
            usize higherLevel = levels[isolatingRunSequence.levelRuns[0].start];
            usize i = isolatingRunSequence.levelRuns[0].start - 1;
            while (i != usize(-1) and removeDueToX9(paragraph[i])) {
                i--;
            }
            if (i == usize(-1))
                higherLevel = max(higherLevel, paragraphLevel);
            else
                higherLevel = max(higherLevel, levels[i]);

            isolatingRunSequence.sos = higherLevel % 2 == 0 ? BidiType::L : BidiType::R;
        }
        {
            usize higherLevel = levels[last(isolatingRunSequence.levelRuns).end() - 1];
            usize i = last(isolatingRunSequence.levelRuns).end() - 1;
            while (i < paragraph.len() and removeDueToX9(paragraph[i])) {
                i++;
            }
            if (i == paragraph.len())
                higherLevel = max(higherLevel, paragraphLevel);
            else
                higherLevel = max(higherLevel, levels[i]);

            isolatingRunSequence.eos = higherLevel % 2 == 0 ? BidiType::L : BidiType::R;
        }
    }
    logDebugIf(DEBUG_BIDI, "isolatingRunSequences: {}", isolatingRunSequences);

    return isolatingRunSequences;
}

// https://unicode.org/reports/tr9/#BD9
// https://unicode.org/reports/tr9/#BD11
// This method returns a vector of indices of matching PDI characters in the paragraph for each isolate initiator
// in the order that they appear in the text.
Vec<Opt<usize>> computeMatchingPDIIndexes(Slice<BidiType> paragraph) {
    Vec<Opt<usize>> matchingPDIIndices;
    Vec<usize> activeIsolate;
    for (usize i = 0; i < paragraph.len(); i++) {
        if (isIsolateInitiator(paragraph[i])) {
            activeIsolate.pushBack(matchingPDIIndices.len());
            matchingPDIIndices.pushBack(NONE);
        } else if (paragraph[i] == BidiType::PDI) {
            if (activeIsolate.len()) {
                matchingPDIIndices[last(activeIsolate)] = i;
                activeIsolate.popBack();
            } else {
                // Otherwise, this PDI is unmatched.
            }
        }
    }

    return matchingPDIIndices;
}

Vec<usize> computeLevelsFromParagraph(Slice<Rune> inputParagraph, MutSlice<BidiType> paragraph, usize paragraphLevel) {
    auto matchingPDIIndices = computeMatchingPDIIndexes(paragraph);
    logDebugIf(DEBUG_BIDI, "matchingPDIIndices {}", matchingPDIIndices);

    // Applying rule X1 (which employs rules X2–X8) to determine explicit embedding levels and directions.
    auto levels = explicitLevelsAndDirections(paragraph, paragraphLevel, matchingPDIIndices);

    logDebugIf(DEBUG_BIDI, "Levels: {}", levels);

    // Applying rule X9 to remove many control characters from further consideration.
    // Applying rule X10 to split the paragraph into isolating run sequences and for each of these:
    auto isolatingRunSequences = implicitLevelsAndDirections(paragraph, paragraphLevel, levels, matchingPDIIndices);

    for (auto const& isolatingRunSequence : isolatingRunSequences) {
        logDebugIf(DEBUG_BIDI, "Processing isolating run sequence: {}", isolatingRunSequence);
        //  Applying rules W1–W7 to resolve weak types.
        resolveWeakTypes(paragraph, isolatingRunSequence);
        logDebugIf(DEBUG_BIDI, "paragraph after resolving weak types: {}", paragraph);

        //  Applying rules N0–N2 to resolve neutral types.
        resolveNeutralTypes(inputParagraph, paragraph, isolatingRunSequence);
        logDebugIf(DEBUG_BIDI, "paragraph after resolving neutral types: {}", paragraph);

        //  Applying rules I1–I2 to resolve implicit embedding levels.
        resolveImplicitLevels(paragraph, isolatingRunSequence, levels);
        logDebugIf(DEBUG_BIDI, "paragraph after resolving implicit levels: {}", paragraph);
    }

    for (usize i = 0; i < paragraph.len(); i++) {
        if (getBidiType(inputParagraph[i]) == BidiType::S)
            levels[i] = 0;
    }

    return levels;
}

export Vec<BidiType> prepareParagraph(Slice<Rune> inputParagraph) {
    Vec<BidiType> paragraph(inputParagraph.len());
    for (usize i = 0; i < inputParagraph.len(); i++) {
        paragraph.pushBack(getBidiType(inputParagraph[i]));
    }
    logDebugIf(DEBUG_BIDI, "Paragraph: {}", paragraph);

    return paragraph;
}

export Tuple<Vec<usize>, usize> computeLevels(Slice<Rune> inputParagraph, Opt<usize> maybeParagraphLevel = NONE) {
    if (maybeParagraphLevel and *maybeParagraphLevel > 1)
        panic("Bidi input paragraph level must be either 0 or 1");

    auto paragraph = prepareParagraph(inputParagraph);

    usize paragraphLevel = maybeParagraphLevel.unwrapOr(determineParagraphLevel(paragraph));
    logDebugIf(DEBUG_BIDI, "Paragraph level: {}", paragraphLevel);

    return {computeLevelsFromParagraph(inputParagraph, paragraph, paragraphLevel), paragraphLevel};
}

// https://unicode.org/reports/tr9/#L1
template <typename T>
void resetSomeEmbeddingsLevelsForL1(MutSlice<T> levels, auto levelFromLineEl, Slice<Rune> inputLine, usize paragraphLevel) {
    auto line = prepareParagraph(inputLine);

    // Any sequence of whitespace characters and/or isolate formatting characters (FSI, LRI, RLI, and PDI)
    auto isBidiTypeToReset = [](BidiType type) {
        return isIsolateInitiator(type) or type == BidiType::PDI or type == BidiType::WS;
    };

    //  On each line, reset the embedding level of the following characters to the paragraph embedding level:
    Opt<usize> startOfSequenceToReset = NONE;
    for (usize i = 0; i < line.len(); i++) {
        if (isBidiTypeToReset(line[i])) {
            if (not startOfSequenceToReset) {
                startOfSequenceToReset = i;
            }
        } else if (line[i] == BidiType::S or line[i] == BidiType::B) {
            // 1. Segment separators,
            // 2. Paragraph separators,
            levelFromLineEl(levels[i]) = paragraphLevel;

            if (startOfSequenceToReset) {
                // 3. Any sequence of whitespace characters and/or isolate formatting characters (FSI, LRI, RLI,
                // and PDI) preceding a segment separator or paragraph separator, and
                for (usize j = *startOfSequenceToReset; j < i; j++) {
                    levelFromLineEl(levels[j]) = paragraphLevel;
                }
            }
            startOfSequenceToReset = NONE;
        } else {
            startOfSequenceToReset = NONE;
        }
    }
    if (startOfSequenceToReset) {
        // 4. Any sequence of whitespace characters and/or isolate formatting characters (FSI, LRI, RLI, and PDI)
        // at the end of the line.
        for (usize i = *startOfSequenceToReset; i < line.len(); i++) {
            levelFromLineEl(levels[i]) = paragraphLevel;
        }
    }
}

// https://unicode.org/reports/tr9/#L2
export template <typename T>
void reorderLine(Slice<Rune> inputLine, usize paragraphLevel, MutSlice<T> line, auto levelFromLineEl) {
    resetSomeEmbeddingsLevelsForL1(line, levelFromLineEl, inputLine, paragraphLevel);

    usize lowestOddLevel = Limits<usize>::MAX;
    for (auto el : line) {
        if (auto level = levelFromLineEl(el); level % 2 == 1) {
            lowestOddLevel = min(lowestOddLevel, level);
        }
    }

    usize maxVisited = Limits<usize>::MAX;
    while (true) {
        usize highestLevel = 0;
        for (auto el : line) {
            if (levelFromLineEl(el) >= maxVisited) {
                continue;
            }
            highestLevel = max(highestLevel, levelFromLineEl(el));
        }

        if (highestLevel < lowestOddLevel) {
            break;
        }

        maxVisited = highestLevel;

        usize i = 0;
        while (i < line.len()) {
            if (levelFromLineEl(line[i]) < highestLevel) {
                i++;
                continue;
            }

            usize start = i;
            while (i < line.len() and levelFromLineEl(line[i]) >= highestLevel) {
                i++;
            }

            reverse(mutSub(line, urange::fromStartEnd(start, i)));
        }
    }
}
}; // namespace Bidi

} // namespace Karm::Icu
