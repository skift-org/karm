module;

#include <karm/macros>

export module Karm.Font.Woff:base;

import Karm.Core;

namespace Karm::Font::Woff {

/// Every length found in a container comes from the network, so nothing is
/// allocated past this.
export constexpr usize MAX_SFNT_SIZE = 128 * 1024 * 1024;

constexpr u32 tag(char const (&s)[5]) {
    return (u32)(u8)s[0] << 24 | (u32)(u8)s[1] << 16 | (u32)(u8)s[2] << 8 | (u32)(u8)s[3];
}

} // namespace Karm::Font::Woff
