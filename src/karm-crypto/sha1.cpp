export module Karm.Crypto:sha1;

import Karm.Core;

namespace Karm::Crypto {

export constexpr usize SHA1_BYTES = 20;

static constexpr Array<u32, 5> SHA1_INITIAL = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
};

struct Sha1State {
    static constexpr usize BLOCK_SIZE = 64;

    Array<u32, 5> _state;
    Array<u8, BLOCK_SIZE> _buf{};
    usize _bufLen = 0;
    u64 _totalLen = 0;

    constexpr Sha1State(Array<u32, 5> const& init) : _state(init) {}

    constexpr void _computeBlock(u8 const* buf) {
        Array<u32, 80> w{};

        for (usize idx = 0; idx < 16; idx++) {
            w[idx] = toBe(reinterpret_cast<u32 const*>(buf)[idx]);
        }

        for (usize idx = 16; idx < 80; idx++) {
            w[idx] = rotl(w[idx - 3] ^ w[idx - 8] ^ w[idx - 14] ^ w[idx - 16], 1);
        }

        u32 a = _state[0];
        u32 b = _state[1];
        u32 c = _state[2];
        u32 d = _state[3];
        u32 e = _state[4];

        for (usize idx = 0; idx < 80; idx++) {
            u32 f = 0;
            u32 k = 0;

            if (idx < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5a827999;
            } else if (idx < 40) {
                f = b ^ c ^ d;
                k = 0x6ed9eba1;
            } else if (idx < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8f1bbcdc;
            } else {
                f = b ^ c ^ d;
                k = 0xca62c1d6;
            }

            u32 tmp = rotl(a, 5) + f + e + k + w[idx];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = tmp;
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
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

        _buf[56] = padlen >> 56;
        _buf[57] = padlen >> 48;
        _buf[58] = padlen >> 40;
        _buf[59] = padlen >> 32;
        _buf[60] = padlen >> 24;
        _buf[61] = padlen >> 16;
        _buf[62] = padlen >> 8;
        _buf[63] = padlen;

        _computeBlock(_buf.buf());

        for (usize idx = 0; idx < _state.len(); idx++) {
            _state[idx] = toBe(_state[idx]);
        }
    }
};

export struct Sha1 {
    using Digest = Array<u8, SHA1_BYTES>;
    static constexpr auto BLOCK_SIZE = Sha1State::BLOCK_SIZE;

    Sha1State _state{SHA1_INITIAL};

    constexpr Sha1() = default;

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

export constexpr Sha1::Digest sha1(Bytes bytes) {
    Sha1 sha;
    sha.update(bytes);
    return sha.digest();
}

} // namespace Karm::Crypto