export module Karm.Crypto:sha2;

import Karm.Core;

namespace Karm::Crypto {

export constexpr usize SHA224_BYTES = 28;
export constexpr usize SHA256_BYTES = 32;
export constexpr usize SHA384_BYTES = 48;
export constexpr usize SHA512_BYTES = 64;

static constexpr Array<u32, 8> SHA224_INITIAL = {
    0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
    0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
};

static constexpr Array<u32, 8> SHA256_INITIAL = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static constexpr Array<u32, 64> SHA256_K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static constexpr Array<u64, 8> SHA384_INITIAL = {
    0xcbbb9d5dc1059ed8, 0x629a292a367cd507,
    0x9159015a3070dd17, 0x152fecd8f70e5939,
    0x67332667ffc00b31, 0x8eb44a8768581511,
    0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4
};

static constexpr Array<u64, 8> SHA512_INITIAL = {
    0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
    0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
    0x510e527fade682d1, 0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
};

static constexpr Array<u64, 80> SHA512_K = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f,
    0xe9b5dba58189dbbc, 0x3956c25bf348b538, 0x59f111f1b605d019,
    0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242,
    0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
    0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3,
    0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65, 0x2de92c6f592b0275,
    0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f,
    0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
    0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc,
    0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6,
    0x92722c851482353b, 0xa2bfe8a14cf10364, 0xa81a664bbc423001,
    0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
    0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99,
    0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb,
    0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc,
    0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915,
    0xc67178f2e372532b, 0xca273eceea26619c, 0xd186b8c721c0c207,
    0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba,
    0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
    0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a,
    0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

struct Sha256State {
    static constexpr usize BLOCK_SIZE = 64;

    Array<u32, 8> _state;
    Array<u8, BLOCK_SIZE> _buf{};
    usize _bufLen = 0;
    u64 _totalLen = 0;

    constexpr Sha256State(Array<u32, 8> const& init) : _state(init) {}

