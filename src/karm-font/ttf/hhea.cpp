export module Karm.Font.Ttf:hhea;

import Karm.Core;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/hhea
export struct Hhea : Io::BChunk {
    static constexpr Str SIG = "hhea";

    using Ascender = Io::BField<i16be, 4>;
    using Descender = Io::BField<i16be, 6>;
    using LineGap = Io::BField<i16be, 8>;
    using AdvanceWidthMax = Io::BField<u16be, 10>;
    using NumberOfHMetrics = Io::BField<u16be, 34>;

    // NOTE: Parser::init() rejects 'hhea' tables shorter than this,
    //       so the fallbacks below are never hit on a loaded font.
    static constexpr usize MIN_LEN = 36;

    isize ascender() const {
        return get<Ascender>().unwrapOr(0);
    }

    isize descender() const {
        return -get<Descender>().unwrapOr(0);
    }

    isize lineGap() const {
        return get<LineGap>().unwrapOr(0);
    }

    isize advanceWidthMax() const {
        return get<AdvanceWidthMax>().unwrapOr(0);
    }

    isize numberOfHMetrics() const {
        return get<NumberOfHMetrics>().unwrapOr(0);
    }
};

} // namespace Karm::Font::Ttf
