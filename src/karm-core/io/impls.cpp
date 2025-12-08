module;

#include <karm-core/macros.h>

export module Karm.Core:io.impls;

import :base.buf;
import :base.res;
import :io.base;
import :io.funcs;

namespace Karm::Io {

export struct Sink : Stream {
    Async::Task<usize> writeAsync(Bytes bytes) override {
        co_return Ok(sizeOf(bytes));
    }

    Async::Task<usize> readAsync(MutBytes) override {
        co_return Ok(0uz);
    }
};

export struct Zero : Stream {
    Async::Task<usize> readAsync(MutBytes bytes) override {
        co_return Ok(zeroFill(bytes));
    }
};

export struct Repeat : Stream {
    u8 _byte{};

    Repeat(u8 byte) : _byte(byte) {}

    Async::Task<usize> readAsync(MutBytes bytes) override {
        co_return Ok(fill(bytes, _byte));
    }
};

export struct Count : Stream {
    Stream& _reader;
    usize _pos{};

    Count(Stream& reader)
        : _reader(reader) {}

    Async::Task<usize> writeAsync(Bytes bytes) override {
        usize written = co_trya$(_reader.writeAsync(bytes));
        _pos += written;
        co_return Ok(written);
    }

    Async::Task<usize> seekAsync(Seek seek) override {
        if (seek != Whence::CURRENT and seek.offset != 0)
            co_return Error::invalidData("can't seek count reader");
        co_return Ok(_pos);
    }
};

struct Limit : Stream {
    Stream& _reader;
    usize _limit{};
    usize _read{};

    Limit(Stream& reader, usize limit)
        : _reader(reader),
          _limit(limit) {}

    Async::Task<usize> readAsync(MutBytes bytes) override {
        usize size = clamp(sizeOf(bytes), 0uz, _limit - _read);
        usize read = co_trya$(_reader.readAsync({bytes.buf(), size}));
        _read += read;
        co_return Ok(read);
    }
};

struct WriterSlice : Stream {
    Stream& _writer;
    usize _start{};
    usize _end{};

    WriterSlice(Stream& writer, usize start, usize end)
        : _writer(writer), _start(start), _end(end) {}

    Async::Task<usize> seekAsync(Seek seek) override {
        usize pos = co_trya$(tellAsync(_writer));
        usize s = co_trya$(sizeAsync(*this));
        pos = co_try$(seek.apply(pos, s));
        pos = clamp(pos, _start, _end);
        co_return co_await _writer.seekAsync(Seek::fromBegin(pos));
    }

    Async::Task<usize> writeAsync(Bytes bytes) override {
        usize pos = co_trya$(tellAsync(_writer));

        if (pos < _start) {
            co_trya$(_writer.seekAsync(Seek::fromBegin(_start)));
        }

        if (pos > _end) {
            co_return Ok(0uz);
        }

        usize size = clamp(sizeOf(bytes), 0uz, _end - pos);
        co_return co_await _writer.writeAsync(sub(bytes, 0, size));
    }
};

Async::Task<WriterSlice> makeSlice(Stream& writer, usize size) {
    auto start = co_trya$(tellAsync(writer));
    auto end = start + size;
    co_return Ok(WriterSlice{writer, start, end});
}

export struct BufReader :
    Stream {

    Bytes _buf{};
    usize _pos{};

    BufReader(Bytes buf) : _buf(buf), _pos(0) {}

    Async::Task<usize> readAsync(MutBytes bytes) override {
        Bytes slice = sub(_buf, _pos, _pos + sizeOf(bytes));
        usize read = copy(slice, bytes);
        _pos += read;
        co_return Ok(read);
    }

    Async::Task<usize> seekAsync(Seek seek) override {
        _pos = co_try$(seek.apply(_pos, sizeOf(_buf)));
        _pos = clamp(_pos, 0uz, sizeOf(_buf));
        co_return Ok(_pos);
    }

    Bytes bytes() const {
        return next(_buf, _pos);
    }

    usize rem() const {
        return sizeOf(_buf) - _pos;
    }
};

export struct BufWriter : Stream {

    MutBytes _buf;
    usize _pos = 0;

    BufWriter(MutBytes buf) : _buf(buf) {}

    Async::Task<usize> seekAsync(Seek seek) override {
        _pos = co_try$(seek.apply(_pos, sizeOf(_buf)));
        _pos = clamp(_pos, 0uz, sizeOf(_buf));
        co_return Ok(_pos);
    }

    Async::Task<usize> writeAsync(Bytes bytes) override {
        MutBytes slice = mutNext(_buf, _pos);
        usize written = copy(bytes, slice);
        _pos += written;
        co_return Ok(written);
    }
};

export struct BufferWriter : Stream {
    Buf<u8> _buf{};

    BufferWriter(usize cap = 16) : _buf(cap) {}

    Async::Task<usize> writeAsync(Bytes bytes) override {
        _buf.insert(COPY, _buf.len(), bytes.buf(), bytes.len());
        co_return Ok(bytes.len());
    }

    Bytes bytes() const {
        return _buf;
    }

    Async::Task<> flushAsync() override {
        _buf.trunc(0);
        co_return Ok();
    }

    Buf<u8> take() {
        return std::move(_buf);
    }

    void clear() {
        _buf.trunc(0);
    }
};

export struct BitReader {
    Stream& _reader;
    u8 _bits{};
    u8 _len{};

    BitReader(Stream& reader)
        : _reader(reader) {
    }

    Async::Task<u8> readBitAsync() {
        if (_len == 0) {
            if (co_trya$(_reader.readAsync(MutBytes{&_bits, 1})) == 0)
                co_return Error::unexpectedEof();
            _len = 8;
        }

        u8 bit = _bits & 1;
        _bits >>= 1;
        _len -= 1;

        co_return Ok(bit);
    }

    template <Meta::Unsigned T>
    Async::Task<T> readBitsAsync(usize n) {
        T bits = 0;
        for (usize i = 0; i < n; i++)
            bits |= co_trya$(readBitAsync()) << i;
        co_return Ok(bits);
    }

    void align() {
        _bits = 0;
        _len = 0;
    }

    Async::Task<u8> readByteAsync() {
        align();
        co_return co_await readBitsAsync<u8>(8);
    }

    template <Meta::Unsigned T>
    Async::Task<T> readBytesAsync(usize n) {
        align();
        T bytes = {};
        for (usize i = 0; i < n; i++)
            bytes |= co_trya$(readByteAsync()) << (8 * i);
        co_return Ok(bytes);
    }
};

} // namespace Karm::Io
