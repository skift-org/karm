module;

#include <karm/macros>

export module Karm.Image:bmp.encoder;

import Karm.Core;
import Karm.Gfx;

using namespace Karm::Literals;

namespace Karm::Image::Bmp {

namespace {

Vec<u8> _encodePixelData(Gfx::Pixels pixels) {
    Vec<u8> data;
    data.resize(pixels.height() * pixels.width() * 4);
    usize i = 0;
    for (isize y = pixels.height() - 1; y >= 0; --y) {
        for (isize x = 0; x < pixels.width(); ++x) {
            // NOTE: In this code loadUnsafe is safe because we are iterating
            //       over the valid range of pixels.
            auto color = pixels.loadUnsafe({x, y});

            data[i++] = color.blue;
            data[i++] = color.green;
            data[i++] = color.red;
            data[i++] = color.alpha;
        }
    }
    return data;
}

} // namespace

export Res<> encode(Gfx::Pixels pixels, Io::BEmit& e) {
    if (pixels.width() < 0 or pixels.height() < 0)
        return Error::invalidData("negative dimensions");

    if (pixels.width() > Limits<i32>::MAX or pixels.height() > Limits<i32>::MAX)
        return Error::invalidData("dimensions too large");

    usize headerSize = 14;
    usize infoSize = 40;
    usize pixelOffset = headerSize + infoSize;

    auto pixelData = _encodePixelData(pixels);

    usize fileSize = pixelOffset + pixelData.len();

    try$(e.writeStr("BM"s));
    try$(e.writeU32le(fileSize));    // file size
    try$(e.writeU32le(0));           // reserved
    try$(e.writeU32le(pixelOffset)); // pixel offset

    try$(e.writeU32le(infoSize));        // info size
    try$(e.writeI32le(pixels.width()));  // width
    try$(e.writeI32le(pixels.height())); // height
    try$(e.writeU16le(1));               // planes
    try$(e.writeU16le(32));              // bpp
    try$(e.writeU32le(0));               // compression
    try$(e.writeU32le(0));               // image size
    try$(e.writeI32le(2835));            // x pixels per meter
    try$(e.writeI32le(2835));            // y pixels per meter
    try$(e.writeU32le(0));               // colors used
    try$(e.writeU32le(0));               // important colors

    try$(e.writeBytes(pixelData));

    return Ok();
}

} // namespace Karm::Image::Bmp
