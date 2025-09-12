module;

#include <karm-sys/mmap.h>

#include "../ttf/fontface.h"

export module Karm.Font:woff.serialize;

import Karm.Core;
import :subset;

namespace Karm::Font::Sfnt {

Res<> serialize(Rc<Ttf::Fontface> font, Subset& subset, Io::Writer& out) {
}

} // namespace Karm::Font::Sfnt