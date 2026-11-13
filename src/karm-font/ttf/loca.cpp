module;

#include <karm/macros>

export module Karm.Font.Ttf:loca;

import Karm.Logger;

import :head;

namespace Karm::Font::Ttf {

export struct Loca : Io::BChunk {
    static constexpr Str SIG = "loca";

    Res<usize> glyfOffset(isize glyphId, Head const& head) const {
        auto format = head.locaFormat();
        if (format == 0) {
            auto s = try$(begin(glyphId * 2));
            return Ok(try$(s.nextU16be()) * 2uz);
        } else if (format == 1) {
            auto s = try$(begin(glyphId * 4));
            return Ok<usize>(try$(s.nextU32be()));
        } else {
            logWarn("unsupported loca format {:x}", format);
            return Error::invalidData("unsupported loca format");
        }
    }
};

} // namespace Karm::Font::Ttf
