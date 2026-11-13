export module Karm.Font.Ttf:head;

import Karm.Core;

namespace Karm::Font::Ttf {

export struct Head : Io::BChunk {
    static constexpr Str SIG = "head";

    using UnitsPerEm = Io::BField<u16be, 18>;
    using IndexToLocFormat = Io::BField<u16be, 50>;

    // NOTE: Parser::init() rejects 'head' tables shorter than this,
    //       so the fallbacks below are never hit on a loaded font.
    static constexpr usize MIN_LEN = 54;

    u16 unitPerEm() const {
        return get<UnitsPerEm>().unwrapOr(0);
    }

    u16 locaFormat() const {
        return get<IndexToLocFormat>().unwrapOr(0);
    }
};

} // namespace Karm::Font::Ttf
