module;

#include <karm/macros>

export module Karm.Ipc:message;

import Karm.Core;
import Karm.Sys;

import :serde;

namespace Karm::Ipc {

export constexpr u64 SEQ_EVENT = -1;

export struct Header {
    u64 seq;
    Meta::Id mid;

    void repr(Io::Emit& e) const {
        e("(header seq: {},  mid: {:016x})", seq, mid);
    }
};

static_assert(Meta::TriviallyCopyable<Header>);

export struct Message {
    static constexpr usize CAP = 4096;

    union {
        struct
        {
            Header _header;
            Array<u8, CAP - sizeof(Header)> _payload;
        };

        Array<u8, CAP> _buf;
    };

    usize _len = 0;

    Array<Sys::Handle, 16> _hnds;
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

    Slice<Sys::Handle> handles() const {
        return sub(_hnds, 0, _hndsLen);
    }

    template <typename T>
    bool is() const {
        return _header.mid == Meta::idOf<T>();
    }

    template <typename T>
    static Res<Message> packReq(u64 seq, T const& payload) {
        Message msg;
        msg._header = {
            seq,
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

    template <typename T>
    Res<Message> packResp(T::Response const& payload) {
        Message resp;
        resp._header = {
            header().seq,
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

} // namespace Karm::Ipc