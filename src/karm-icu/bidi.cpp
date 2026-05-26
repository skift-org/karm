export module Karm.Icu:bidi;

import Karm.Core;
import :base;
import :ucd;

// Implementation of unicode bidi algorithm
// https://unicode.org/reports/tr9/
// https://www.unicode.org/Public/PROGRAMS/BidiReferenceJava/BidiReference.java
namespace Karm::Icu::Bidi {

// https://unicode.org/reports/tr9/#BD8
export bool isIsolateInitiator(BidiClass type) {
    return type == BidiClass::LEFT_TO_RIGHT_ISOLATE or
           type == BidiClass::RIGHT_TO_LEFT_ISOLATE or
           type == BidiClass::FIRST_STRONG_ISOLATE;
}

// https://unicode.org/reports/tr9/#NI
export bool isNeutralOrIsolate(BidiClass type) {
    return isIsolateInitiator(type) or
           type == BidiClass::POP_DIRECTIONAL_ISOLATE or
           type == BidiClass::PARAGRAPH_SEPARATOR or
           type == BidiClass::SEGMENT_SEPARATOR or
           type == BidiClass::WHITE_SPACE or
           type == BidiClass::OTHER_NEUTRAL;
}

static bool bracketsMatch(Rune a, Rune b) {
    // Note that although bracket pairs are defined under canonical equivalence, canonical equivalents only exist
    // between U+3008/U+3009, and U+2329/U+232A, and the Unicode Consortium will not add more such pairs.
    if (a == 0x232A)
        a = 0x3009;
    if (b == 0x2329)
        b = 0x3008;
    return Properties::of(a).bidiMirroringGlyph() == b;
}

// MARK: Resolving Embedding Levels
// https://unicode.org/reports/tr9/#Resolving_Embedding_Levels

// 3.3 Resolving Embedding Levels - https://unicode.org/reports/tr9/#Resolving_Embedding_Levels
// Applying rules P2 and P3 to determine the paragraph level.
usize determineParagraphLevel(Slice<BidiClass> paragraph) {
    // P2. In each paragraph, find the first character of type L, AL, or R while skipping over any characters
    // between an isolate initiator and its matching PDI or, if it has no matching PDI, the end of the paragraph.

    // P3. If a character is found in P2 and it is of type AL or R, then set the paragraph embedding level to one;
    // otherwise, set it to zero.
    usize isolateCount = 0;
    for (auto const& c : paragraph) {
        if (isIsolateInitiator(c)) {
            isolateCount++;
        } else if (c == BidiClass::POP_DIRECTIONAL_ISOLATE) {
            isolateCount--;
        } else if (isolateCount > 0) {
            continue;
        } else if (
            c == BidiClass::ARABIC_LETTER or
            c == BidiClass::RIGHT_TO_LEFT
        ) {
            return 1;
        } else if (c == BidiClass::LEFT_TO_RIGHT) {
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
    MutSlice<BidiClass> paragraph, usize paragraphLevel, Vec<Opt<usize>> const& matchingPDIIndices
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

    auto overrideCharIfNotNeutral = [&](BidiClass c) {
        // Whenever the directional override status of the last entry on the directional status stack is not
        // neutral, reset the current character type according to the directional override status of the last
        // entry on the directional status stack.
        if (last(directionalStatusStack).directionOverrideStatus == DirectionOverrideStatus::LTR) {
            return BidiClass::LEFT_TO_RIGHT;
        } else if (last(directionalStatusStack).directionOverrideStatus == DirectionOverrideStatus::RTL) {
            return BidiClass::RIGHT_TO_LEFT;
        }
        return c;
    };

    // In rules X2 through X5, insert an initial step setting the explicit embedding or
    // override character's embedding level to the embedding level of the last entry on the
    // directional status stack. This applies to RLE, LRE, RLO, and LRO.
    usize matchingPDIIndicesIndex = 0;
    for (usize i = 0; i < paragraph.len(); i++) {
        auto& c = paragraph[i];
        if (c == BidiClass::RIGHT_TO_LEFT_EMBEDDING) {
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

        } else if (c == BidiClass::LEFT_TO_RIGHT_EMBEDDING) {
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

        } else if (c == BidiClass::RIGHT_TO_LEFT_OVERRIDE) {
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
        } else if (c == BidiClass::LEFT_TO_RIGHT_OVERRIDE) {
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
            if (c == BidiClass::FIRST_STRONG_ISOLATE) {
                // apply rules P2 and P3 to the sequence of characters between the FSI and its matching PDI
                // or if there is no matching PDI, the end of the paragraph
                // as if this sequence of characters were a paragraph

                auto isolateParagraphLevel = determineParagraphLevel(
                    sub(paragraph, i + 1, matchingPDIIndices[matchingPDIIndicesIndex].unwrapOr(paragraph.len()))
                );

                // If these rules decide on paragraph embedding level 1, treat the FSI as an RLI in rule X5a.
                // Otherwise, treat it as an LRI in rule X5b.
                c = isolateParagraphLevel == 1
                        ? BidiClass::RIGHT_TO_LEFT_ISOLATE
                        : BidiClass::LEFT_TO_RIGHT_ISOLATE;
            }

            if (c == BidiClass::RIGHT_TO_LEFT_ISOLATE) {
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
        } else if (
            c != BidiClass::PARAGRAPH_SEPARATOR and
            c != BidiClass::POP_DIRECTIONAL_FORMAT and
            c != BidiClass::POP_DIRECTIONAL_ISOLATE
        ) {
            // X6. For all types besides B, BN, RLE, LRE, RLO, LRO, PDF, RLI, LRI, FSI, and PDI:
            // In rule X6, remove the exclusion of BN characters for the purposes of setting embedding levels.

            // Set the current character’s embedding level to the embedding level of the last entry on
            // the directional status stack.
            levels[i] = last(directionalStatusStack).embeddingLevel;

            // Continue not updating the character types of these characters.
            if (c != BidiClass::BOUNDARY_NEUTRAL)
                c = overrideCharIfNotNeutral(c);
        } else if (c == BidiClass::POP_DIRECTIONAL_ISOLATE) {
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
        } else if (c == BidiClass::POP_DIRECTIONAL_FORMAT) {
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
        } else if (c == BidiClass::PARAGRAPH_SEPARATOR) {
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

inline bool removeDueToX9(BidiClass c) {
    // X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.
    return c == BidiClass::BOUNDARY_NEUTRAL or
           c == BidiClass::LEFT_TO_RIGHT_EMBEDDING or
           c == BidiClass::RIGHT_TO_LEFT_OVERRIDE or
           c == BidiClass::LEFT_TO_RIGHT_OVERRIDE or
           c == BidiClass::POP_DIRECTIONAL_FORMAT or
           c == BidiClass::RIGHT_TO_LEFT_EMBEDDING;
}

// https://unicode.org/reports/tr9/#BD13
struct IsolatingRunSequence {
    Vec<urange> levelRuns;
    BidiClass sos = BidiClass::LEFT_TO_RIGHT; //< Start-of-sequence type (L or R)
    BidiClass eos = BidiClass::LEFT_TO_RIGHT; //< End-of-sequence type (L or R)
};

// https://unicode.org/reports/tr9/#BD13
Vec<IsolatingRunSequence> computeIsolatingRunSequence(
    MutSlice<BidiClass> paragraph,
    Vec<usize>& levels,
    Vec<Opt<usize>> const& matchingPDIIndices
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

        // Finding if this level run belongs to an isolating sequence, that should be pointed by isolatingSequenceIndex
        usize isolatingSequenceIndex;
        if (paragraph[l] == BidiClass::POP_DIRECTIONAL_ISOLATE and last(nextMatchingPDIIndices).v0 == l) {
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
};

using BracketPairStack = Vec<BracketPairStackEntry>;

// https://unicode.org/reports/tr9/#BD16
Vec<Pair<usize>> findBracketPairs(
    Slice<Rune> input, MutSlice<BidiClass> paragraph, IsolatingRunSequence const& isolatingRunSequence
) {
    static usize const MAX_DEPTH = 63;
    Vec<Pair<usize>> bracketPairs;
    BracketPairStack bracketPairStack;

    // BD14. An opening paired bracket is a character whose Bidi_Paired_Bracket_Type property value is Open and
    // whose current bidirectional character type is ON.
    auto isOpeningPairedBracket = [&](usize i) {
        return Properties::of(input[i]).bidiPairedBracketType() == BidiPairedBracketType::OPEN and
               paragraph[i] == BidiClass::OTHER_NEUTRAL;
    };

    // BD15. A closing paired bracket is a character whose Bidi_Paired_Bracket_Type property value is Close and
    // whose current bidirectional character type is ON.
    auto isClosingPairedBracket = [&](usize i) {
        return Properties::of(input[i]).bidiPairedBracketType() == BidiPairedBracketType::CLOSE and
               paragraph[i] == BidiClass::OTHER_NEUTRAL;
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
    MutSlice<BidiClass> paragraph, IsolatingRunSequence const& isolatingRunSequence,
    auto predForTarget, auto predForAdjacent, BidiClass newType
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
void resolveWeakTypes(MutSlice<BidiClass> paragraph, IsolatingRunSequence const& isolatingRunSequence) {
    // W1. Examine each nonspacing mark (NSM) in the isolating run sequence, ...
    // could be an OPT
    Opt<usize> prevValidIndex = NONE;
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (paragraph[i] != BidiClass::NONSPACING_MARK) {
                prevValidIndex = i;
                continue;
            }

            if (not prevValidIndex) {
                // If the NSM is at the start of the isolating run sequence, it will get the type of sos.
                paragraph[i] = isolatingRunSequence.sos;
            } else if (isIsolateInitiator(paragraph[*prevValidIndex]) or paragraph[*prevValidIndex] == BidiClass::POP_DIRECTIONAL_ISOLATE) {
                // (...) change the type of the NSM to Other Neutral if the previous character is
                // an isolate initiator or PDI
                paragraph[i] = BidiClass::OTHER_NEUTRAL;
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
            if (paragraph[i] == BidiClass::EUROPEAN_NUMBER) {
                if (lastIndexOfInterest and paragraph[*lastIndexOfInterest] == BidiClass::ARABIC_LETTER)
                    paragraph[i] = BidiClass::ARABIC_NUMBER;
            } else if (
                paragraph[i] == BidiClass::RIGHT_TO_LEFT or
                paragraph[i] == BidiClass::LEFT_TO_RIGHT or
                paragraph[i] == BidiClass::ARABIC_LETTER
            ) {
                lastIndexOfInterest = i;
            }
        }
    }

    // W3. Change all ALs to R.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (paragraph[i] == BidiClass::ARABIC_LETTER) {
                paragraph[i] = BidiClass::RIGHT_TO_LEFT;
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
                paragraph[*prevPrevValidIndex] == BidiClass::EUROPEAN_NUMBER and
                (paragraph[*prevValidIndex] == BidiClass::EUROPEAN_SEPARATOR or paragraph[*prevValidIndex] == BidiClass::COMMON_SEPARATOR) and
                paragraph[i] == BidiClass::EUROPEAN_NUMBER
            ) {
                paragraph[*prevValidIndex] = BidiClass::EUROPEAN_NUMBER;
            } else if (
                paragraph[*prevPrevValidIndex] == BidiClass::ARABIC_NUMBER and
                paragraph[*prevValidIndex] == BidiClass::COMMON_SEPARATOR and
                paragraph[i] == BidiClass::ARABIC_NUMBER
            ) {
                paragraph[*prevValidIndex] = BidiClass::ARABIC_NUMBER;
            }

            prevPrevValidIndex = prevValidIndex;
            prevValidIndex = i;
        }
    }

    // W5. A sequence of European terminators adjacent to European numbers changes to all European numbers.
    changeTypeGivenAdjacentAndTargetPreds(
        paragraph, isolatingRunSequence, [](MutSlice<BidiClass> paragraph, usize i) {
            return paragraph[i] == BidiClass::EUROPEAN_NUMBER;
        },
        [](MutSlice<BidiClass> paragraph, usize i) {
            return paragraph[i] == BidiClass::EUROPEAN_TERMINATOR or
                   paragraph[i] == BidiClass::BOUNDARY_NEUTRAL;
        },
        BidiClass::EUROPEAN_NUMBER
    );

    // W6. All remaining separators and terminators (after the application of W4 and W5) change to Other Neutral.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (paragraph[i] == BidiClass::EUROPEAN_SEPARATOR or
                paragraph[i] == BidiClass::EUROPEAN_TERMINATOR or
                paragraph[i] == BidiClass::COMMON_SEPARATOR) {
                paragraph[i] = BidiClass::OTHER_NEUTRAL;
            }
        }
    }

    // In rule W6, change all BN types adjacent to ET, ES, or CS to ON as well.
    changeTypeGivenAdjacentAndTargetPreds(
        paragraph, isolatingRunSequence,
        [](MutSlice<BidiClass> paragraph, usize i) {
            return paragraph[i] == BidiClass::EUROPEAN_SEPARATOR or
                   paragraph[i] == BidiClass::EUROPEAN_TERMINATOR or
                   paragraph[i] == BidiClass::COMMON_SEPARATOR;
        },
        [](MutSlice<BidiClass> paragraph, usize i) {
            return paragraph[i] == BidiClass::BOUNDARY_NEUTRAL;
        },
        BidiClass::OTHER_NEUTRAL
    );

    // W7. Search backward from each instance of a European number until the first strong type (R, L, or sos) is
    // found. If an L is found, then change the type of the European number to L.
    lastIndexOfInterest = NONE;
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (paragraph[i] == BidiClass::EUROPEAN_NUMBER) {
                if (not lastIndexOfInterest) {
                    if (isolatingRunSequence.sos == BidiClass::LEFT_TO_RIGHT) {
                        paragraph[i] = BidiClass::LEFT_TO_RIGHT;
                    }
                } else if (paragraph[*lastIndexOfInterest] == BidiClass::LEFT_TO_RIGHT) {
                    paragraph[i] = BidiClass::LEFT_TO_RIGHT;
                }
            } else if (
                paragraph[i] == BidiClass::RIGHT_TO_LEFT or
                paragraph[i] == BidiClass::LEFT_TO_RIGHT
            ) {
                lastIndexOfInterest = i;
            }
        }
    }
}

BidiClass applyNeutralTypesScopeChanges(BidiClass type) {
    // Within this scope, bidirectional types EN and AN are treated as R.
    if (type == BidiClass::RIGHT_TO_LEFT or
        type == BidiClass::ARABIC_NUMBER or
        type == BidiClass::EUROPEAN_NUMBER)
        return BidiClass::RIGHT_TO_LEFT;
    return type;
}

// https://unicode.org/reports/tr9/#N0
void runN0ForBracketPair(
    Slice<Rune> input, MutSlice<BidiClass> paragraph, IsolatingRunSequence const& isolatingRunSequence,
    Pair<usize> const& bracketPair, usize startLevelRun, BidiClass lastStrongBeforeBracket
) {
    // Any number of characters that had original bidirectional character type NSM prior to the application of W1
    // that immediately follow a paired bracket which changed to L or R under N0 should change to match the type of
    // their preceding bracket.
    auto changeFollowingNSM = [&](usize i, BidiClass newType) {
        while (
            i + 1 < paragraph.len() and
            ((
                 Properties::of(input[i + 1]).bidiClass() == BidiClass::NONSPACING_MARK and paragraph[i + 1] == BidiClass::OTHER_NEUTRAL
             ) or
             Properties::of(input[i + 1]).bidiClass() == BidiClass::BOUNDARY_NEUTRAL)) {
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
            bool isStrong =
                pi == BidiClass::RIGHT_TO_LEFT or
                pi == BidiClass::LEFT_TO_RIGHT;

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
    Slice<Rune> input, MutSlice<BidiClass> paragraph, IsolatingRunSequence const& isolatingRunSequence
) {
    // N0. Process bracket pairs in an isolating run sequence sequentially in the logical order of
    // the text positions of the opening paired brackets using the logic given below.

    // Identify the bracket pairs in the current isolating run sequence according to BD16.
    auto bracketPairs = findBracketPairs(input, paragraph, isolatingRunSequence);

    // For each bracket-pair element in the list of pairs of text positions
    BidiClass lastStrongBeforeCurrentBracket = isolatingRunSequence.sos;
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
                if (
                    pi == BidiClass::RIGHT_TO_LEFT or
                    pi == BidiClass::LEFT_TO_RIGHT
                )
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
    auto applySurroundingDirectionToSubseq = [&](BidiClass before, BidiClass after, Pair<usize> startSeq, Pair<usize> endSeq) {
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
    BidiClass strongBeforeNIseq = isolatingRunSequence.sos;
    Pair<usize> startOfNIseq{};
    for (usize j = 0; j < isolatingRunSequence.levelRuns.len(); j++) {
        auto const& levelRun = isolatingRunSequence.levelRuns[j];
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (removeDueToX9(paragraph[i]))
                continue;

            if (isNeutralOrIsolate(paragraph[i])) {
                if (not insideNIseq) {
                    startOfNIseq = {i, j};
                    insideNIseq = true;
                }
            } else {
                if (insideNIseq)
                    applySurroundingDirectionToSubseq(
                        strongBeforeNIseq,
                        applyNeutralTypesScopeChanges(paragraph[i]),
                        startOfNIseq,
                        {i, j}
                    );
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

            if (isNeutralOrIsolate(paragraph[i])) {
                paragraph[i] = isolatingRunSequence.sos;
            }
        }
    }
}

// https://unicode.org/reports/tr9/#Resolving_Implicit_Levels
void resolveImplicitLevels(MutSlice<BidiClass> paragraph, IsolatingRunSequence const& isolatingRunSequence, Vec<usize>& levels) {
    // I1. For all characters with an even (left-to-right) embedding level, those of type R go up one level
    // and those of type AN or EN go up two levels.
    // I2. For all characters with an odd (right-to-left) embedding level, those of type L, EN or AN go up one level.
    for (auto const& levelRun : isolatingRunSequence.levelRuns) {
        for (usize i = levelRun.start; i < levelRun.end(); i++) {
            if (levels[i] % 2 == 0) {
                if (paragraph[i] == BidiClass::RIGHT_TO_LEFT) {
                    levels[i]++;
                } else if (
                    paragraph[i] == BidiClass::ARABIC_NUMBER or
                    paragraph[i] == BidiClass::EUROPEAN_NUMBER
                ) {
                    levels[i] += 2;
                }
            } else {
                if (
                    paragraph[i] == BidiClass::LEFT_TO_RIGHT or
                    paragraph[i] == BidiClass::EUROPEAN_NUMBER or
                    paragraph[i] == BidiClass::ARABIC_NUMBER
                ) {
                    levels[i]++;
                }
            }
        }
    }
}

// https://unicode.org/reports/tr9/#Preparations_for_Implicit_Processing
Vec<IsolatingRunSequence> implicitLevelsAndDirections(MutSlice<BidiClass> paragraph, usize paragraphLevel, Vec<usize>& levels, Vec<Opt<usize>> const& matchingPDIIndices) {
    // X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.
    // In rule X9, do not remove any characters, but turn all RLE, LRE, RLO, LRO, and PDF characters into BN.
    for (auto& c : paragraph) {
        if (removeDueToX9(c))
            c = BidiClass::BOUNDARY_NEUTRAL;
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

            isolatingRunSequence.sos = higherLevel % 2 == 0 ? BidiClass::LEFT_TO_RIGHT : BidiClass::RIGHT_TO_LEFT;
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

            isolatingRunSequence.eos = higherLevel % 2 == 0 ? BidiClass::LEFT_TO_RIGHT : BidiClass::RIGHT_TO_LEFT;
        }
    }

    return isolatingRunSequences;
}

// https://unicode.org/reports/tr9/#BD9
// https://unicode.org/reports/tr9/#BD11
// This method returns a vector of indices of matching PDI characters in the paragraph for each isolate initiator
// in the order that they appear in the text.
Vec<Opt<usize>> computeMatchingPDIIndexes(Slice<BidiClass> paragraph) {
    Vec<Opt<usize>> matchingPDIIndices;
    Vec<usize> activeIsolate;
    for (usize i = 0; i < paragraph.len(); i++) {
        if (isIsolateInitiator(paragraph[i])) {
            activeIsolate.pushBack(matchingPDIIndices.len());
            matchingPDIIndices.pushBack(NONE);
        } else if (paragraph[i] == BidiClass::POP_DIRECTIONAL_ISOLATE) {
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

Vec<usize> computeLevelsFromParagraph(Slice<Rune> inputParagraph, MutSlice<BidiClass> paragraph, usize paragraphLevel) {
    auto matchingPDIIndices = computeMatchingPDIIndexes(paragraph);

    // Applying rule X1 (which employs rules X2–X8) to determine explicit embedding levels and directions.
    auto levels = explicitLevelsAndDirections(paragraph, paragraphLevel, matchingPDIIndices);

    // Applying rule X9 to remove many control characters from further consideration.
    // Applying rule X10 to split the paragraph into isolating run sequences and for each of these:
    auto isolatingRunSequences = implicitLevelsAndDirections(paragraph, paragraphLevel, levels, matchingPDIIndices);

    for (auto const& isolatingRunSequence : isolatingRunSequences) {
        //  Applying rules W1–W7 to resolve weak types.
        resolveWeakTypes(paragraph, isolatingRunSequence);

        //  Applying rules N0–N2 to resolve neutral types.
        resolveNeutralTypes(inputParagraph, paragraph, isolatingRunSequence);

        //  Applying rules I1–I2 to resolve implicit embedding levels.
        resolveImplicitLevels(paragraph, isolatingRunSequence, levels);
    }

    for (usize i = 0; i < paragraph.len(); i++) {
        if (Properties::of(inputParagraph[i]).bidiClass() == BidiClass::SEGMENT_SEPARATOR)
            levels[i] = 0;
    }

    return levels;
}

export Vec<BidiClass> prepareParagraph(Slice<Rune> inputParagraph) {
    Vec<BidiClass> paragraph(inputParagraph.len());
    for (usize i = 0; i < inputParagraph.len(); i++) {
        paragraph.pushBack(Properties::of(inputParagraph[i]).bidiClass());
    }

    return paragraph;
}

export Tuple<Vec<usize>, usize> computeLevels(Slice<Rune> inputParagraph, Opt<usize> maybeParagraphLevel = NONE) {
    if (maybeParagraphLevel and *maybeParagraphLevel > 1)
        panic("Bidi input paragraph level must be either 0 or 1");

    auto paragraph = prepareParagraph(inputParagraph);

    usize paragraphLevel = maybeParagraphLevel.unwrapOr(determineParagraphLevel(paragraph));

    return {computeLevelsFromParagraph(inputParagraph, paragraph, paragraphLevel), paragraphLevel};
}

// https://unicode.org/reports/tr9/#L1
template <typename T>
void resetSomeEmbeddingsLevelsForL1(MutSlice<T> levels, auto levelFromLineEl, Slice<Rune> inputLine, usize paragraphLevel) {
    auto line = prepareParagraph(inputLine);

    // Any sequence of whitespace characters and/or isolate formatting characters (FSI, LRI, RLI, and PDI)
    auto isBidiClassToReset = [](BidiClass type) {
        return isIsolateInitiator(type) or type == BidiClass::POP_DIRECTIONAL_ISOLATE or type == BidiClass::WHITE_SPACE;
    };

    //  On each line, reset the embedding level of the following characters to the paragraph embedding level:
    Opt<usize> startOfSequenceToReset = NONE;
    for (usize i = 0; i < line.len(); i++) {
        if (isBidiClassToReset(line[i])) {
            if (not startOfSequenceToReset) {
                startOfSequenceToReset = i;
            }
        } else if (line[i] == BidiClass::SEGMENT_SEPARATOR or line[i] == BidiClass::PARAGRAPH_SEPARATOR) {
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

} // namespace Karm::Icu::Bidi
