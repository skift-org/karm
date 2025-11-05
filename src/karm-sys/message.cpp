module;

#include <karm-core/macros.h>

export module Karm.Sys:message;

import Karm.Core;
import :types;

namespace Karm::Sys {

// MARK: Message Serializer/Deserializer ---------------------------------------

export struct MessageSerializer : Serde::PackSerializer {
    Vec<Handle> _handles = {};
    bool _inHandle = false;

    MessageSerializer(Io::BEmit& emit)
        : PackSerializer(emit) {}

    void give(Handle hnd) {
        _handles.emplaceBack(hnd);
    }

    Slice<Handle> handles() const {
        return _handles;
    }

    void clear() {
        _handles.clear();
    }

    Res<> beginUnit(Serde::Type t) override {
        if (t.kind == Serde::Type::UNIT and t.tag == "__handle__")
            _inHandle = true;
        return PackSerializer::beginUnit(t);
    }

    Res<> endUnit() override {
        _inHandle = false;
        return PackSerializer::endUnit();
    }

    Res<> serializeUnsigned(u64 v, Serde::SizeHint hint) override {
        if (_inHandle) {
            give(Sys::Handle{v});
            return Ok();
        }
        return PackSerializer::serializeUnsigned(v, hint);
    }
};

export struct MessageDeserializer : Serde::PackDeserializer {
    Cursor<Handle> _handles;
    bool _inHandle = false;

    MessageDeserializer(Io::BScan& scan, Slice<Handle> handles)
        : PackDeserializer(scan), _handles(handles) {}

    Handle take() {
        if (_handles.ended())
            return INVALID;
        return _handles.next();
    }

    Res<Serde::Type> beginUnit(Serde::Type t) override {
        if (t.kind == Serde::Type::UNIT and t.tag == "__handle__")
            _inHandle = true;
        return PackDeserializer::beginUnit(t);
    }

    Res<> endUnit() override {
        _inHandle = false;
        return PackDeserializer::endUnit();
    }

    Res<u64> deserializeUnsigned(Serde::SizeHint hint) override {
        if (_inHandle)
            return Ok(take().value());
        return PackDeserializer::deserializeUnsigned(hint);
    }
};

// MARK: Primitive Types -------------------------------------------------------

export struct Port : Distinct<u64, struct _PortTag> {
    static Port const INVALID;
    static Port const BUS;
    static Port const BROADCAST;

    using Distinct::Distinct;

    void repr(Io::Emit& e) const {
        if (*this == INVALID)
            e("invalid");
        else if (*this == BUS)
            e("bus");
        else if (*this == BROADCAST)
            e("broadcast");
        else
            e("{}", value());
    }

    Res<> serialize(Serde::Serializer& ser) const {
        return ser.serialize(value());
    }

    static Res<Port> deserialize(Serde::Deserializer& de) {
        return Ok(Port{try$(de.deserialize<u64>())});
    }
};

Port const Port::INVALID{0};
Port const Port::BUS{Limits<u64>::MAX};
Port const Port::BROADCAST{Limits<u64>::MAX - 1};

export struct Header {
    u64 seq;
    Port from;
    Port to;
    Meta::Id mid;

    void repr(Io::Emit& e) const {
        e("(header seq: {}, from: {}, to: {}, mid: {:016x})", seq, from, to, mid);
    }
};

static_assert(Meta::TrivialyCopyable<Header>);

export struct Message {
    static constexpr usize CAP = 4096;

    union {
        struct {
            Header _header;
            Array<u8, CAP - sizeof(Header)> _payload;
        };

        Array<u8, CAP> _buf;
    };

    usize _len = 0;

    Array<Handle, 16> _hnds;
    usize _hndsLen = 0;

    Header& header() {
        return _header;
    }

    Header const& header() const {
        return _header;
    }

    usize len() const {
        return _len;
    }

    Bytes bytes() {
        return sub(_buf, 0, len());
    }

    Slice<Handle> handles() const {
        return sub(_hnds, 0, _hndsLen);
    }

    template <typename T>
    bool is() const {
        return _header.mid == Meta::idOf<T>();
    }

    template <typename T, typename... Args>
    static Res<Message> packReq(Port to, u64 seq, Args&&... args) {
        T payload{std::forward<Args>(args)...};

        Message msg;
        msg._header = {
            seq,
            Port::INVALID,
            to,
            Meta::idOf<T>(),
        };
        Io::BufWriter reqBuf{msg._payload};
        Io::BEmit emit{reqBuf};
        MessageSerializer messageSerializer{emit};

        try$(Serde::serialize(messageSerializer, payload));
        for (auto h : messageSerializer.handles()) {
            msg._hnds[msg._hndsLen++] = h;
        }

        msg._len = try$(Io::tell(reqBuf)) + sizeof(Header);

        return Ok(std::move(msg));
    }

    template <typename T, typename... Args>
    Res<Message> packResp(Args&&... args) {
        typename T::Response payload{std::forward<Args>(args)...};

        Message resp;
        resp._header = {
            header().seq,
            header().to,
            header().from,
            Meta::idOf<typename T::Response>(),
        };

        Io::BufWriter respBuf{resp._payload};
        Io::BEmit emit{respBuf};
        MessageSerializer messageSerializer{emit};
        try$(Serde::serialize(messageSerializer, payload));

        resp._len = try$(Io::tell(respBuf)) + sizeof(Header);

        return Ok(std::move(resp));
    }

    template <typename T>
    Res<T> unpack() {
        Io::BScan scan{bytes()};
        MessageDeserializer s{scan, handles()};
        if (not is<T>())
            return Error::invalidData("unexpected message");
        try$(Serde::deserialize<Header>(s));
        return Serde::deserialize<T>(s);
    }
};

} // namespace Karm::Sys
