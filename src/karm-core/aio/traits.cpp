export module Karm.Core:aio.traits;

import :async.task;
import :async.cancellation;

namespace Karm::Aio {

template <typename T>
concept AsyncWritable = requires(T& writer, Bytes bytes, Async::CancellationToken ct) {
    { writer.writeAsync(bytes, ct) } -> Meta::Same<Async::Task<usize>>;
};

template <typename T>
concept AsyncReadable = requires(T& reader, MutBytes bytes, Async::CancellationToken ct) {
    { reader.readAsync(bytes, ct) } -> Meta::Same<Async::Task<usize>>;
};

export struct Writer {
    virtual ~Writer() = default;
    virtual Async::Task<usize> writeAsync(Bytes buf, Async::CancellationToken ct) = 0;
};

static_assert(AsyncWritable<Writer>);

export struct Reader {
    virtual ~Reader() = default;
    virtual Async::Task<usize> readAsync(MutBytes buf, Async::CancellationToken ct) = 0;
};

static_assert(AsyncReadable<Reader>);

} // namespace Karm::Aio
