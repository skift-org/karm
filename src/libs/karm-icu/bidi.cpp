module;

#include <karm-base/string.h>

export module Karm.Icu:bidi;

namespace Karm::Icu {

// Implementation of unicode bidi algorithm
// https://unicode.org/reports/tr9/
// https://www.unicode.org/Public/PROGRAMS/BidiReferenceJava/BidiReference.java

// 3.2 MARK: Bidirectional Character Types -------------------------------------
// https://unicode.org/reports/tr9/#Bidirectional_Character_Types
export enum struct BidiType : u8 {
    L = 0,    //< Left-to-right
    LRE = 1,  //< Left-to-Right Embedding
    LRO = 2,  //< Left-to-Right Override
    R = 3,    //< Right-to-Left
    AL = 4,   //< Right-to-Left Arabic
    RLE = 5,  //< Right-to-Left Embedding
    RLO = 6,  //< Right-to-Left Override
    PDF = 7,  //< Pop Directional Format
    EN = 8,   //< European Number
    ES = 9,   //< European Number Separator
    ET = 10,  //< European Number Terminator
    AN = 11,  //< Arabic Number
    CS = 12,  //< Common Number Separator
    NSM = 13, //< Non-Spacing Mark
    BN = 14,  //< Boundary Neutral
    B = 15,   //< Paragraph Separator
    S = 16,   //< Segment Separator
    WS = 17,  //< Whitespace
    ON = 18,  //< Other Neutrals
    LRI = 19, //< Left-to-Right Isolate
    RLI = 20, //< Right-to-Left Isolate
    FSI = 21, //< First-Strong Isolate
    PDI = 22, //< Pop Directional Isolate

    _LEN
};

bool isIsolateInitiator(BidiType type) {
    return type == BidiType::LRI || type == BidiType::RLI || type == BidiType::FSI;
}

export struct Bidi {
    // MARK: Resolving Embedding Levels
    // https://unicode.org/reports/tr9/#Resolving_Embedding_Levels

    // 3.3 Resolving Embedding Levels - https://unicode.org/reports/tr9/#Resolving_Embedding_Levels
    // Applying rules P2 and P3 to determine the paragraph level.
    usize determineParagraphLevel(Slice<BidiType> paragraph) {
        // P2. In each paragraph, find the first character of type L, AL, or R while skipping over any characters
        // between an isolate initiator and its matching PDI or, if it has no matching PDI, the end of the paragraph.

        // P3. If a character is found in P2 and it is of type AL or R, then set the paragraph embedding level to one;
        // otherwise, set it to zero.
        usize isoleteCount = 0;
        for (auto const& c : paragraph) {
            if (isIsolateInitiator(c)) {
                isoleteCount++;
            } else if (c == BidiType::PDI) {
                isoleteCount--;
            } else if (isoleteCount > 0) {
                continue;
            } else if (c == BidiType::AL || c == BidiType::R) {
                return 1;
            } else if (c == BidiType::L) {
                return 0;
            }
        }

        return 0;
    }

    enum struct DirectionOverrideStatus : u8 {
        NEUTRAL = 0, //< Neutral
        LTR = 1,     //< Left-to-Right
        RTL = 2      //< Right-to-Left
    };

    struct DirectionalStatusStack {
        struct Entry {
            usize embeddingLevel;
            // https://unicode.org/reports/tr9/#BD6
            // BD6. The directional override status determines whether the bidirectional type of characters is to
            // be reset. The directional override status is set by using explicit directional formatting characters.
            // This status has three states, as shown in Table 2.
            DirectionOverrideStatus directionOverrideStatus;

            // https://unicode.org/reports/tr9/#BD12
            // BD12. The directional isolate status is a Boolean value set by using isolate formatting characters:
            // it is true when the current embedding level was started by an isolate initiator.
            bool directionIsolateStatus;
        };

        static usize const MAX_DEPTH = 125;

        Vec<Entry> entries;

        DirectionalStatusStack(usize paragraphEmbeddingLevel) : entries() {
            entries.pushBack({.embeddingLevel = paragraphEmbeddingLevel, .directionOverrideStatus = NEUTRAL, .directionIsolateStatus = false});
        }

        Entry const& top() {
            if (entries.isEmpty()) {
                panic("");
            }
            return entries.back();
        }

