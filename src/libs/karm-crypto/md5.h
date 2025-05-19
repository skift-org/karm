#pragma once

#include <karm-base/array.h>
#include <karm-base/slice.h>

namespace Karm::Crypto {

static constexpr usize MD5_BYTES = 16;

struct Md5 {
    using Digest = Array<u8, MD5_BYTES>;

    Array<u32, 4> _state;
    Array<u8, 64> _buffer;
    u64 _bitlen;
    u32 _datalen;

    Md5();

    void update(Bytes bytes);
    void finalize();
    Digest digest();

private:
    void transform();
};

Array<u8, MD5_BYTES> md5(Bytes bytes);

} // namespace Karm::Crypto