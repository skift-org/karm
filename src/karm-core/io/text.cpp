module;

#include <karm-core/macros.h>

export module Karm.Core:io.text;

import :base.string;
import :base.vec;
import :io.base;

namespace Karm::Io {

export struct TextWriter {
    template <StaticEncoding E>
    Res<> writeStr(_Str<E> str) {
        for (auto rune : iterRunes(str))
            try$(writeRune(rune));
        return Ok();
    }

    virtual Res<> writeRune(Rune rune) = 0;
};

export template <StaticEncoding E = Utf8>
struct TextEncoder : TextWriter {
    Vec<u8> _buf;

    Res<> writeRune(Rune rune) override {
        typename E::One one;
        if (not E::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        auto b = bytes(one);
        _buf.ensure(_buf.len() + b.len());
        _buf.insertMany(_buf.len(), b);
        return Ok();
    }

    Bytes bytes() const {
        return _buf;
    }

    Vec<u8> take() {
        return std::move(_buf);
    }
};

export template <StaticEncoding E>
struct _StringWriter : TextWriter, _StringBuilder<E> {
    _StringWriter(usize cap = 16) : _StringBuilder<E>(cap) {}

    Res<> writeRune(Rune rune) override {
        _StringBuilder<E>::append(rune);
        return Ok();
    }

    Res<> writeUnit(Slice<typename E::Unit> unit) {
        _StringBuilder<E>::append(unit);
        return Ok();
    }
};

export using StringWriter = _StringWriter<Utf8>;

} // namespace Karm::Io
