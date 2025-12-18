export module Karm.Image:bmp.encoder;

import Karm.Core;
import Karm.Gfx;

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

    e.writeStr("BM"s);
    e.writeU32le(fileSize);    // file size
    e.writeU32le(0);           // reserved
    e.writeU32le(pixelOffset); // pixel offset

    e.writeU32le(infoSize);        // info size
    e.writeI32le(pixels.width());  // width
    e.writeI32le(pixels.height()); // height
    e.writeU16le(1);               // planes
    e.writeU16le(32);              // bpp
    e.writeU32le(0);               // compression
    e.writeU32le(0);               // image size
    e.writeI32le(2835);            // x pixels per meter
    e.writeI32le(2835);            // y pixels per meter
    e.writeU32le(0);               // colors used
    e.writeU32le(0);               // important colors

    e.writeBytes(pixelData);

    return Ok();
}

} // namespace Karm::Image::Bmp
