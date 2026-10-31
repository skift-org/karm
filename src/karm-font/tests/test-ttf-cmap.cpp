#include <karm/test>

import Karm.Core;
import Karm.Gfx;
import Karm.Font.Ttf;

namespace Karm::Font::Ttf::Tests {

test$("ttf-cmap-format12-mapping-stays-in-the-bmp") {
    // Format 12 subtable with two BMP groups and one supplementary-plane group whose low 16 bits
    // collide with the BMP group above it (U+262A5 vs U+62A5, as in Noto Sans SC).
    static constexpr Array<u8, 52> TABLE = {
        0x00, 0x0C, 0x00, 0x00,                                                 // format 12, reserved
        0x00, 0x00, 0x00, 0x34,                                                 // length = 52
        0x00, 0x00, 0x00, 0x00,                                                 // language
        0x00, 0x00, 0x00, 0x03,                                                 // numGroups = 3
        0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x01, // U+0041..U+0043 -> 1..3
        0x00, 0x00, 0x62, 0xA5, 0x00, 0x00, 0x62, 0xA5, 0x00, 0x00, 0x00, 0x64, // U+62A5 -> 100
        0x00, 0x02, 0x62, 0xA5, 0x00, 0x02, 0x62, 0xA6, 0x00, 0x00, 0x00, 0xC8, // U+262A5..U+262A6 -> 200..201
    };

    Cmap::Table table{.platformId = 3, .encodingId = 10, .type = 12, .slice = Bytes{TABLE}};

    // Lookups by code point are not limited to the BMP
    assertEq$(table.glyphIdFor(0x62A5).index, (u16)100);
    assertEq$(table.glyphIdFor(0x262A5).index, (u16)200);

    // The code -> glyph mapping used for PDF CIDs is 16-bit: supplementary code points must be left out
    // instead of being truncated into duplicates of BMP codes (which shifted the PDF CIDToGIDMap)
    auto mapping = table.extractMapping();
    assertEq$(mapping.len(), 4uz);
    for (auto [code, glyph] : iter(mapping)) {
        if (code == 0x62A5)
            assertEq$(glyph, (u16)100);
    }

    return Ok();
}

} // namespace Karm::Font::Ttf::Tests
