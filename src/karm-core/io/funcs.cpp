module;

#include <karm-core/macros.h>

export module Karm.Core:io.funcs;

import :base.ring;
import :io.text;
import :async.task;

namespace Karm::Io {

export Async::Task<usize> copyAsync(Stream& reader, Stream& writer, usize size);

// MARK: Read ------------------------------------------------------------------

export Async::Task<usize> preadAsync(Stream& reader, MutBytes bytes, Seek seek) {
    co_trya$(reader.seekAsync(seek));
    co_return co_await reader.readAsync(bytes);
}

export Async::Task<u8> getByteAsync(Stream& reader) {
    u8 byte;
    co_trya$(reader.readAsync({&byte, 1}));
    co_return Ok(byte);
}

export Async::Task<String> readAllUtf8Async(Stream& reader) {
    StringWriter writer;
    Array<Utf8::Unit, 512> buf;
    while (true) {
        usize read = co_trya$(reader.readAsync(buf.mutBytes()));
        if (read == 0)
            break;
        co_try$(writer.writeUnit({buf.buf(), read}));
    }
    co_return Ok(writer.take());
}

// MARK: Write -----------------------------------------------------------------

export Async::Task<usize> pwriteAsync(Stream& writer, Bytes bytes, Seek seek) {
    co_trya$(writer.seekAsync(seek));
    co_return co_await writer.writeAsync(bytes);
}

export [[clang::coro_wrapper]] Async::Task<usize> putByteAsync(Stream& writer, u8 byte) {
    return writer.writeAsync({&byte, 1});
}

// MARK: Seek ------------------------------------------------------------------

export [[clang::coro_wrapper]] Async::Task<usize> tellAsync(Stream& seeker) {
    return seeker.seekAsync(Seek::fromCurrent(0));
}

export Async::Task<usize> sizeAsync(Stream& seeker) {
    usize current = co_trya$(tellAsync(seeker));
    usize end = co_trya$(seeker.seekAsync(Seek::fromEnd(0)));
    co_trya$(seeker.seekAsync(Seek::fromBegin(current)));
    co_return Ok(end);
}

// MARK: Copy ------------------------------------------------------------------

export Async::Task<usize> copyAsync(Stream& reader, MutBytes bytes) {
    usize readed = 0;
    while (readed < bytes.len()) {
        readed += co_trya$(reader.readAsync(mutNext(bytes, readed)));
    }
    co_return Ok(readed);
}

export Async::Task<usize> copyAsync(Stream& reader, Stream& writer) {
    Array<u8, 4096> buffer;
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

export Async::Task<usize> copyAsync(Stream& reader, Stream& writer, usize size) {
    Array<u8, 4096> buf;
    usize result = 0;
    while (size > 0) {
        auto read = co_trya$(reader.readAsync(mutSub(buf, 0, size)));
        if (read == 0)
            co_return Ok(result);

        result += read;

        auto written = co_trya$(writer.writeAsync(sub(buf, 0, read)));
        if (written != read)
            co_return Ok(result);

        size -= read;
    }
    co_return Ok(result);
}

export Async::Task<Tuple<usize, bool>> readLineAsync(Stream& reader, Stream& writer, Bytes delim) {
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
    co_return Ok(Tuple{result, true});
}

} // namespace Karm::Io
