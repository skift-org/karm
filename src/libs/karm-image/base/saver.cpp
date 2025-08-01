module;

#include <karm-gfx/buffer.h>
#include <karm-mime/uti.h>
#include <karm-sys/file.h>

export module Karm.Image:base.saver;

import Karm.Core;
import :bmp.encoder;
import :jpeg.encoder;
import :qoi.encoder;
import :tga.encoder;

namespace Karm::Image {

export struct Saver {
    Mime::Uti format = Mime::Uti::PUBLIC_BMP;
    static constexpr f64 MIN_QUALITY = 0.0;
    static constexpr f64 DEFAULT_QUALITY = 0.75;
    static constexpr f64 MAX_QUALITY = 1.0;
    f64 quality = DEFAULT_QUALITY;
};

export Res<> save(Gfx::Pixels pixels, Io::BEmit& e, Saver const& props = {}) {
    if (props.format == Mime::Uti::PUBLIC_BMP) {
        return Bmp::encode(pixels, e);
    } else if (props.format == Mime::Uti::PUBLIC_TGA) {
        return Tga::encode(pixels, e);
    } else if (props.format == Mime::Uti::PUBLIC_JPEG) {
        return Jpeg::encode(pixels, e);
    } else if (props.format == Mime::Uti::PUBLIC_QOI) {
        return Qoi::encode(pixels, e);
    } else {
        return Error::invalidData("unsupported image format");
    }
}

export Res<> save(Gfx::Pixels pixels, Io::Writer& w, Saver const& props = {}) {
    Io::BEmit e{w};
    return save(pixels, e, props);
}

export Res<> save(Gfx::Pixels pixels, Mime::Url const& url, Saver const& props = {}) {
    auto file = try$(Sys::File::create(url));
    Io::BEmit e{file};
    return save(pixels, e, props);
}

} // namespace Karm::Image
