module;

#include <karm-core/macros.h>

export module Karm.Crypto:adler32;

import Karm.Core;

namespace Karm::Crypto {

static constexpr usize ADLER32_BASE = 65521;

export struct Adler32 {
    u32 a = 1, b = 0;

    void update(Bytes buf) {
        for (usize i = 0; i < buf.len(); ++i) {
            a = (a + buf[i]) % ADLER32_BASE;
            b = (b + a) % ADLER32_BASE;
        }
    }

    u32 digest() const {
        return (b << 16) | a;
    }
};

export struct Adler32Reader : Io::Reader {
    Io::Reader& _reader;
    Adler32 _adler;

    Adler32Reader(Io::Reader& reader)
        : _reader(reader) {}

    Res<usize> read(MutBytes bytes) override {
        usize n = try$(_reader.read(bytes));
        _adler.update(sub(bytes, 0, n));
        return Ok(n);
    }

    u32 digest() const {
        return _adler.digest();
    }
};

export struct Adler32Writer : Io::Writer {
    Io::Writer& _writer;
    Adler32 _adler;

    Adler32Writer(Io::Writer& writer)
        : _writer(writer) {}

    Res<usize> write(Bytes bytes) override {
        usize n = try$(_writer.write(bytes));
        _adler.update(sub(bytes, 0, n));
        return Ok(n);
    }

    u32 digest() const {
        return _adler.digest();
    }
};

export u32 adler32(Bytes bytes) {
    Adler32 adler;
    adler.update(bytes);
    return adler.digest();
}

} // namespace Karm::Crypto