        void pop() {
            if (entries.isEmpty()) {
                panic("");
            }
            entries.popBack();
        }

        void push(usize embeddingLevel, DirectionOverrideStatus directionOverrideStatus, bool directionIsolateStatus) {
            entries.pushBack({.embeddingLevel = embeddingLevel, .directionOverrideStatus = directionOverrideStatus, .directionIsolateStatus = directionIsolateStatus});
        }

        bool isFull() const {
            return entries.len() >= MAX_DEPTH;
        }
    };

    // https://unicode.org/reports/tr9/#Explicit_Levels_and_Directions
    Vec<usize> explicitLevelsAndDirections(Slice<BidiType> paragraph, usize paragraphLevel, Vec<usize> &levels, Vec<Opt<usize>> const& matchingPDIIndices) {
        // https://unicode.org/reports/tr9/#X1
        // At the start of the pass, the directional status stack is initialized to an entry reflecting the
        // paragraph embedding level, with the directional override status neutral and the directional
        // isolate status false; this entry is not popped off until the end of the paragraph.
        DirectionalStatusStack directionalStatusStack;

        // number of isolate initiators that were encountered in the pass so far without encountering their matching
        // PDIs, but were invalidated by the depth limit and thus are not reflected in the directional status stack
        usize overflowIsolateCount = 0;

        // number of embedding initiators that were encountered in the pass so far without encountering their
        // matching PDF, or encountering the PDI of an isolate within which they are nested, but were invalidated
        // by the depth limit, and thus are not reflected in the directional status stack.
        usize overflowEmbeddingCount = 0;

        // number of isolate initiators that were encountered in the pass so far without encountering their matching
        // PDIs, and have been judged valid by the depth limit
        usize validIsolateCount = 0;

        auto leastOddGreaterThan(usize level) {
            if (level % 2 == 0) {
                return level + 1;
            }
            return level + 2;
        };

        auto leastEvenGreaterThan(usize level) {
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
            return newEmbeddingLevel <= DirectionalStatusStack::MAX_DEPTH &&
                   overflowIsolateCount == 0 &&
                   overflowEmbeddingCount == 0;
        };

        auto overrideCharIfNotNeutral = [&](BidiType c) {
            // Whenever the directional override status of the last entry on the directional status stack is not
            // neutral, reset the current character type according to the directional override status of the last
            // entry on the directional status stack.
            if (directionalStatusStack.top().directionOverrideStatus == DirectionOverrideStatus::LTR) {
                return BidiType::L;
            } else if (directionalStatusStack.top().directionOverrideStatus == DirectionOverrideStatus::RTL) {
                return BidiType::R;
            }
            return c;
        };

        usize matchingPDIIndicesIndex = 0;
        for (usize i = 0; i < paragraph.len(); i++) {
            auto& c = paragraph[i];
            // https://unicode.org/reports/tr9/#X2
            // X2. With each RLE, perform the following steps:
            if (c == BidiType::RLE) {
                // Compute the least odd embedding level greater than the embedding level of the last entry on the
                // directional status stack.
                usize newEmbeddingLevel = leastOddGreaterThan(directionalStatusStack.top().embeddingLevel);

                if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                    // ..., then this RLE is valid.
                    // Push an entry consisting of the new embedding level, neutral directional override status, and
                    // false directional isolate status onto the directional status stack.
                    directionalStatusStack.push(newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, false);
                } else {
                    incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
                }

            } else if (c == BidiType::LRE) {
                // https://unicode.org/reports/tr9/#X3
                // X3. With each LRE, perform the following steps:

                // Compute the least even embedding level greater than the embedding level of the last entry on
                // the directional status stack.
                usize newEmbeddingLevel = leastEvenGreaterThan(directionalStatusStack.top().embeddingLevel);

                if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                    // ..., then this LRE is valid.
                    // Push an entry consisting of the new embedding level, neutral directional override status,
                    // and false directional isolate status onto the directional status stack.
                    directionalStatusStack.push(newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, false);
                } else {
                    incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
                }

            } else if (c == BidiType::RLO) {
                // https://unicode.org/reports/tr9/#X4
                // X4. With each RLO, perform the following steps:

                // Compute the least odd embedding level greater than the embedding level of the last entry on the directional status stack.
                usize newEmbeddingLevel = leastOddGreaterThan(directionalStatusStack.top().embeddingLevel);

                if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                    // , then this RLO is valid.
                    // Push an entry consisting of the new embedding level, right-to-left directional override
                    // status, and false directional isolate status onto the directional status stack.
                    directionalStatusStack.push(newEmbeddingLevel, DirectionOverrideStatus::RTL, false);
                } else {
                    incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
                }
            } else if (c == BidiType::LRO) {
                // https://unicode.org/reports/tr9/#X5
                // X5. With each LRO, perform the following steps:

                // Compute the least even embedding level greater than the embedding level of the last entry on the directional status stack.
                usize newEmbeddingLevel = leastEvenGreaterThan(directionalStatusStack.top().embeddingLevel);

                if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                    // , then this LRO is valid.
                    // Push an entry consisting of the new embedding level, left-to-right directional override
                    // status, and false directional isolate status onto the directional status stack.
                    directionalStatusStack.push(newEmbeddingLevel, DirectionOverrideStatus::LTR, false);
                } else {
                    incrementOverflowEmbeddingCountIfOverFlowIsolateCountZero();
                }
            } else if (isIsolateInitiator(c)) {
                if (c == BidiType::FSI) {
                    // apply rules P2 and P3 to the sequence of characters between the FSI and its matching PDI
                    // or if there is no matching PDI, the end of the paragraph
                    // as if this sequence of characters were a paragraph

                    auto isolateParagraphLevel = determineParagraphLevel(
                        sub(paragraph, i + 1, matchingPDIIndices[matchingPDIIndicesIndex].unwrap_or(paragraph.len()))
                    );

                    // If these rules decide on paragraph embedding level 1, treat the FSI as an RLI in rule X5a.
                    // Otherwise, treat it as an LRI in rule X5b.
                    c = isolateParagraphLevel == 1 ? BidiType::RLI : BidiType::LRI;
                }

                if (c == BidiType::RLI) {
                    // https://unicode.org/reports/tr9/#X5a
                    // X5a. With each RLI, perform the following steps:

                    // Set the RLI's embedding level to the embedding level of the last entry on the directional status stack.
                    usize rliEmbeddingLevel = directionalStatusStack.top().embeddingLevel;

                    c = overrideCharIfNotNeutral(c);

                    // Compute the least odd embedding level greater than the embedding level of the last entry on
                    // the directional status stack.
                    usize newEmbeddingLevel = leastOddGreaterThan(directionalStatusStack.top().embeddingLevel);

                    // If this new level would be valid and the overflow isolate count and the overflow embedding count
                    // are both zero,
                    if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                        // then this RLI is valid.
                        // Increment the valid isolate count by one,
                        validIsolateCount++;
                        // and push an entry consisting of the new embedding level, neutral directional override status,
                        // and true directional isolate status onto the directional status stack.
                        directionalStatusStack.push(newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, true);
                    } else {
                        // Otherwise, this is an overflow RLI. Increment the overflow isolate count by one,
                        // and leave all other variables unchanged.
                        overflowIsolateCount++;
                    }

                } else {
                    // https://unicode.org/reports/tr9/#X5b
                    // X5b. With each LRI, perform the following steps:
                    // Set the LRI's embedding level to the embedding level of the last entry on the directional status stack.
                    usize rliEmbeddingLevel = directionalStatusStack.top().embeddingLevel;

                    c = overrideCharIfNotNeutral(c);

                    // Compute the least even embedding level greater than the embedding level of the last entry on the
                    // directional status stack.
                    usize newEmbeddingLevel = leastEvenGreaterThan(directionalStatusStack.top().embeddingLevel);

                    // If this new level would be valid and the overflow isolate count and the overflow embedding count
                    // are both zero,
                    if (checkIfValidNewLevelAndCounts(newEmbeddingLevel)) {
                        // then this LRI is valid.
                        // Increment the valid isolate count by one,
                        validIsolateCount++;
                        // and push an entry consisting of the new embedding level, neutral directional override status,
                        //  and true directional isolate status onto the directional status stack.
                        directionalStatusStack.push(newEmbeddingLevel, DirectionOverrideStatus::NEUTRAL, true);
                    } else {
                        // Otherwise, this is an overflow LRI. Increment the overflow isolate count by one, and leave
                        // all other variables unchanged.
                        overflowIsolateCount++;
                    }
                }
            } else if(c != BidiType::B and c != BidiType::BN and c != BidiType::PDF and c != BidiType::PDI) {
                // https://unicode.org/reports/tr9/#X6
                // X6. For all types besides B, BN, RLE, LRE, RLO, LRO, PDF, RLI, LRI, FSI, and PDI:
                
                // Set the current character’s embedding level to the embedding level of the last entry on the directional status stack.
                levels[i] = directionalStatusStack.top().embeddingLevel;

                c = overrideCharIfNotNeutral(c);
            } else if(c == BidiType::PDI) {
                // If the overflow isolate count is greater than zero, 
                if(overflowIsolateCount > 0) {
                    // this PDI matches an overflow isolate initiator. 
                    // Decrement the overflow isolate count by one.
                    overflowIsolateCount--;
                } if(validIsolateCount == 0) {
                    // Otherwise, if the valid isolate count is zero, this PDI does not match any isolate initiator,
                    // valid or overflow. 
                    // Do nothing.
                } else {
                    // Otherwise, this PDI matches a valid isolate initiator. Perform the following steps:
                    // Reset the overflow embedding count to zero. 
                    overflowEmbeddingCount = 0;

                    // While the directional isolate status of the last entry on the stack is false, 
                    // pop the last entry from the directional status stack. 
                    while(not directionalStatusStack.top().directionIsolateStatus) 
                        directionalStatusStack.pop();
                    
                    // Pop the last entry from the directional status stack and decrement the valid isolate count by one. 
                    directionalStatusStack.pop();
                    validIsolateCount--;
                }

                // Set the PDI’s level to the entry's embedding level.
                levels[i] = directionalStatusStack.top().embeddingLevel;
                c = overrideCharIfNotNeutral(c);
            } else if(c == BidiType::PDF) {
                // If the overflow isolate count is greater than zero, 
                if(overflowIsolateCount > 0) {
                    // do nothing. 
                } else if(overflowEmbeddingCount > 0) {
                    // Otherwise, if the overflow embedding count is greater than zero, decrement it by one. 
                    overflowEmbeddingCount--;
                } else if(not directionalStatusStack.top().directionIsolateStatus and directionalStatusStack.len() > ){
                    // Otherwise, if the directional isolate status of the last entry on the directional status stack
                    // is false, and the directional status stack contains at least two entries, 
                    // pop the last entry from the directional status stack.
                    directionalStatusStack.pop();
                } else {
                    // Otherwise, do nothing. 
                }
            } else if(c == BidiType::B) {
                // Explicit paragraph separators (bidirectional character type B) indicate the end of a paragraph.
                // As such, they are not included in any embedding, override or isolate.
                // They are simply assigned the paragraph embedding level.
                levels[i] = paragraphLevel;
            }

            if (isIsolateInitiator(c))
                matchingPDIIndicesIndex++;
        }
    }

    struct IsolatingRunSequence {
        Vec<urange> levelRuns;
        BidiType sos;     //< Start-of-sequence type (L or R)
        BidiType eos;     //< End-of-sequence type (L or R)
    };

    void computeIsolatingRunSequence(Slice<BidiType> paragraph, Vec<usize> &levels, Vec<Opt<usize>> const& matchingPDIIndices){
        Vec<IsolatingRunSequence> isolatingRunSequences;

        // keeping next matching PDI index and related isolating sequence index
        Vec<Pair<usize>> nextMatchingPDIIndices;

        usize matchingPDIIndicesIndex = 0;
        for(usize l = 0; l < paragraph.len();) {    
            
            // computing level run
            usize r = l + 1;
            while(r < paragraph.len() and levels[r] == levels[l]) {
                r++;
            }

            // updating matchingPDIIndicesIndex
            for(usize i = l; i < r; i++) {
                matchingPDIIndicesIndex += isIsolateInitiator(paragraph[i]) ? 1 : 0;   
            }

            // l...r is a level run
            // computing isolatingSequenceIndex
            usize isolatingSequenceIndex;
            if(paragraph[l] == BidiType::PDI and nextMatchingPDIIndices.back().first == l) {
                isolatingSequenceIndex = nextMatchingPDIIndices.back().second;
                isolatingRunSequences[isolatingSequenceIndex].levelRuns.pushBack(urange{l, r});
                // we should be continuing a sequence run from before if PDI is matching
            } else {
                isolatingSequenceIndex = isolatingRunSequences.len();
                isolatingRunSequences.pushBack({
                    .levelRuns = {urange{l, r}},
                    .sos = BidiType::L, // default to L
                    .eos = BidiType::L  // default to L
                });
            }

            
            if(isIsolateInitiator(paragraph[r - 1]) and matchingPDIIndices[matchingPDIIndicesIndex]){
                nextMatchingPDIIndices.pushBack({
                    *matchingPDIIndices[matchingPDIIndicesIndex],
                    isolatingSequenceIndex
                });
            }
        }
        return isolatingRunSequences;
    }

    bool removeDueToX9(BidiType c) {
        // https://unicode.org/reports/tr9/#X9
        // X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.
        return c == BidiType::RLE || c == BidiType::LRE || c == BidiType::RLO ||
               c == BidiType::LRO || c == BidiType::PDF || c == BidiType::BN;
    }

    // https://unicode.org/reports/tr9/#Resolving_Weak_Types
    void resolveWeakTypes(){
        // implementa oq manda mesmo pq eh linear
        // dps esse
    }

    // https://unicode.org/reports/tr9/#Resolving_Neutral_Types
    void resolveNeutralTypes(){
        // por fim esse
    }

    // https://unicode.org/reports/tr9/#Resolving_Implicit_Levels
    void resolveImplicitLevels(){
        // facinho, esse primeiro
    }

    // https://unicode.org/reports/tr9/#Preparations_for_Implicit_Processing
    Vec<usize> implicitLevelsAndDirections(Slice<BidiType> paragraph, usize paragraphLevel, Vec<usize> &levels, Vec<Opt<usize>> const& matchingPDIIndices) {
        // X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.

        // X10. Perform the following steps:

        // Compute the set of isolating run sequences as specified by BD13,
        Vec<IsolatingRunSequence> isolatingRunSequences = computeIsolatingRunSequence(paragraph, levels, matchingPDIIndices);

        // Determine the start-of-sequence (sos) and end-of-sequence (eos) types, either L or R, for each isolating run sequence.
        for(auto & isolatingRunSequence : isolatingRunSequences) {
            // determine sos and eos types
            {
                usize i = isolatingRunSequence.levelRuns[0].start - 1;
                while(i > 0 and removeDueToX9(paragraph[i])) {
                    i--;
                }
    
                isolatingRunSequence.sos = i == 0 and removeDueToX9(paragraph[0]) ? paragraphLevel : paragraph[i];
            }
            {
                usize i = isolatingRunSequence.levelRuns.last().end;
                while(i < paragraph.len() and removeDueToX9(paragraph[i])) {
                    i++;
                }
    
                isolatingRunSequence.eos = i == paragraph.len()? paragraphLevel : paragraph[i];
            }
        }
    }

    // https://unicode.org/reports/tr9/#BD9
    Vec<Opt<usize>> computeMatchingPDIIndexes(Slice<BidiType> paragraph) {
        Vec<Opt<usize>> matchingPDIIndices;
        Vec<usize> activeIsolate;
        for (auto i = 0; i < paragraph.len(); i++) {
            auto c = paragraph[i];
            if (isIsolateInitiator(c)) {
                matchingPDIIndices.pushBack(NONE);
                activeIsolate.pushBack(matchingPDIIndices.len());
            } else if (c == BidiType::PDI) {
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

    // // https://unicode.org/reports/tr9/#BD11
    // Vec<Opt<usize>> computeMatchingPDFIndexes(Slice<BidiType> paragraph){

    void run(Slice<BidiType> paragraph) {
        // Applying rule P1 to split the text into paragraphs, and for each of these:

        //   Applying rules P2 and P3 to determine the paragraph level.
        usize paragraphLevel = determineParagraphLevel(paragraph);

        //   Applying rule X1 (which employs rules X2–X8) to determine explicit embedding levels and directions.

        //   Applying rule X9 to remove many control characters from further consideration.

        //   Applying rule X10 to split the paragraph into isolating run sequences and for each of these:

        //       Applying rules W1–W7 to resolve weak types.

        //       Applying rules N0–N2 to resolve neutral types.

        //       Applying rules I1–I2 to resolve implicit embedding levels.
    }
};

} // namespace Karm::Icu
