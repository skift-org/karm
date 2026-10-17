export module Karm.Crypto:blake2;

import Karm.Core;

namespace Karm::Crypto {

export constexpr usize BLAKE2B_BLOCK_BYTES = 128;
export constexpr usize BLAKE2B_MAX_BYTES = 64;
export constexpr usize BLAKE2B_MAX_KEY_BYTES = 64;

export constexpr usize BLAKE2B256_BYTES = 32;
export constexpr usize BLAKE2B384_BYTES = 48;
export constexpr usize BLAKE2B512_BYTES = 64;

static constexpr Array<u64, 8> BLAKE2B_IV = {
    0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
    0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
    0x510e527fade682d1, 0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
};

static constexpr usize BLAKE2B_ROUNDS = 12;

static constexpr Array<u8, 16 * 10> BLAKE2B_SIGMA = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3,
    11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4,
    7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8,
    9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13,
    2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9,
    12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11,
    13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10,
    6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5,
    10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0
};

struct Blake2bState {
    static constexpr usize BLOCK_SIZE = BLAKE2B_BLOCK_BYTES;

    Array<u64, 8> _state;
    Array<u8, BLOCK_SIZE> _buf{};
    usize _bufLen = 0;
    u64 _counterLo = 0;
    u64 _counterHi = 0;

    constexpr Blake2bState(usize digestSize, Bytes key) : _state(BLAKE2B_IV) {
        auto [keyBuf, keyLen] = key;

        if (keyLen > BLAKE2B_MAX_KEY_BYTES)
            panic("blake2b: key longer than 64 bytes");

        _state[0] ^= 0x01010000 ^ (keyLen << 8) ^ digestSize;

        if (keyLen > 0) {
            for (usize idx = 0; idx < keyLen; idx++)
                _buf[idx] = keyBuf[idx];
            _bufLen = BLOCK_SIZE;
        }
    }

    static constexpr u64 _load64(u8 const* buf) {
        u64 res = 0;
        for (usize idx = 0; idx < 8; idx++)
            res |= static_cast<u64>(buf[idx]) << (8 * idx);
        return res;
    }

    static constexpr void _mix(Array<u64, 16>& v, usize a, usize b, usize c, usize d, u64 x, u64 y) {
        v[a] = v[a] + v[b] + x;
        v[d] = rotr(v[d] ^ v[a], 32);
        v[c] = v[c] + v[d];
        v[b] = rotr(v[b] ^ v[c], 24);
        v[a] = v[a] + v[b] + y;
        v[d] = rotr(v[d] ^ v[a], 16);
        v[c] = v[c] + v[d];
        v[b] = rotr(v[b] ^ v[c], 63);
    }

    constexpr void _incrementCounter(u64 len) {
        _counterLo += len;
        if (_counterLo < len)
            _counterHi++;
    }

    constexpr void _compress(bool last) {
        Array<u64, 16> m{};
        for (usize idx = 0; idx < 16; idx++)
            m[idx] = _load64(_buf.buf() + idx * 8);

        Array<u64, 16> v{};
        for (usize idx = 0; idx < 8; idx++) {
            v[idx] = _state[idx];
            v[idx + 8] = BLAKE2B_IV[idx];
        }

        v[12] ^= _counterLo;
        v[13] ^= _counterHi;

        if (last)
            v[14] = ~v[14];

        for (usize round = 0; round < BLAKE2B_ROUNDS; round++) {
            u8 const* s = BLAKE2B_SIGMA.buf() + (round % 10) * 16;

            _mix(v, 0, 4, 8, 12, m[s[0]], m[s[1]]);
            _mix(v, 1, 5, 9, 13, m[s[2]], m[s[3]]);
            _mix(v, 2, 6, 10, 14, m[s[4]], m[s[5]]);
            _mix(v, 3, 7, 11, 15, m[s[6]], m[s[7]]);

            _mix(v, 0, 5, 10, 15, m[s[8]], m[s[9]]);
            _mix(v, 1, 6, 11, 12, m[s[10]], m[s[11]]);
            _mix(v, 2, 7, 8, 13, m[s[12]], m[s[13]]);
            _mix(v, 3, 4, 9, 14, m[s[14]], m[s[15]]);
        }

        for (usize idx = 0; idx < 8; idx++)
            _state[idx] ^= v[idx] ^ v[idx + 8];
    }

