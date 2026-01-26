module;

#include <karm/macros>

export module Karm.Sys:file;

import Karm.Core;
import Karm.Ref;

import :async;
import :fd;

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
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::createFile(url));
        return Ok<File>(fd, url);
    }

    static Res<File> open(Ref::Url url) {
        if (url.scheme == "data") {
            auto fd = makeRc<BlobFd>(try$(url.blob));
            return Ok<File>(fd, url);
        }

        if (url.scheme != "bundle")
            try$(ensureUnrestricted());

        auto fd = try$(_Embed::openFile(url));
        return Ok<File>(fd, url);
    }

    static Res<File> openOrCreate(Ref::Url url) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::openOrCreateFile(url));
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

    Res<Ref::Mime> sniff(bool ignoreUrl = false) {
        if (not ignoreUrl) {
            if (auto mime = Ref::sniffSuffix(_url.path.suffix()))
                return Ok(mime.take());
        }

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
};

/// Read the entire file as a UTF-8 string.
export Res<String> readAllUtf8(Ref::Url const& url) {
    auto file = try$(Sys::File::open(url));
    return Io::readAllUtf8(file);
}

} // namespace Karm::Sys
