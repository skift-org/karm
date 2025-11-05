module Karm.Gfx;

import Karm.Math;
import Karm.Core;
import :font;
import :canvas;

namespace Karm::Gfx {
    
// MARK: Font Family -----------------------------------------------------------

void FontFamily::contour(Canvas& g, Glyph glyph) const {
    auto& member = _members[glyph.font];
    g.scale(_adjust.sizeAdjust * member.adjust.sizeAdjust);
    member.face->contour(g, glyph);
}

// MARK: Font ------------------------------------------------------------------

void Font::contour(Canvas& g, Glyph glyph) {
    g.scale(fontsize);
    fontface->contour(g, glyph);
}

} // namespace Karm::Gfx
