module;

#include <karm-core/macros.h>

#include "defs.h"

export module Karm.Sys:chan;

import Karm.Core;

import :_embed;
import :fd;

namespace Karm::Sys {

export struct In : Io::Stream {
    Rc<Fd> _fd;

    In(Rc<Fd> fd)
        : _fd(fd) {}

    Res<usize> readAsync(MutBytes bytes) override {
        return _fd->readAsync(bytes);
    }

    Rc<Fd> fd() {
        return _fd;
    }
};

export struct Out : Io::TextWriter, Io::Stream {
    using E = Encoding;
    Rc<Fd> _fd;

    Out(Rc<Fd> fd)
        : _fd(fd) {}

    Res<usize> writeAsync(Bytes bytes) override {
        return _fd->writeAsync(bytes);
    }

    Res<> writeRune(Rune rune) override {
        typename E::One one;
        if (not E::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        try$(writeAsync(bytes(one)));
        return Ok();
    }

    Rc<Fd> fd() {
        return _fd;
    }

    Res<> flushAsync() override {
        return _fd->flushAsync();
    }
};

export struct Err : Io::TextWriter, Io::Stream {
    using E = Encoding;

    Rc<Fd> _fd;

    Err(Rc<Fd> fd)
        : _fd(fd) {}

    Res<usize> writeAsync(Bytes bytes) override {
        return _fd->writeAsync(bytes);
    }

    Res<> writeRune(Rune rune) override {
        typename E::One one;
        if (not E::encodeUnit(rune, one))
            return Error::invalidInput("encoding error");
        try$(writeAsync(bytes(one)));
        return Ok();
    }

    Rc<Fd> fd() {
        return _fd;
    }

    Res<> flushAsync() override {
        return _fd->flushAsync();
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
    (void)out().writeStr(Str{LINE_ENDING});
    (void)out().flushAsync();
}

export void errln(Str str = "", auto&&... args) {
    (void)Io::format(err(), str, std::forward<decltype(args)>(args)...);
    (void)err().writeStr(Str{LINE_ENDING});
    (void)err().flushAsync();
}

} // namespace Karm::Sys
