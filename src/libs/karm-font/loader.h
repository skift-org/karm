#pragma once

#include <karm-gfx/font.h>
#include <karm-sys/mmap.h>

namespace Karm::Font {

Res<Rc<Gfx::Fontface>> loadFontface(Sys::Mmap&& map);

Res<Rc<Gfx::Fontface>> loadFontface(Mime::Url url);

Res<Rc<Gfx::Fontface>> loadFontfaceOrFallback(Mime::Url url);

Res<Gfx::Font> loadFont(f64 size, Mime::Url url);

Res<Gfx::Font> loadFontOrFallback(f64 size, Mime::Url url);

} // namespace Karm::Font
