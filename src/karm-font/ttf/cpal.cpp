module;

#include <karm/macros>

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

    Res<usize> numPalettes() const {
        return Ok<usize>(try$(get<NumPalettes>()));
    }

    Res<usize> numPaletteEntries() const {
        return Ok<usize>(try$(get<NumPaletteEntries>()));
    }

    Res<Opt<Gfx::Color>> color(usize palette, usize entry) const {
        if (palette >= try$(numPalettes()) or entry >= try$(numPaletteEntries()))
            return Ok(NONE);

        auto indices = try$(begin(COLOR_RECORD_INDICES + palette * 2));
        usize firstRecord = try$(indices.nextU16be());
        usize record = firstRecord + entry;
        if (record >= try$(get<NumColorRecords>()))
            return Ok(NONE);

        // NOTE: Color records are stored as BGRA.
        auto s = try$(begin(try$(get<ColorRecordsArrayOffset>()) + record * COLOR_RECORD_SIZE));
        auto blue = try$(s.nextU8be());
        auto green = try$(s.nextU8be());
        auto red = try$(s.nextU8be());
        auto alpha = try$(s.nextU8be());
        return Ok(Some(Gfx::Color{red, green, blue, alpha}));
    }
};

} // namespace Karm::Font::Ttf
