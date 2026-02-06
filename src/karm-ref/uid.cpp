module;

#include <karm/macros>

export module Karm.Ref:uid;

import Karm.Core;
import Karm.Crypto;

namespace Karm::Ref {

// https://datatracker.ietf.org/doc/html/rfc9562
template <typename U32, typename U16>
struct [[gnu::packed]] _Uid {
    U32 timeLow{};
    U16 timeMid{};
    U16 timeHighAndVersion{};
    u16be clkSeqAndVariant{};
    Array<u8, 6> node{};

    static Res<_Uid> v4() {
        _Uid uid;
        auto bytes = uid.mutBytes();
        try$(Crypto::entropy(bytes));
        uid._setVersionAndVariant(4, 2);
        return Ok(uid);
    }

    static Res<_Uid> v7(SystemTime ts) {
        _Uid uid;
        auto timestamp = ts.sinceEpoch().toMSecs();
        uid.timeLow = (timestamp >> 16) & 0xffffffff;
        uid.timeMid = timestamp & 0xffff;
        auto bytes = uid.mutBytes();
        try$(Crypto::entropy(mutSub(bytes, 6, 16)));
        uid._setVersionAndVariant(7, 2);
        return Ok(uid);
    }

    void _setVersionAndVariant(u8 version, u8 variant) {
        timeHighAndVersion = (timeHighAndVersion & 0x0fff) | (version & 0x0f) << 12;
        clkSeqAndVariant = (clkSeqAndVariant & 0x3fff) | (variant & 0x03) << 14;
    }

    u8 version() const {
        return timeHighAndVersion >> 12;
    }

    u8 variant() const {
        return clkSeqAndVariant >> 14;
    }

    static Res<_Uid> parse(Io::SScan& s) {
        _Uid uid;
        uid.timeLow = try$(Io::atou(s, {.base = 16}).okOr(Error::invalidInput("expected uid")));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        uid.timeMid = try$(Io::atou(s, {.base = 16}).okOr(Error::invalidInput("expected uid")));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        uid.timeHighAndVersion = try$(Io::atou(s, {.base = 16}).okOr(Error::invalidInput("expected uid")));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        uid.clkSeqAndVariant = try$(Io::atou(s, {.base = 16}).okOr(Error::invalidInput("expected uid")));
        if (not s.skip("-"))
            return Error::invalidInput("expected uid");

        try$(Crypto::hexDecode(s, uid.node));
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
        e("{:08x}-{:04x}-{:04x}-{:04x}-", timeLow, timeMid, timeHighAndVersion, clkSeqAndVariant);
        Crypto::hexEncode(node, e).unwrap();
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

export using Uuid = _Uid<u32be, u16be>;

/// <b>FUCK MICROSLOP</b>
export using Guid = _Uid<u32le, u16le>;

static_assert(sizeof(Uuid) == 16);
static_assert(sizeof(Guid) == 16);

} // namespace Karm::Ref

export Karm::Ref::Uuid operator""_uuid(char const* str, Karm::usize len) {
    auto res = Karm::Ref::Uuid::parse({str, len});
    if (not res.has())
        Karm::debug(res.none().msg());
    return res.unwrap("invalid UUID");
}

/// <b>FUCK MICROSLOP</b>
export Karm::Ref::Guid operator""_guid(char const* str, Karm::usize len) {
    return Karm::Ref::Guid::parse({str, len}).unwrap("invalid GUID");
}
