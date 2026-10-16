module;

#include <karm/macros>

export module Karm.Png:encoder;

import Karm.Core;
import Karm.Archive;
import Karm.Crypto;
import Karm.Gfx.Pixels;

import :base;

// PNG Image encoder
// https://www.w3.org/TR/2003/REC-PNG-20031110/
namespace Karm::Png {

static Res<> _writeChunk(Io::BEmit& e, Str type, Bytes data) {
    Crypto::Crc32 crc;
    crc.update(bytes(type));
    crc.update(data);

    try$(e.writeU32be(data.len()));
    try$(e.writeStr(type));
    try$(e.writeBytes(data));
    try$(e.writeU32be(crc.digest()));
    return Ok();
}

static Res<> _writeHeader(Io::BEmit& e, Gfx::Pixels pixels) {
    Io::BufferWriter bw;
    Io::BEmit header{bw};
    try$(header.writeU32be(pixels.width()));
    try$(header.writeU32be(pixels.height()));
    try$(header.writeU8be(8)); // Bit depth
    try$(header.writeU8be(toUnderlyingType(ColorType::TRUECOLOR_ALPHA)));
    try$(header.writeU8be(toUnderlyingType(CompressionMethod::DEFLATE)));
    try$(header.writeU8be(toUnderlyingType(FilterMethod::STANDARD)));
    try$(header.writeU8be(toUnderlyingType(InterlacingMethod::NULL)));
    return _writeChunk(e, IHDR, bw.bytes());
}

static Vec<u8> _scanlines(Gfx::Pixels pixels) {
    Vec<u8> scanlines;
    scanlines.ensure(pixels.height() * (1 + pixels.width() * 4));

    for (isize y = 0; y < pixels.height(); y++) {
        scanlines.pushBack(toUnderlyingType(Filter::NONE));
        for (isize x = 0; x < pixels.width(); x++) {
            auto color = pixels.loadUnsafe({x, y});
            scanlines.pushBack(color.red);
            scanlines.pushBack(color.green);
            scanlines.pushBack(color.blue);
            scanlines.pushBack(color.alpha);
        }
    }

    return scanlines;
}

export Res<> encode(Gfx::Pixels pixels, Io::BEmit& e) {
    try$(e.writeBytes(SIG));
    try$(_writeHeader(e, pixels));

    auto data = try$(Archive::zlibCompress(_scanlines(pixels)));
    try$(_writeChunk(e, IDAT, data));

    return _writeChunk(e, IEND, {});
}

} // namespace Karm::Png
