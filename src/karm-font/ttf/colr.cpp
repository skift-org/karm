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

    Opt<Layers> layers(usize glyphId) const {
        if (not present())
            return NONE;

        usize lo = 0;
        usize hi = get<NumBaseGlyphRecords>();
        auto records = begin().skip(get<BaseGlyphRecordsOffset>());

        while (lo < hi) {
            usize mid = lo + (hi - lo) / 2;
            auto s = records;
            s.skip(mid * BASE_GLYPH_RECORD_SIZE);
            usize id = s.nextU16be();

            if (id == glyphId) {
                usize first = s.nextU16be();
                usize len = s.nextU16be();
                if (len == 0 or first + len > get<NumLayerRecords>())
                    return NONE;
                return Some(Layers{first, len});
            }

            if (id < glyphId)
                lo = mid + 1;
            else
                hi = mid;
        }

        return NONE;
    }

    Layer layer(usize index) const {
        auto s = begin().skip(get<LayerRecordsOffset>() + index * LAYER_RECORD_SIZE);
        auto glyphId = s.nextU16be();
        auto paletteIndex = s.nextU16be();
        return {glyphId, paletteIndex};
    }

    // Paint each layer of the glyph, contour(g, glyphId) appends the outline of a layer glyph.
    void paint(Gfx::Canvas& g, usize glyphId, Cpal const& cpal, auto contour) const {
        auto maybeLayers = layers(glyphId);
        if (not maybeLayers)
            return;
        auto layers = *maybeLayers;

        for (auto i : urange::zeroTo(layers.len)) {
            auto l = layer(layers.first + i);

            g.push();
            g.beginPath();
            contour(g, l.glyphId);

            // NOTE: Always use the first palette, and fall back to
            //       the current fill for out-of-range palette entries.
            Opt<Gfx::Color> color = NONE;
            if (l.paletteIndex != FOREGROUND_PALETTE_INDEX)
                color = cpal.color(0, l.paletteIndex);

            if (color)
                g.fill(*color);
            else
                g.fill();
            g.pop();
        }
    }
};

} // namespace Karm::Font::Ttf
