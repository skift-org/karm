module;

#include <karm-base/base.h>

export module Karm.Icu:base;

namespace Karm::Icu {

usize const PLANE_SIZE = 65536; // 2^16

// MARK: Bidirectional Character Types
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

} // namespace Karm::Icu
