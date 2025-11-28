module;

#include <karm-core/macros.h>
#include "defs.h"

export module Karm.Sys:chan;

import Karm.Core;

import :_embed;
import :fd;

namespace Karm::Sys {

export struct In : Io::Reader {
    Rc<Fd> _fd;

    In(Rc<Fd> fd)
        : _fd(fd) {}

    Res<usize> read(MutBytes bytes) override {
        return _fd->read(bytes);
    }

    Rc<Fd> fd() {
        return _fd;
    }
};

export struct Out : Io::TextWriter, Io::Writer, Io::Flusher {
    using E = Sys::Encoding;
    Rc<Fd> _fd;

    Out(Rc<Fd> fd)
        : _fd(fd) {}

    Res<usize> write(Bytes bytes) override {
        return _fd->write(bytes);
    }

    Res<> writeRune(Rune rune) override {
        typename E::One one;
        if (not E::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        try$(write(bytes(one)));
        return Ok();
    }

    Rc<Fd> fd() {
        return _fd;
    }

    Res<> flush() override {
        return _fd->flush();
    }
};

export struct Err : Io::TextWriter, Io::Writer, Io::Flusher {
    using E = Sys::Encoding;

    Rc<Fd> _fd;

    Err(Rc<Fd> fd)
        : _fd(fd) {}

    Res<usize> write(Bytes bytes) override {
        return _fd->write(bytes);
    }

    Res<> writeRune(Rune rune) override {
        typename E::One one;
        if (not E::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        try$(write(bytes(one)));
        return Ok();
    }

    Rc<Fd> fd() {
        return _fd;
    }

    Res<> flush() override {
        return _fd->flush();
    }
};

export In& in() {
    static In _in{_Embed::createIn().take()};
    return _in;
}

export Out& out() {
    static Out _out{_Embed::createOut().take()};
    return _out;
}

export Err& err() {
    static Err _err{_Embed::createErr().take()};
    return _err;
}

export void print(Str str = "", auto&&... args) {
    (void)Io::format(out(), str, std::forward<decltype(args)>(args)...);
}

export void err(Str str = "", auto&&... args) {
    (void)Io::format(err(), str, std::forward<decltype(args)>(args)...);
}

export void println(Str str = "", auto&&... args) {
    (void)Io::format(out(), str, std::forward<decltype(args)>(args)...);
    (void)out().writeStr(Str{Sys::LINE_ENDING});
    (void)out().flush();
}

export void errln(Str str = "", auto&&... args) {
    (void)Io::format(err(), str, std::forward<decltype(args)>(args)...);
    (void)err().writeStr(Str{Sys::LINE_ENDING});
    (void)err().flush();
}

} // namespace Karm::Sys
