
#include <karm-sys/file.h>
#include <karm-sys/mmap.h>

#include "loader.h"
#include "ttf/fontface.h"

namespace Karm::Font {

Res<Rc<Gfx::Fontface>> loadFontface(Sys::Mmap&& map) {
    return Ok(try$(Ttf::Fontface::load(std::move(map))));
}

Res<Rc<Gfx::Fontface>> loadFontface(Mime::Url url) {
    auto file = try$(Sys::File::open(url));
    auto map = try$(Sys::mmap().map(file));
    return loadFontface(std::move(map));
}

Res<Rc<Gfx::Fontface>> loadFontfaceOrFallback(Mime::Url url) {
    if (auto result = loadFontface(url); result) {
        return result;
    }
    return Ok(Gfx::Fontface::fallback());
}

Res<Gfx::Font> loadFont(f64 size, Mime::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontface(url)),
        .fontsize = size,
    });
}

Res<Gfx::Font> loadFontOrFallback(f64 size, Mime::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontfaceOrFallback(url)),
        .fontsize = size,
    });
}

} // namespace Karm::Font
