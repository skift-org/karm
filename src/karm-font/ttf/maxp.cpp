module;

#include <karm/macros>

export module Karm.Font.Ttf:maxp;

import Karm.Core;

namespace Karm::Font::Ttf {

export struct Maxp : Io::BChunk {
    static constexpr Str SIG = "maxp";

    Res<u16> numGlyphs() const {
        auto s = try$(begin(4));
        return s.nextU16be();
    }
};

} // namespace Karm::Font::Ttf
