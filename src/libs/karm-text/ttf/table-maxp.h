#pragma once

#include <karm-io/bscan.h>

namespace Ttf {

struct Maxp : Io::BChunk {
    static constexpr Str SIG = "maxp";

    u16 numGlyphs() const {
        auto s = begin();
        s.skip(4);
        return s.next<u16be>();
    }
};

} // namespace Ttf
