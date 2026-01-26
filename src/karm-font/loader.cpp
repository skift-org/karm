module;

#include <karm/macros>

export module Karm.Font:loader;

import Karm.Core;
import Karm.Font.Ttf;
import Karm.Gfx;
import Karm.Ref;
import Karm.Sys;

namespace Karm::Font {

export Res<Rc<Gfx::Fontface>> loadFontface(Sys::Mmap&& map) {
    return Ok(try$(Ttf::Fontface::load(std::move(map))));
}

export Res<Rc<Gfx::Fontface>> loadFontface(Ref::Url url) {
    auto file = try$(Sys::File::open(url));
    auto map = try$(Sys::mmap(file));
    return loadFontface(std::move(map));
}

export Res<Rc<Gfx::Fontface>> loadFontfaceOrFallback(Ref::Url url) {
    if (auto result = loadFontface(url); result) {
        return result;
    }
    return Ok(Gfx::Fontface::fallback());
}

export Res<Gfx::Font> loadFont(f64 size, Ref::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontface(url)),
        .fontsize = size,
    });
}

export Res<Gfx::Font> loadFontOrFallback(f64 size, Ref::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontfaceOrFallback(url)),
        .fontsize = size,
    });
}

} // namespace Karm::Font
