module;

#include "../ttf/fontface.h"

export module Karm.Font:sfnt.serialize;

import Karm.Core;
import Karm.Sys;
import :subset;

namespace Karm::Font::Sfnt {

Res<> serialize(Rc<Ttf::Fontface>, Subset const&, Io::Writer&) {
    return Error::notImplemented();
}

} // namespace Karm::Font::Sfnt