    constexpr void _computeBlock(u8 const* buf) {
        Array<u32, 64> w{};

        for (usize idx = 0; idx < 16; idx++) {
            w[idx] = toBe(reinterpret_cast<u32 const*>(buf)[idx]);
        }

        for (usize idx = 16; idx < 64; idx++) {
            u32 s0 = rotr(w[idx - 15], 7) ^ rotr(w[idx - 15], 18) ^ (w[idx - 15] >> 3);
            u32 s1 = rotr(w[idx - 2], 17) ^ rotr(w[idx - 2], 19) ^ (w[idx - 2] >> 10);
            w[idx] = w[idx - 16] + s0 + w[idx - 7] + s1;
        }

        u32 a = _state[0];
        u32 b = _state[1];
        u32 c = _state[2];
        u32 d = _state[3];
        u32 e = _state[4];
        u32 f = _state[5];
        u32 g = _state[6];
        u32 h = _state[7];

        for (usize idx = 0; idx < 64; idx++) {
            u32 s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            u32 ch = (e & f) ^ ((~e) & g);
            u32 tmp1 = h + s1 + ch + SHA256_K[idx] + w[idx];
            u32 s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            u32 tmp2 = s0 + ((a & b) ^ (a & c) ^ (b & c));

            h = g;
            g = f;
            f = e;
            e = d + tmp1;
            d = c;
            c = b;
            b = a;
            a = tmp1 + tmp2;
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
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

struct Sha512State {
    static constexpr usize BLOCK_SIZE = 128;
    Array<u64, 8> _state;
    Array<u8, BLOCK_SIZE> _buf{};
    usize _bufLen = 0;
    u64 _totalLen = 0;

    constexpr Sha512State(Array<u64, 8> const& init) : _state(init) {}

    constexpr void _computeBlock(u8 const* buf) {
        Array<u64, 80> w{};

        for (usize idx = 0; idx < 16; idx++) {
            w[idx] = toBe(reinterpret_cast<u64 const*>(buf)[idx]);
        }

        for (usize idx = 16; idx < 80; idx++) {
            u64 s0 = rotr(w[idx - 15], 1) ^ rotr(w[idx - 15], 8) ^ (w[idx - 15] >> 7);
            u64 s1 = rotr(w[idx - 2], 19) ^ rotr(w[idx - 2], 61) ^ (w[idx - 2] >> 6);
            w[idx] = w[idx - 16] + s0 + w[idx - 7] + s1;
        }

        u64 a = _state[0];
        u64 b = _state[1];
        u64 c = _state[2];
        u64 d = _state[3];
        u64 e = _state[4];
        u64 f = _state[5];
        u64 g = _state[6];
        u64 h = _state[7];

        for (usize idx = 0; idx < 80; idx++) {
            u64 s1 = rotr(e, 14) ^ rotr(e, 18) ^ rotr(e, 41);
            u64 ch = (e & f) ^ ((~e) & g);
            u64 tmp1 = h + s1 + ch + SHA512_K[idx] + w[idx];
            u64 s0 = rotr(a, 28) ^ rotr(a, 34) ^ rotr(a, 39);
            u64 tmp2 = s0 + ((a & b) ^ (a & c) ^ (b & c));

            h = g;
            g = f;
            f = e;
            e = d + tmp1;
            d = c;
            c = b;
            b = a;
            a = tmp1 + tmp2;
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
    }

    constexpr void update(u8 byte) {
        _buf[_bufLen++] = byte;
        if (_bufLen == 128) {
            _computeBlock(_buf.buf());
            _bufLen = 0;
            _totalLen += 128;
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

        if (_bufLen > 112) {
            while (_bufLen < 128) {
                _buf[_bufLen++] = 0;
            }
            _computeBlock(_buf.buf());
            _bufLen = 0;
        }

        while (_bufLen < 120) {
            _buf[_bufLen++] = 0;
        }

        _buf[120] = padlen >> 56;
        _buf[121] = padlen >> 48;
        _buf[122] = padlen >> 40;
        _buf[123] = padlen >> 32;
        _buf[124] = padlen >> 24;
        _buf[125] = padlen >> 16;
        _buf[126] = padlen >> 8;
        _buf[127] = padlen;

        _computeBlock(_buf.buf());

        for (usize idx = 0; idx < _state.len(); idx++) {
            _state[idx] = toBe(_state[idx]);
        }
    }
};

// ==============================================================================
// No macros needed: C++20 allows passing the array references directly
// ==============================================================================

export template <typename State, usize DIGEST_SIZE, auto const& INITIAL>
struct Sha {
    using Digest = Array<u8, DIGEST_SIZE>;
    static constexpr auto BLOCK_SIZE = State::BLOCK_SIZE;

    State _state{INITIAL};

    constexpr Sha() = default;

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

export using Sha224 = Sha<Sha256State, SHA224_BYTES, SHA224_INITIAL>;
export using Sha256 = Sha<Sha256State, SHA256_BYTES, SHA256_INITIAL>;
export using Sha384 = Sha<Sha512State, SHA384_BYTES, SHA384_INITIAL>;
export using Sha512 = Sha<Sha512State, SHA512_BYTES, SHA512_INITIAL>;

export constexpr Sha224::Digest sha224(Bytes bytes) {
    Sha224 sha;
    sha.update(bytes);
    return sha.digest();
}

export constexpr Sha256::Digest sha256(Bytes bytes) {
    Sha256 sha;
    sha.update(bytes);
    return sha.digest();
}

export constexpr Sha384::Digest sha384(Bytes bytes) {
    Sha384 sha;
    sha.update(bytes);
    return sha.digest();
}

export constexpr Sha512::Digest sha512(Bytes bytes) {
    Sha512 sha;
    sha.update(bytes);
    return sha.digest();
}

} // namespace Karm::Crypto