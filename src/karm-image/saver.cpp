module;

#include <karm/macros>

export module Karm.Image:saver;

import Karm.Math;
import Karm.Core;
import Karm.Ref;
import Karm.Sys;
import Karm.Gfx.Pixels;

import :codec;

namespace Karm::Image {

export Res<> save(Gfx::Pixels pixels, Io::BEmit& e, Saver const& props = {}) {
    for (auto* codec = Codec::first(); codec; codec = codec->next) {
        if (codec->encode and props.format == codec->uti)
            return codec->encode(pixels, e, props);
    }
    return Error::invalidData("unsupported image format");
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

} // namespace Karm::Image
