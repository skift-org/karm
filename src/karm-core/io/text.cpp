module;

#include <karm-core/macros.h>

export module Karm.Core:io.text;

import :base.string;
import :io.traits;

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
    Io::Writer& _writer;

    TextEncoder(Io::Writer& writer)
        : _writer(writer) {}

    Res<> writeRune(Rune rune) override {
        typename E::One one;
        if (not E::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        try$(_writer.write(bytes(one)));
        return Ok();
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
