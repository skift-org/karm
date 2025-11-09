export module Karm.Fs:vfs;

import Karm.Core;
import :node;
#include "karm-core/macros.h"

namespace Karm::Fs {

export struct VFile : Node {
    Vec<u8> _buf;
};

// A virtual file backed by a mmap region
export struct VFileMmap : Node {
    Sys::Mmap _mmap;
    usize _len;

    VFileMmap(Sys::Mmap mmap, usize len)
        : _mmap(std::move(mmap)), _len(len) {}

    Async::Task<Sys::Stat> statAsync() override {
        auto stat = co_trya$(Node::statAsync());
        stat.type = Sys::Type::FILE;
        stat.size = _len;
        co_return Ok(stat);
    }

    Async::Task<usize> readAsync(MutBytes buf, usize offset) override {
        auto read = copy(
            sub(
                _mmap.bytes(),
                offset, offset + buf.len()
            ),
            buf
        );
        co_return Ok(read);
    }
};

export struct VDir : Node {
    Map<String, Rc<Node>> _entries;

    Async::Task<> linkAsync(Str name, Rc<Node> node) override {
        if (_entries.has(name))
            co_return Error::alreadyExists();
        _entries.put(name, node);
        co_return Ok();
    }

    Async::Task<Rc<Node>> lookupAsync(Str name) override {
        if (not _entries.has(name))
            co_return Error::notFound();
        co_return Ok(_entries.get(name));
    }

    Async::Task<> unlinkAsync(Str name) override {
        if (not _entries.del(name))
            co_return Error::notFound();
        co_return Ok();
    }

    Async::Task<Rc<Node>> createAsync(Str name, Sys::Type type) override {
        if (type == Sys::Type::DIR) {
            auto node = co_trya$(Fs::createAsync<VDir>());
            co_trya$(linkAsync(name, node));
            co_return Ok(node);
        }
        if (type == Sys::Type::FILE) {
            auto node = co_trya$(Fs::createAsync<VFile>());
            co_trya$(linkAsync(name, node));
            co_return Ok(node);
        }
        co_return Error::invalidInput();
    }
};

} // namespace Karm::Fs