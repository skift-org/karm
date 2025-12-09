module;

#include <karm-core/macros.h>

export module Karm.Core:aio.funcs;

import :base.vec;
import :base.ring;
import :io.text;
import :io.impls;
import :aio.traits;
import :aio.adapt;

namespace Karm::Aio {

export Async::Task<usize> copyAsync(AsyncReadable auto& reader, AsyncWritable auto& writer) {
    Array<u8, 4096> buffer = {};
    usize result = 0;
    while (true) {
        auto read = co_trya$(reader.readAsync(mutBytes(buffer)));
        if (read == 0)
            co_return Ok(result);

        result += read;

        auto written = co_trya$(writer.writeAsync(sub(buffer, 0, read)));
        if (written != read)
            co_return Ok(result);
    }
}

export Async::Task<Vec<u8>> readAllAsync(AsyncReadable auto& reader) {
    Io::BufferWriter buf;
    auto bufAsync = adapt(buf);
    co_trya$(Aio::copyAsync(reader, bufAsync));
    co_return Ok(buf.take());
}

export Async::Task<String> readAllUtf8Async(AsyncReadable auto& reader) {
    Io::StringWriter w;
    Array<Utf8::Unit, 512> buf = {};
    while (true) {
        usize read = co_trya$(reader.readAsync(buf.mutBytes()));
        if (read == 0)
            break;
        co_try$(w.writeUnit(sub(buf, 0, read)));
    }
    co_return Ok(w.take());
}

export Async::Task<> drainAsync(AsyncReadable auto& reader) {
    Array<Utf8::Unit, 512> buf = {};
    while (true) {
        usize read = co_trya$(reader.readAsync(buf.mutBytes()));
        if (read == 0)
            break;
    }
    co_return Ok();
}

export Async::Task<Tuple<usize, bool>> readLineAsync(AsyncReadable auto& reader, AsyncWritable auto& writer, Bytes delim) {
    if (delim.len() > 16)
        panic("delimiter string too large");

    u8 b;
    usize result = 0;
    Ring<u8> lastBytes{delim.len()};

    while (true) {
        auto read = co_trya$(reader.readAsync({&b, 1}));

        if (read == 0)
            co_return Ok(Tuple{result, false});

        result += read;

        if (lastBytes.rem() == 0)
            lastBytes.popFront();
        lastBytes.pushBack(b);

        auto written = co_trya$(writer.writeAsync({&b, 1}));
        if (written != read)
            co_return Error::writeZero();

        if (lastBytes == delim) {
            result -= delim.len();
            break;
        }
    }

    co_return Ok(Tuple<usize, bool>{result, true});
}

} // namespace Karm::Aio
