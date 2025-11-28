module;

#include <karm-core/macros.h>

export module Karm.Core:io.funcs;

import :base.ring;
import :io.text;

namespace Karm::Io {

export Res<usize> copy(Stream& reader, Stream& writer, usize size);

// MARK: Read ------------------------------------------------------------------

export Res<usize> pread(Stream& reader, MutBytes bytes, Seek seek) {
    try$(reader.seek(seek));
    return reader.read(bytes);
}

export Res<u8> getByte(Stream& reader) {
    u8 byte;
    try$(reader.read({&byte, 1}));
    return Ok(byte);
}

export Res<String> readAllUtf8(Stream& reader) {
    StringWriter writer;
    Array<Utf8::Unit, 512> buf;
    while (true) {
        usize read = try$(reader.read(buf.mutBytes()));
        if (read == 0) {
            break;
        }
        try$(writer.writeUnit({buf.buf(), read}));
    }
    return Ok(writer.take());
}

// MARK: Write -----------------------------------------------------------------

export Res<usize> pwrite(Stream& writer, Bytes bytes, Seek seek) {
    try$(writer.seek(seek));
    return writer.write(bytes);
}

export Res<usize> putByte(Stream& writer, u8 byte) {
    return writer.write({&byte, 1});
}

// MARK: Seek ------------------------------------------------------------------

export Res<usize> tell(Stream& seeker) {
    return seeker.seek(Seek::fromCurrent(0));
}

export Res<usize> size(Stream& seeker) {
    usize current = try$(tell(seeker));
    usize end = try$(seeker.seek(Seek::fromEnd(0)));
    try$(seeker.seek(Seek::fromBegin(current)));
    return Ok(end);
}

// MARK: Copy ------------------------------------------------------------------

export Res<usize> copy(Stream& reader, MutBytes bytes) {
    usize readed = 0;
    while (readed < bytes.len()) {
        readed += try$(reader.read(mutNext(bytes, readed)));
    }
    return Ok(readed);
}

export Res<usize> copy(Stream& reader, Stream& writer) {
    Array<u8, 4096> buffer;
    usize result = 0;
    while (true) {
        auto read = try$(reader.read(mutBytes(buffer)));
        if (read == 0)
            return Ok(result);

        result += read;

        auto written = try$(writer.write(sub(buffer, 0, read)));
        if (written != read)
            return Ok(result);
    }
}

export Res<usize> copy(Stream& reader, Stream& writer, usize size) {
    Array<u8, 4096> buf;
    usize result = 0;
    while (size > 0) {
        auto read = try$(reader.read(mutSub(buf, 0, size)));
        if (read == 0)
            return Ok(result);

        result += read;

        auto written = try$(writer.write(sub(buf, 0, read)));
        if (written != read)
            return Ok(result);

        size -= read;
    }
    return Ok(result);
}

export Res<Tuple<usize, bool>> readLine(Stream& reader, Stream& writer, Bytes delim) {
    if (delim.len() > 16)
        panic("delimiter string too large");

    u8 b;
    usize result = 0;
    Ring<u8> lastBytes{delim.len()};

    while (true) {
        auto read = try$(reader.read({&b, 1}));

        if (read == 0)
            return Ok(Tuple<usize, bool>{result, false});

        result += read;

        if (lastBytes.rem() == 0)
            lastBytes.popFront();
        lastBytes.pushBack(b);

        auto written = try$(writer.write({&b, 1}));
        if (written != read)
            return Error::writeZero();

        if (lastBytes == delim) {
            result -= delim.len();
            break;
        }
    }
    return Ok(Tuple<usize, bool>{result, true});
}

} // namespace Karm::Io
