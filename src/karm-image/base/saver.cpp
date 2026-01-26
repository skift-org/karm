module;

#include <karm/macros>

export module Karm.Image:base.saver;

import Karm.Math;
import Karm.Core;
import Karm.Ref;
import Karm.Sys;
import Karm.Scene;
import Karm.Gfx;

import :bmp.encoder;
import :jpeg.encoder;
import :qoi.encoder;
import :tga.encoder;

namespace Karm::Image {

export struct Saver {
    /// Image format to save as
    Ref::Uti format = Ref::Uti::PUBLIC_BMP;

    /// Quality for lossy formats (JPEG)
    f64 quality = DEFAULT_QUALITY;
    static constexpr f64 MIN_QUALITY = 0.0;
    static constexpr f64 DEFAULT_QUALITY = 0.75;
    static constexpr f64 MAX_QUALITY = 1.0;

    f64 density = 1; /// For saving scenes as raster images
};

export Res<> save(Gfx::Pixels pixels, Io::BEmit& e, Saver const& props = {}) {
    if (props.format == Ref::Uti::PUBLIC_BMP) {
        return Bmp::encode(pixels, e);
    } else if (props.format == Ref::Uti::PUBLIC_TGA) {
        return Tga::encode(pixels, e);
    } else if (props.format == Ref::Uti::PUBLIC_JPEG) {
        return Jpeg::encode(pixels, e);
    } else if (props.format == Ref::Uti::PUBLIC_QOI) {
        return Qoi::encode(pixels, e);
    } else {
        return Error::invalidData("unsupported image format");
    }
}

export Res<> save(Gfx::Pixels pixels, Io::Writer& w, Saver const& props = {}) {
    Io::BEmit e{w};
    return save(pixels, e, props);
}

export Res<Vec<u8>> save(Gfx::Pixels pixels, Saver const& props = {}) {
    Io::BufferWriter bw;
    try$(save(pixels, bw, props));
    return Ok(bw.take());
}

export Res<> save(Gfx::Pixels pixels, Ref::Url const& url, Saver const& props = {}) {
    auto file = try$(Sys::File::create(url));
    Io::BEmit e{file};
    return save(pixels, e, props);
}

export Res<Vec<u8>> save(Rc<Scene::Node> scene, Math::Vec2i size, Saver const& props = {}) {
    if (props.format == Ref::Uti::PUBLIC_SVG) {
        return Ok(bytes(scene->svg(size)));
    } else {
        auto surface = scene->snapshot(size, props.density);
        return Image::save(*surface, props);
    }
}

} // namespace Karm::Image
