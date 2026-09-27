export module Karm.Font.Ttf:cpal;

import Karm.Core;
import Karm.Gfx;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/cpal
export struct Cpal : Io::BChunk {
    static constexpr Str SIG = "CPAL";

    using NumPaletteEntries = Io::BField<u16be, 2>;
    using NumPalettes = Io::BField<u16be, 4>;
    using NumColorRecords = Io::BField<u16be, 6>;
    using ColorRecordsArrayOffset = Io::BField<u32be, 8>;

    static constexpr usize COLOR_RECORD_INDICES = 12;
    static constexpr usize COLOR_RECORD_SIZE = 4;

    usize numPalettes() const {
        return get<NumPalettes>();
    }

    usize numPaletteEntries() const {
        return get<NumPaletteEntries>();
    }

    Opt<Gfx::Color> color(usize palette, usize entry) const {
        if (palette >= numPalettes() or entry >= numPaletteEntries())
            return NONE;

        usize firstRecord = begin().skip(COLOR_RECORD_INDICES + palette * 2).nextU16be();
        usize record = firstRecord + entry;
        if (record >= get<NumColorRecords>())
            return NONE;

        // NOTE: Color records are stored as BGRA.
        auto s = begin().skip(get<ColorRecordsArrayOffset>() + record * COLOR_RECORD_SIZE);
        auto blue = s.nextU8be();
        auto green = s.nextU8be();
        auto red = s.nextU8be();
        auto alpha = s.nextU8be();
        return Some(Gfx::Color{red, green, blue, alpha});
    }
};

} // namespace Karm::Font::Ttf
