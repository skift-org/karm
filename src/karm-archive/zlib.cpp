module;

#include <karm/macros>

export module Karm.Archive:zlib;

import Karm.Core;
import Karm.Crypto;
import Karm.Debug;
import Karm.Logger;

import :flate;

using namespace Karm::Literals;

namespace Karm::Archive {

static auto debugZlib = Debug::Flag::debug("zlib", "Log zlib decompression"s);

export Res<> zlibDecompress(Io::BitReader& r, Io::Writer& out) {
    u8 cmf = try$(r.readByte());
    logDebugIf(debugZlib, "cmf: {:#02x}", cmf);

    u8 cm = cmf & 0xf;
    if (cm != 8)
        return Error::invalidData("invalid compression method");

    u8 cinfo = (cmf >> 4) & 0xf;
    if (cinfo > 7)
        return Error::invalidData("invalid compression info");

    u8 flg = try$(r.readByte());
    logDebugIf(debugZlib, "flg: {#02x}", flg);

    if ((cmf * 256u + flg) % 31 != 0)
        return Error::invalidData("invalid checksum");

    if ((flg >> 5) & 1)
        return Error::notImplemented("preset dictionary not implemented");

    try$(inflate(r, out));

    u32 adler = try$(r.readBytes<u32>(4));
    logDebugIf(debugZlib, "adler: {:#04x}", adler);
    (void)adler; // TODO: check adler
    return Ok();
}

export Res<> zlibDecompress(Io::Reader& reader, Io::Writer& out) {
    Io::BitReader bits{reader};
    return zlibDecompress(bits, out);
}

export Res<> zlibDecompress(Bytes bytes, Io::Writer& out) {
    Io::BufReader reader{bytes};
    return zlibDecompress(reader, out);
}

export Res<Vec<u8>> zlibDecompress(Bytes bytes) {
    Io::BufferWriter w;
    try$(zlibDecompress(bytes, w));
    return Ok(w.take());
}

export Res<> zlibCompress(Bytes bytes, Io::Writer& out) {
    u8 cmf = 0x78; // cm = 8 (deflate), cinfo = 7 (32K window)
    u8 flg = 0x9c; // flevel = 2, fcheck such that (cmf * 256 + flg) % 31 == 0
    try$(Io::putByte(out, cmf));
    try$(Io::putByte(out, flg));

    try$(deflate(bytes, out));

    u32 adler = Crypto::adler32(bytes);
    try$(Io::putByte(out, adler >> 24));
    try$(Io::putByte(out, adler >> 16));
    try$(Io::putByte(out, adler >> 8));
    try$(Io::putByte(out, adler));

    return Ok();
}

export Res<Vec<u8>> zlibCompress(Bytes bytes) {
    Io::BufferWriter w;
    try$(zlibCompress(bytes, w));
    return Ok(w.take());
}

} // namespace Karm::Archive
