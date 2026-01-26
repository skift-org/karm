export module Karm.Font.Ttf:maxp;

import Karm.Core;

namespace Karm::Font::Ttf {

export struct Maxp : Io::BChunk {
    static constexpr Str SIG = "maxp";

    u16 numGlyphs() const {
        auto s = begin();
        s.skip(4);
        return s.nextU16be();
    }
};

} // namespace Karm::Font::Ttf
