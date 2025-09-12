module;

#include <karm-gfx/font.h>

export module Karm.Font:subset;

import Karm.Core;

namespace Karm::Font {

export struct Subset {
    bool all = false;
    Set<Rune> runes;
    Set<Gfx::Glyph> glyphs;
    Ref::Uti output = Ref::Uti::PUBLIC_TTF;

    void addAll() {
        all = true;
    }

    void addRune(Rune rune) {
        runes.put(rune);
    }

    void addGlyph(Gfx::Glyph glyph) {
        glyphs.put(glyph);
    }
};

Res<> subset(Rc<Gfx::Fontface> font, Subset& subset, Io::Writer& out);

} // namespace Karm::Font