module;

#include <karm/macros>

export module Karm.Core:aio.funcs;

import :base.vec;
import :base.ring;
import :io.text;
import :io.impls;
import :aio.traits;
import :aio.adapt;

namespace Karm::Aio {

export Async::Task<usize> copyAsync(AsyncReadable auto& reader, AsyncWritable auto& writer, Async::CancellationToken ct) {
    Array<u8, Io::DEFAULT_BUFFER_SIZE> buffer = {};
    usize result = 0;
    while (true) {
        auto read = co_trya$(reader.readAsync(mutBytes(buffer), ct));
        if (read == 0)
            co_return Ok(result);

        result += read;

        auto written = co_trya$(writer.writeAsync(sub(buffer, 0, read), ct));
        if (written != read)
            co_return Ok(result);
    }
}

export Async::Task<Vec<u8>> readAllAsync(AsyncReadable auto& reader, Async::CancellationToken ct) {
    Io::BufferWriter buf;
    auto bufAsync = adapt(buf);
    co_trya$(Aio::copyAsync(reader, bufAsync, ct));
    co_return Ok(buf.take());
}

export Async::Task<String> readAllUtf8Async(AsyncReadable auto& reader, Async::CancellationToken ct) {
    Io::StringWriter w;
    Array<Utf8::Unit, 512> buf = {};
    while (true) {
        usize read = co_trya$(reader.readAsync(buf.mutBytes(), ct));
        if (read == 0)
            break;
        co_try$(w.writeUnit(sub(buf, 0, read)));
    }
    co_return Ok(w.take());
}

export Async::Task<> drainAsync(AsyncReadable auto& reader, Async::CancellationToken ct) {
    Array<Utf8::Unit, 512> buf = {};
    while (true) {
        usize read = co_trya$(reader.readAsync(buf.mutBytes(), ct));
        if (read == 0)
            break;
    }
    co_return Ok();
}

export Async::Task<Tuple<usize, bool>> readLineAsync(AsyncReadable auto& reader, AsyncWritable auto& writer, Bytes delim, Async::CancellationToken ct) {
    if (delim.len() > 16)
        panic("delimiter string too large");

    u8 b;
    usize result = 0;
    Ring<u8> lastBytes{delim.len()};

    while (true) {
        auto read = co_trya$(reader.readAsync({&b, 1}, ct));

        if (read == 0)
            co_return Ok(Tuple{result, false});

        result += read;

        if (lastBytes.rem() == 0)
            lastBytes.popFront();
        lastBytes.pushBack(b);

        auto written = co_trya$(writer.writeAsync({&b, 1}, ct));
        if (written != read)
            co_return Error::writeZero();

        if (lastBytes == delim) {
            result -= delim.len();
            break;
        }
    }

    co_return Ok(Tuple{result, true});
}

} // namespace Karm::Aio
