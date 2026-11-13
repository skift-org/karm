module;

#include <karm/macros>

export module Karm.Font:loader;

import Karm.Core;
import Karm.Font.Ttf;
import Karm.Font.Woff;
import Karm.Gfx;
import Karm.Ref;
import Karm.Sys;

namespace Karm::Font {

static Res<Opt<Vec<u8>>> _decodeContainer(Bytes bytes) {
    if (Woff::sniffWoff1(bytes))
        return Ok(Some(try$(Woff::decodeWoff1(bytes))));

    if (Woff::sniffWoff2(bytes))
        return Ok(Some(try$(Woff::decodeWoff2(bytes))));

    return Ok(NONE);
}

export Res<Rc<Gfx::Fontface>> loadFontface(Sys::Mmap&& map) {
    if (auto decoded = try$(_decodeContainer(map.bytes())))
        return Ok(try$(Ttf::Fontface::load(decoded.take())));
    return Ok(try$(Ttf::Fontface::load(std::move(map))));
}

export Res<Rc<Gfx::Fontface>> loadFontface(Vec<u8>&& buf) {
    if (auto decoded = try$(_decodeContainer(buf)))
        return Ok(try$(Ttf::Fontface::load(decoded.take())));
    return Ok(try$(Ttf::Fontface::load(std::move(buf))));
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
