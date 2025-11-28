export module Karm.Core:aio.adapt;

import :io.base;
import :aio.traits;

namespace Karm::Aio {

struct Adapter : Stream {
    Io::Stream& _inner;

    Adapter(Io::Stream& inner)
        : _inner(inner) {}

    Async::Task<usize> writeAsync(Bytes buf) override {
        co_return _inner.write(buf);
    }

    Async::Task<usize> readAsync(MutBytes buf) override {
        co_return _inner.read(buf);
    }
};

export Adapter adapt(Io::Stream& t) {
    return {t};
}

} // namespace Karm::Aio
