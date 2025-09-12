module Karm.Font;

import :sfnt;

namespace Karm::Font {

Res<> subset(Rc<Gfx::Fontface> font, Subset& subset, Io::Writer& out) {
    if (subset.output == Ref::Uti::PUBLIC_TTF) {

    } else {
        return Error::invalidInput("un supported font conatiner");
    }
}

} // namespace Karm::Font