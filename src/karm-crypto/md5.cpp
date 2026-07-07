export module Karm.Crypto:md5;

import Karm.Core;

namespace Karm::Crypto {

export constexpr usize MD5_BYTES = 16;

static constexpr Array<u32, 4> MD5_INITIAL = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476
};

// K[i] = floor(abs(sin(i + 1)) * 2^32), i = 0..63
static constexpr Array<u32, 64> MD5_K = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static constexpr Array<u32, 64> MD5_S = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

struct Md5State {
    static constexpr usize BLOCK_SIZE = 64;

    Array<u32, 4> _state;
    Array<u8, BLOCK_SIZE> _buf{};
    usize _bufLen = 0;
    u64 _totalLen = 0;

    constexpr Md5State(Array<u32, 4> const& init) : _state(init) {}

    constexpr void _computeBlock(u8 const* buf) {
        Array<u32, 16> w{};

        for (usize idx = 0; idx < 16; idx++) {
            w[idx] = toLe(reinterpret_cast<u32 const*>(buf)[idx]);
        }

        u32 a = _state[0];
        u32 b = _state[1];
        u32 c = _state[2];
        u32 d = _state[3];

        for (usize idx = 0; idx < 64; idx++) {
            u32 f = 0;
            usize g = 0;

            if (idx < 16) {
                f = (b & c) | ((~b) & d);
                g = idx;
            } else if (idx < 32) {
                f = (d & b) | ((~d) & c);
                g = (5 * idx + 1) % 16;
            } else if (idx < 48) {
                f = b ^ c ^ d;
                g = (3 * idx + 5) % 16;
            } else {
                f = c ^ (b | (~d));
                g = (7 * idx) % 16;
            }

            f = f + a + MD5_K[idx] + w[g];
            a = d;
            d = c;
            c = b;
            b = b + rotl(f, MD5_S[idx]);
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
    }

    constexpr void update(u8 byte) {
        _buf[_bufLen++] = byte;
        if (_bufLen == 64) {
            _computeBlock(_buf.buf());
            _bufLen = 0;
            _totalLen += 64;
        }
    }

    constexpr void update(Bytes bytes) {
        auto [buf, len] = bytes;
        for (usize i = 0; i < len; ++i) {
            update(buf[i]);
        }
    }

    constexpr void finalize() {
        _totalLen += _bufLen;
        u64 padlen = _totalLen << 3;

        _buf[_bufLen++] = 0x80;

        if (_bufLen > 56) {
            while (_bufLen < 64) {
                _buf[_bufLen++] = 0;
            }
            _computeBlock(_buf.buf());
            _bufLen = 0;
        }

        while (_bufLen < 56) {
            _buf[_bufLen++] = 0;
        }

        _buf[56] = padlen;
        _buf[57] = padlen >> 8;
        _buf[58] = padlen >> 16;
        _buf[59] = padlen >> 24;
        _buf[60] = padlen >> 32;
        _buf[61] = padlen >> 40;
        _buf[62] = padlen >> 48;
        _buf[63] = padlen >> 56;

        _computeBlock(_buf.buf());

        for (usize idx = 0; idx < _state.len(); idx++) {
            _state[idx] = toLe(_state[idx]);
        }
    }
};

export struct Md5 {
    using Digest = Array<u8, MD5_BYTES>;
    static constexpr auto BLOCK_SIZE = Md5State::BLOCK_SIZE;

    Md5State _state{MD5_INITIAL};

    constexpr Md5() = default;

    constexpr void update(u8 byte) {
        _state.update(byte);
    }

    constexpr void update(Bytes bytes) {
        _state.update(bytes);
    }

    constexpr Digest digest() {
        _state.finalize();
        return Digest::from(_state._state.bytes());
    }
};

export constexpr Md5::Digest md5(Bytes bytes) {
    Md5 md;
    md.update(bytes);
    return md.digest();
}

} // namespace Karm::Crypto