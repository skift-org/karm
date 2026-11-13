module;

#include <karm/macros>

export module Karm.Font.Ttf:colr;

import Karm.Core;
import Karm.Gfx;

import :cpal;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/colr
// NOTE: Only version 0 (layered glyphs with solid colors) is supported.
export struct Colr : Io::BChunk {
    static constexpr Str SIG = "COLR";

    using NumBaseGlyphRecords = Io::BField<u16be, 2>;
    using BaseGlyphRecordsOffset = Io::BField<u32be, 4>;
    using LayerRecordsOffset = Io::BField<u32be, 8>;
    using NumLayerRecords = Io::BField<u16be, 12>;

    static constexpr usize BASE_GLYPH_RECORD_SIZE = 6;
    static constexpr usize LAYER_RECORD_SIZE = 4;

    // Palette index meaning "use the current text color".
    static constexpr u16 FOREGROUND_PALETTE_INDEX = 0xFFFF;

    struct Layer {
        u16 glyphId;
        u16 paletteIndex;
    };

    struct Layers {
        usize first;
        usize len;
    };

    Res<Opt<Layers>> layers(usize glyphId) const {
        if (not present())
            return Ok(NONE);

        usize lo = 0;
        usize hi = try$(get<NumBaseGlyphRecords>());
        auto records = try$(begin(try$(get<BaseGlyphRecordsOffset>())));

        while (lo < hi) {
            usize mid = lo + (hi - lo) / 2;
            auto s = try$(records.peek(mid * BASE_GLYPH_RECORD_SIZE));
            usize id = try$(s.nextU16be());

            if (id == glyphId) {
                usize first = try$(s.nextU16be());
                usize len = try$(s.nextU16be());
                if (len == 0 or first + len > try$(get<NumLayerRecords>()))
                    return Ok(NONE);
                return Ok(Some(Layers{first, len}));
            }

            if (id < glyphId)
                lo = mid + 1;
            else
                hi = mid;
        }

        return Ok(NONE);
    }

    Res<Layer> layer(usize index) const {
        auto s = try$(begin(try$(get<LayerRecordsOffset>()) + index * LAYER_RECORD_SIZE));
        auto glyphId = try$(s.nextU16be());
        auto paletteIndex = try$(s.nextU16be());
        return Ok(Layer{glyphId, paletteIndex});
    }

    // Paint each layer of the glyph, contour(g, glyphId) -> Res<> appends the outline of a layer glyph.
    Res<> paint(Gfx::Canvas& g, usize glyphId, Cpal const& cpal, auto contour) const {
        auto maybeLayers = try$(layers(glyphId));
        if (not maybeLayers)
            return Ok();
        auto layers = *maybeLayers;

        for (auto i : urange::zeroTo(layers.len)) {
            auto l = try$(layer(layers.first + i));

            // NOTE: Always use the first palette, and fall back to
            //       the current fill for out-of-range palette entries.
            Opt<Gfx::Color> color = NONE;
            if (l.paletteIndex != FOREGROUND_PALETTE_INDEX)
                color = try$(cpal.color(0, l.paletteIndex));

            g.push();
            g.beginPath();
            auto res = contour(g, l.glyphId);

            if (res) {
                if (color)
                    g.fill(*color);
                else
                    g.fill();
            }
            g.pop();
            try$(res);
        }

        return Ok();
    }
};

} // namespace Karm::Font::Ttf
