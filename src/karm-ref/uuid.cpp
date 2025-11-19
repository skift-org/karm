module;

#include <karm-core/macros.h>

export module Karm.Ref:uuid;

import Karm.Core;
import Karm.Crypto;

namespace Karm::Ref {

export struct Uuid {
    u32 a{};
    u16 b{};
    u16 c{};
    Array<u8, 8> d{};

    static Res<Uuid> v4() {
        Uuid uuid;
        auto bytes = uuid.mutBytes();
        try$(Crypto::entropy(bytes));
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        return Ok(uuid);
    }

    static Res<Uuid> parse(Io::SScan& s) {
        Uuid uuid;
        uuid.a = try$(Io::atou(s, {.base = 16}));
        if (not s.skip("-"))
            return Error::invalidInput("expected uuid");

        uuid.b = try$(Io::atou(s, {.base = 16}));
        if (not s.skip("-"))
            return Error::invalidInput("expected uuid");

        uuid.c = try$(Io::atou(s, {.base = 16}));
        if (not s.skip("-"))
            return Error::invalidInput("expected uuid");

        try$(Crypto::hexDecode(s, uuid.d));
        return Ok(uuid);
    }

    static Res<Uuid> parse(Str str) {
        Io::SScan s{str};
        return parse(s);
    }

    Bytes bytes() const {
        return {reinterpret_cast<u8 const*>(this), sizeof(Uuid)};
    }

    MutBytes mutBytes() {
        return {reinterpret_cast<u8*>(this), sizeof(Uuid)};
    }

    bool operator==(Uuid const&) const = default;

    auto operator<=>(Uuid const&) const = default;

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

    static Res<Uuid> deserialize(Serde::Deserializer& de) {
        return parse(try$(Serde::deserialize<String>(de)));
    }
};

static_assert(sizeof(Uuid) == 16);

} // namespace Karm::Ref
