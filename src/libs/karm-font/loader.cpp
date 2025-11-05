module;

#include "ttf/fontface.h"

export module Karm.Font:loader;

import Karm.Core;
import Karm.Sys;
import Karm.Ref;
import Karm.Gfx;
import :sfnt;
import :woff;

namespace Karm::Font {

export Res<Rc<Ttf::Container>> _loadContainer(Sys::Mmap&& map) {
    if (Sfnt::sniff(map.bytes())) {
        return Sfnt::Container::load(std::move(map));
    } else if (Woff1::sniff(map.bytes())) {
        return Woff1::Container::load(std::move(map));
    } else if (Woff2::sniff(map.bytes())) {
        return Woff2::Container::load(std::move(map));
    } else {
        return Error::invalidData("unknown truetype container");
    }
}

export Res<Rc<Gfx::Fontface>> loadFontface(Sys::Mmap&& map) {
    auto container = try$(_loadContainer(std::move(map)));
    return Ok(try$(Ttf::Fontface::load(container)));
}

export Res<Rc<Gfx::Fontface>> loadFontface(Ref::Url url) {
    auto file = try$(Sys::File::open(url));
    auto map = try$(Sys::mmap(file));
    return loadFontface(std::move(map));
}

export Res<Gfx::Font> loadFont(f64 size, Ref::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontface(url)),
        .fontsize = size,
    });
}

} // namespace Karm::Font
