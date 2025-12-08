module;

#include <karm-core/macros.h>

export module Karm.Sys:file;

import Karm.Core;
import Karm.Ref;

import :async;
import :fd;

namespace Karm::Sys {

export struct File :
    Io::Stream,
    Meta::NoCopy {

    Rc<Fd> _fd;
    Ref::Url _url;

    static Res<File> create(Ref::Url url) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::createFile(url));
        return Ok(File{fd, url});
    }

    static Res<File> open(Ref::Url url) {
        if (url.scheme == "data") {
            auto fd = makeRc<BlobFd>(try$(url.blob));
            return Ok(File{fd, url});
        }

        if (url.scheme != "bundle")
            try$(ensureUnrestricted());

        auto fd = try$(_Embed::openFile(url));
        return Ok(File{fd, url});
    }

    static Res<File> openOrCreate(Ref::Url url) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::openOrCreateFile(url));
        return Ok(File{fd, url});
    }

    File(Rc<Fd> fd, Ref::Url url)
        : _fd(fd), _url(url) {}

    [[clang::coro_wrapper]]
    Async::Task<usize> readAsync(MutBytes bytes) override {
        return globalSched().readAsync(_fd, bytes);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> writeAsync(Bytes bytes) override {
        return globalSched().writeAsync(_fd, bytes);
    }

    Async::Task<usize> seekAsync(Io::Seek seek) override {
        return _fd->seekAsync(seek);
    }

    [[clang::coro_wrapper]]
    Async::Task<> flushAsync() override {
        return globalSched().flushAsync(_fd);
    }

    Res<Stat> stat() {
        return _fd->stat();
    }

    Rc<Fd> fd() {
        return _fd;
    }

    Async::Task<Ref::Mime> sniffAsync(bool ignoreUrl = false) {
        if (not ignoreUrl) {
            if (auto mime = Ref::sniffSuffix(_url.path.suffix()))
                co_return Ok(mime.take());
        }

        auto old = co_trya$(Io::tellAsync(*this));
        co_trya$(seekAsync(Io::Seek::fromBegin(0)));
        auto mime = co_try$(Ref::sniff(*this));
        co_trya$(seekAsync(Io::Seek::fromBegin(old)));
        co_return Ok(mime);
    }
};

/// Read the entire file as a UTF-8 string.
export Async::Task<String> readAllUtf8Async(Ref::Url const& url) {
    auto file = co_try$(Sys::File::open(url));
    co_return Io::readAllUtf8Async(file);
}

} // namespace Karm::Sys
