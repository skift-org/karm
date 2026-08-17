module;

#include <karm/macros>

export module Karm.Sys:file;

import Karm.Core;
import Karm.Ref;

import :async;
import :fd;

using namespace Karm::Literals;
using namespace Karm::Fmt::Literals;

namespace Karm::Sys {

export struct File :
    Io::Seeker,
    Io::Flusher,
    Aio::Reader,
    Io::Reader,
    Aio::Writer,
    Io::Writer,
    Meta::NoCopy {

    Rc<Fd> _fd;
    Ref::Url _url;

    File(Rc<Fd> fd, Ref::Url url)
        : _fd(fd), _url(url) {}

    static Res<File> create(Ref::Url url) {
        return openWith(url, {OpenOption::CREATE_NEW, OpenOption::WRITE});
    }

    static Res<File> open(Ref::Url url) {
        return openWith(url, {OpenOption::READ});
    }

    static Res<File> openOrCreate(Ref::Url url) {
        return openWith(url, {OpenOption::CREATE, OpenOption::WRITE, OpenOption::READ});
    }

    static Res<File> openWith(Ref::Url url, Flags<OpenOption> options) {
        if (url.scheme == "data") {
            if (options.any({OpenOption::WRITE, OpenOption::CREATE}))
                return Error::invalidInput("cannot write in data URL.");
            auto fd = makeRc<BlobFd>(try$(url.blob));
            return Ok<File>(fd, url);
        }

        if (url.scheme == "bundle" and options.any({OpenOption::WRITE, OpenOption::CREATE}))
            return Error::invalidInput("cannot write in bundle.");

        Str action =
            options.has({OpenOption::CREATE_NEW})
                ? "create"s
            : options.has({OpenOption::CREATE})
                ? "open or create"s
                : "open"s;

        auto fd = try$(
            _Embed::openFile(url, options)
                .wrapErr("could not {} {}"_f(action, url))
        );

        return Ok<File>(fd, url);
    }

    Res<usize> read(MutBytes bytes) override {
        return _fd->read(bytes);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> readAsync(MutBytes bytes, Async::CancellationToken ct) override {
        return globalSched().readAsync(_fd, bytes, ct);
    }

    Res<usize> write(Bytes bytes) override {
        return _fd->write(bytes);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> writeAsync(Bytes bytes, Async::CancellationToken ct) override {
        return globalSched().writeAsync(_fd, bytes, ct);
    }

    Res<Ref::Uti> sniff() {
        auto old = try$(Io::tell(*this));
        Defer _ = [&] {
            seek(Io::Seek::fromBegin(old)).unwrap();
        };
        try$(seek(Io::Seek::fromBegin(0)));
        auto mime = try$(Ref::sniffReader(*this));
        return Ok(mime);
    }

    Res<usize> seek(Io::Seek seek) override {
        return _fd->seek(seek);
    }

    Res<> truncate(usize size) {
        return _fd->truncate(size);
    }

    Res<> flush() override {
        return _fd->flush();
    }

    [[clang::coro_wrapper]]
    Async::Task<> flushAsync(Async::CancellationToken ct) {
        return globalSched().flushAsync(_fd, ct);
    }

    Ref::Url url() const {
        return _url;
    }

    Res<Stat> stat() {
        return _fd->stat();
    }

    Rc<Fd> fd() {
        return _fd;
    }

    template <StaticEncoding E = Utf8>
    Res<_String<E>> readAllText() {
        return Io::readAllText<E>(*this);
    }

    template <StaticEncoding E = Utf8>
    Res<> writeAllText(Str buf) {
        Io::TextEncoder<E> enc{*this};
        try$(enc.writeStr(buf));
        return Ok();
    }

    Res<Vec<u8>> readAll() {
        Io::BufferWriter bw;
        try$(Io::copy(*this, bw));
        return Ok(bw.take());
    }

    Res<> writeAll(Bytes buf) {
        Io::BufReader br{buf};
        try$(Io::copy(br, *this));
        return Ok();
    }
};

export template <StaticEncoding E = Utf8>
Res<_String<E>> readAllText(Ref::Url const& url) {
    auto file = try$(Sys::File::open(url));
    return file.readAllText<E>();
}

export template <StaticEncoding E = Utf8>
Res<> writeAllText(Ref::Url const& url, Str buf) {
    auto file = try$(Sys::File::openOrCreate(url));
    return file.writeAllText(buf);
}

export Res<Vec<u8>> readAll(Ref::Url const& url) {
    auto file = try$(Sys::File::open(url));
    return file.readAll();
}

export Res<> writeAll(Ref::Url const& url, Bytes buf) {
    auto file = try$(File::openOrCreate(url));
    return file.writeAll(buf);
}

export Res<usize> copy(Ref::Url const& from, Ref::Url const& to) {
    auto fromFile = try$(File::open(from));
    auto toFile = try$(File::create(to));
    return Io::copy(fromFile, toFile);
}

} // namespace Karm::Sys
