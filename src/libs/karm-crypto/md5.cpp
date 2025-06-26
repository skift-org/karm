#include <karm-base/endian.h>

#include "md5.h"

namespace Karm::Crypto {

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

static constexpr Array<u8, 64> MD5_S = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static inline u32 F(u32 x, u32 y, u32 z) { return (x & y) | (~x & z); }
static inline u32 G(u32 x, u32 y, u32 z) { return (x & z) | (y & ~z); }
static inline u32 H(u32 x, u32 y, u32 z) { return x ^ y ^ z; }
static inline u32 I(u32 x, u32 y, u32 z) { return y ^ (x | ~z); }

Md5::Md5() : _state{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476}, _bitlen{0}, _datalen{0} {}

void Md5::update(Bytes bytes) {
    auto [buf, len] = bytes;

    for (usize i = 0; i < len; ++i) {
        _buffer[_datalen] = buf[i];
        _datalen++;
        
        if (_datalen == 64) {
            transform();
            _bitlen += 512;
            _datalen = 0;
        }
    }
}

void Md5::transform() {
    u32 a = _state[0];
    u32 b = _state[1];
    u32 c = _state[2];
    u32 d = _state[3];
    u32 m[16];
    
    for (usize i = 0; i < 16; ++i) {
        m[i] = _buffer[i * 4]
             | (_buffer[i * 4 + 1] << 8)
             | (_buffer[i * 4 + 2] << 16)
             | (_buffer[i * 4 + 3] << 24);
    }

    // round 1
    for (usize i = 0; i < 16; ++i) {
        u32 temp = F(b, c, d) + a + MD5_K[i] + m[i];
        a = d;
        d = c;
        c = b;
        b += Karm::rotl(temp, MD5_S[i]);
    }

    // round 2
    for (usize i = 16; i < 32; ++i) {
        u32 g = (5 * i + 1) % 16;
        u32 temp = G(b, c, d) + a + MD5_K[i] + m[g];
        a = d;
        d = c;
        c = b;
        b += Karm::rotl(temp, MD5_S[i]);
    }

    // round 3
    for (usize i = 32; i < 48; ++i) {
        u32 g = (3 * i + 5) % 16;
        u32 temp = H(b, c, d) + a + MD5_K[i] + m[g];
        a = d;
        d = c;
        c = b;
        b += Karm::rotl(temp, MD5_S[i]);
    }

    // round 4
    for (usize i = 48; i < 64; ++i) {
        u32 g = (7 * i) % 16;
        u32 temp = I(b, c, d) + a + MD5_K[i] + m[g];
        a = d;
        d = c;
        c = b;
        b += Karm::rotl(temp, MD5_S[i]);
    }

    _state[0] += a;
    _state[1] += b;
    _state[2] += c;
    _state[3] += d;
}

void Md5::finalize() {
    usize i = _datalen;
    
    // pad whatever data is left in the buffer
    _buffer[i++] = 0x80; // append the bit '1' to the message
    
    // pad with zeros until 56 bytes mod 64
    if (i > 56) {
        while (i < 64) {
            _buffer[i++] = 0;
        }
        transform();
        i = 0;
    }
    
    while (i < 56) {
        _buffer[i++] = 0;
    }
    
    u64 bits = _bitlen + (_datalen * 8);
    _buffer[56] = bits;
    _buffer[57] = bits >> 8;
    _buffer[58] = bits >> 16;
    _buffer[59] = bits >> 24;
    _buffer[60] = bits >> 32;
    _buffer[61] = bits >> 40;
    _buffer[62] = bits >> 48;
    _buffer[63] = bits >> 56;
    
    transform();
}

Md5::Digest Md5::digest() {
    Md5::Digest hash;
    
    finalize();

    for (u32 i = 0; i < 4; ++i) {
        hash[i * 4] = _state[i];
        hash[i * 4 + 1] = _state[i] >> 8;
        hash[i * 4 + 2] = _state[i] >> 16;
        hash[i * 4 + 3] = _state[i] >> 24;
    }
    
    return hash;
}

Array<u8, MD5_BYTES> md5(Bytes bytes) {
    Md5 hasher;
    hasher.update(bytes);
    return hasher.digest();
}

} // namespace Karm::Crypto