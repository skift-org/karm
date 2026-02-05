module;

#include <karm/macros>

export module Karm.Ref:uid;

import Karm.Core;
import Karm.Crypto;

namespace Karm::Ref {

struct _UuidTag;
struct _GuidTag;

template <typename Tag, typename U32, typename U16>
struct _Uid {
    U32 a{};
    U16 b{};
    U16 c{};
    Array<u8, 8> d{};

    static Res<_Uid> v4()
        requires Meta::Same<Tag, _UuidTag>
    {
        _Uid uuid;
        auto bytes = uuid.mutBytes();
        try$(Crypto::entropy(bytes));
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        return Ok(uuid);
    }

    static Res<_Uid> v7(SystemTime ts)
        requires Meta::Same<Tag, _UuidTag>
    {
        _Uid uuid;
        auto timestamp = ts.sinceEpoch().toMSecs();
        uuid.a = (timestamp >> 16) & 0xffffffff;
        uuid.b = timestamp & 0xffff;
        auto bytes = uuid.mutBytes();
        try$(Crypto::entropy(mutSub(bytes, 6, 16)));
        bytes[6] = (bytes[6] & 0x0f) | 0x70;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        return Ok(uuid);
    }

    static Res<_Uid> parse(Io::SScan& s) {
        _Uid uid;
        uid.a = try$(Io::atou(s, {.base = 16}));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        uid.b = try$(Io::atou(s, {.base = 16}));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        uid.c = try$(Io::atou(s, {.base = 16}));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        try$(Crypto::hexDecode(s, uid.d));
        return Ok(uid);
    }

    static Res<_Uid> parse(Str str) {
        Io::SScan s{str};
        return parse(s);
    }

    Bytes bytes() const {
        return {reinterpret_cast<u8 const*>(this), sizeof(_Uid)};
    }

    MutBytes mutBytes() {
        return {reinterpret_cast<u8*>(this), sizeof(_Uid)};
    }

    bool operator==(_Uid const&) const = default;

    auto operator<=>(_Uid const&) const = default;

    void repr(Io::Emit& e) const {
        e("{:08x}-{:04x}-{:04x}-", a, b, c);
        Crypto::hexEncode(d, e).unwrap();
    }

    String unparsed() const {
        return Io::toStr(*this);
    }

    Res<> serialize(Serde::Serializer& ser) const {
        return Serde::serialize(ser, Io::toStr(*this));
    }

    static Res<_Uid> deserialize(Serde::Deserializer& de) {
        return parse(try$(Serde::deserialize<String>(de)));
    }
};

export using Uuid = _Uid<_UuidTag, u32be, u16be>;

/// <b>FUCK MICROSLOP</b>
export using Guid = _Uid<_UuidTag, u32le, u16le>;

static_assert(sizeof(Uuid) == 16);

} // namespace Karm::Ref