    constexpr void update(u8 byte) {
        if (_bufLen == BLOCK_SIZE) {
            _incrementCounter(BLOCK_SIZE);
            _compress(false);
            _bufLen = 0;
        }
        _buf[_bufLen++] = byte;
    }

    constexpr void update(Bytes bytes) {
        auto [buf, len] = bytes;
        for (usize i = 0; i < len; ++i) {
            update(buf[i]);
        }
    }

    constexpr void finalize() {
        _incrementCounter(_bufLen);
        while (_bufLen < BLOCK_SIZE) {
            _buf[_bufLen++] = 0;
        }
        _compress(true);
    }
};

export constexpr usize BLAKE2B_DYNAMIC = 0;

export template <usize DIGEST_SIZE = BLAKE2B_DYNAMIC>
struct Blake2b {
    static_assert(DIGEST_SIZE >= 1 and DIGEST_SIZE <= BLAKE2B_MAX_BYTES);

    using Digest = Array<u8, DIGEST_SIZE>;
    static constexpr auto BLOCK_SIZE = Blake2bState::BLOCK_SIZE;

    Blake2bState _state{DIGEST_SIZE, Bytes{}};

    constexpr Blake2b() = default;

    constexpr explicit Blake2b(Bytes key) : _state(DIGEST_SIZE, key) {}

    constexpr Blake2b& update(u8 byte) {
        _state.update(byte);
        return *this;
    }

    constexpr Blake2b& update(Bytes bytes) {
        _state.update(bytes);
        return *this;
    }

    constexpr Digest digest() {
        _state.finalize();

        Digest out{};
        for (usize idx = 0; idx < DIGEST_SIZE; idx++)
            out[idx] = static_cast<u8>(_state._state[idx / 8] >> (8 * (idx % 8)));
        return out;
    }
};

template <>
struct Blake2b<BLAKE2B_DYNAMIC> {
    using Digest = Box<u8[]>;

    static constexpr auto BLOCK_SIZE = Blake2bState::BLOCK_SIZE;
    usize _digestSize;
    Blake2bState _state;

    constexpr explicit Blake2b(usize digestSize, Bytes key = {})
        : _digestSize(digestSize), _state(digestSize, key) {}

    constexpr Blake2b& update(u8 byte) {
        _state.update(byte);
        return *this;
    }

    constexpr Blake2b& update(Bytes bytes) {
        _state.update(bytes);
        return *this;
    }

    constexpr Digest digest() {
        _state.finalize();
        auto out = Box<u8[]>::filled(_digestSize);
        for (usize idx = 0; idx < _digestSize; idx++)
            out[idx] = static_cast<u8>(_state._state[idx / 8] >> (8 * (idx % 8)));
        return out;
    }
};

export using Blake2b256 = Blake2b<BLAKE2B256_BYTES>;
export using Blake2b384 = Blake2b<BLAKE2B384_BYTES>;
export using Blake2b512 = Blake2b<BLAKE2B512_BYTES>;

export template <usize DIGEST_SIZE = BLAKE2B512_BYTES>
constexpr Blake2b<DIGEST_SIZE>::Digest blake2b(Bytes bytes) {
    Blake2b<DIGEST_SIZE> b2;
    b2.update(bytes);
    return b2.digest();
}

export template <usize DIGEST_SIZE = BLAKE2B512_BYTES>
constexpr Blake2b<DIGEST_SIZE>::Digest blake2b(Bytes bytes, Bytes key) {
    Blake2b<DIGEST_SIZE> b2{key};
    b2.update(bytes);
    return b2.digest();
}

export constexpr Blake2b256::Digest blake2b256(Bytes bytes) {
    return blake2b<BLAKE2B256_BYTES>(bytes);
}

export constexpr Blake2b384::Digest blake2b384(Bytes bytes) {
    return blake2b<BLAKE2B384_BYTES>(bytes);
}

export constexpr Blake2b512::Digest blake2b512(Bytes bytes) {
    return blake2b<BLAKE2B512_BYTES>(bytes);
}

} // namespace Karm::Crypto