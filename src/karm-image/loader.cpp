module;

#include <karm/macros>

export module Karm.Image:loader;

import Karm.Sys;
import Karm.Ref;
import Karm.Gfx.Pixels;

import :codec;

namespace Karm::Image {

export Res<Rc<Gfx::Image>> load(Bytes bytes) {
    for (auto* codec = Codec::first(); codec; codec = codec->next) {
        if (codec->sniff(bytes))
            return codec->decode(bytes);
    }
    return Error::invalidData("unknown image format");
}

export Res<Rc<Gfx::Image>> load(Ref::Url url) {
    if (url.scheme == "data") {
        auto blob = try$(url.blob);
        return load(blob->data);
    }
    auto file = try$(Sys::File::open(url));
    auto map = try$(Sys::mmap(file));
    return load(map.bytes());
}

export Res<Rc<Gfx::Image>> loadOrFallback(Ref::Url url) {
    if (auto result = load(url); result)
        return result;
    return Ok(Gfx::Image::fallback());
}

} // namespace Karm::Image
