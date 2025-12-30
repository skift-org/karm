module;

#include <karm-core/macros.h>

export module Karm.Image:base.loader;

import Karm.Ref;
import Karm.Sys;

import :decoder;

namespace Karm::Image {

export Res<Rc<Gfx::Surface>> load(Bytes bytes, Ref::Uti format) {
    auto decoder = try$(Decoder::createFrom(bytes, format));

    auto mimeData = Gfx::MimeData{
        .uti = format,
        .buf = Buf<u8>(bytes),
        .colorSpace = decoder->metadata().colorSpace,
        .bitDepth = decoder->metadata().bitDepth,
        .invertedColors = decoder->metadata().invertedColors,
    };

    auto size = decoder->metadata().size;
    auto fmt = Gfx::Fmt::forDecoding(mimeData.colorSpace, mimeData.bitDepth);

    Gfx::FillFunc decodeFunc = [](Opt<Gfx::MimeData> mimeData, Gfx::MutPixels pixels) -> Res<> {
        if (not mimeData) {
            return Error::invalidInput("missing mime data");
        }

        auto decoder = try$(Decoder::createFrom(mimeData->buf, mimeData->uti));
        return decoder->decode(pixels);
    };

    return Ok(Gfx::Surface::allocLazy(std::move(decodeFunc), mimeData, size, fmt));
}

export Res<Rc<Gfx::Surface>> load(Bytes bytes) {
    return load(bytes, try$(Ref::Uti::fromMime(Ref::sniffBytes(bytes))));
}

export Res<Rc<Gfx::Surface>> load(Ref::Url url) {
    if (url.scheme == "data") {
        auto blob = try$(url.blob);
        return load(blob->data);
    }
    auto file = try$(Sys::File::open(url));
    auto map = try$(Sys::mmap(file));
    return load(map.bytes());
}

export Res<Rc<Gfx::Surface>> loadOrFallback(Ref::Url url) {
    if (auto result = load(url); result)
        return result;
    return Ok(Gfx::Surface::fallback());
}

} // namespace Karm::Image
