module;

#include <karm/macros>

export module Karm.Sys:ipc;

import Karm.Core;
import Karm.Ref;
import Karm.Logger;
import :socket;

namespace Karm::Sys {

// MARK: Message Serializer/Deserializer ---------------------------------------

export struct IpcMessageSerializer : Serde::PackSerializer {
    Vec<Handle> _handles = {};
    bool _inHandle = false;

    IpcMessageSerializer(Io::BEmit& emit)
        : PackSerializer(emit) {
    }

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

export struct IpcMessageDeserializer : Serde::PackDeserializer {
    Cursor<Handle> _handles;
    bool _inHandle = false;

    IpcMessageDeserializer(Io::BScan& scan, Slice<Handle> handles)
        : PackDeserializer(scan), _handles(handles) {
    }

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

export constexpr u64 SEQ_EVENT = -1;

export struct IpcHeader {
    u64 seq;
    Meta::Id mid;

    void repr(Io::Emit& e) const {
        e("(header seq: {},  mid: {:016x})", seq, mid);
    }
};

static_assert(Meta::TrivialyCopyable<IpcHeader>);

export struct IpcMessage {
    static constexpr usize CAP = 4096;

    union {
        struct
        {
            IpcHeader _header;
            Array<u8, CAP - sizeof(IpcHeader)> _payload;
        };

        Array<u8, CAP> _buf;
    };

    usize _len = 0;

    Array<Handle, 16> _hnds;
    usize _hndsLen = 0;

    IpcHeader& header() {
        return _header;
    }

    IpcHeader const& header() const {
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

    template <typename T>
    static Res<IpcMessage> packReq(u64 seq, T const& payload) {
        IpcMessage msg;
        msg._header = {
            seq,
            Meta::idOf<T>(),
        };
        Io::BufWriter reqBuf{msg._payload};
        Io::BEmit emit{reqBuf};
        IpcMessageSerializer messageSerializer{emit};

        try$(Serde::serialize(messageSerializer, payload));
        for (auto h : messageSerializer.handles()) {
            msg._hnds[msg._hndsLen++] = h;
        }

        msg._len = try$(Io::tell(reqBuf)) + sizeof(IpcHeader);

        return Ok(std::move(msg));
    }

    template <typename T>
    Res<IpcMessage> packResp(T::Response const& payload) {
        IpcMessage resp;
        resp._header = {
            header().seq,
            Meta::idOf<typename T::Response>(),
        };

        Io::BufWriter respBuf{resp._payload};
        Io::BEmit emit{respBuf};
        IpcMessageSerializer messageSerializer{emit};
        try$(Serde::serialize(messageSerializer, payload));

        resp._len = try$(Io::tell(respBuf)) + sizeof(IpcHeader);

        return Ok(std::move(resp));
    }

    template <typename T>
    Res<T> unpack() {
        Io::BScan scan{bytes()};
        IpcMessageDeserializer s{scan, handles()};
        if (not is<T>())
            return Error::invalidData("unexpected message");
        try$(Serde::deserialize<IpcHeader>(s));
        return Serde::deserialize<T>(s);
    }
};

// MARK: Primitive operations --------------------------------------------------

export template <typename T>
Res<> rpcSend(IpcConnection& con, u64 seq, T const& payload) {
    auto msg = try$(IpcMessage::packReq<T>(seq, payload));
    try$(con.send(msg.bytes(), msg.handles()));
    return Ok();
}

export Async::Task<IpcMessage> rpcRecvAsync(IpcConnection& con, Async::CancellationToken ct) {
    IpcMessage msg;
    auto [bufLen, hndsLen] = co_trya$(con.recvAsync(msg._buf, msg._hnds, ct));
    if (bufLen < sizeof(IpcHeader))
        co_return Error::invalidData("invalid message");
    msg._len = bufLen;
    msg._hndsLen = hndsLen;

    co_return msg;
}

// MARK: Client ----------------------------------------------------------------

export struct IpcClient : Meta::NoCopy {
    struct _State {
        IpcConnection connection;
        Async::Queue<IpcMessage> incoming{};
        Map<u64, Async::_Promise<IpcMessage>> pending{};
        Async::Cancellation cancellation;
        u64 seq = 1;

        void failAllPending(Error error) {
            IpcMessage const msg{};
            (void)msg.packReq<Error>(SEQ_EVENT, error);
            for (auto& [seq, v] : pending.iterUnordered()) {
                v.resolve(msg);
            }
        }
    };

    Rc<_State> _state;
    Opt<bool> _active = true; // HACK: Abuse the fact that when an Opt is moved from it become NONE

    static Async::Task<IpcClient> connectAsync(Ref::Url url, Async::CancellationToken) {
        auto conn = co_try$(Sys::IpcConnection::connect(url));
        co_return Ok<IpcClient>(std::move(conn));
    }

    IpcClient(IpcConnection con) : _state(makeRc<_State>(std::move(con))) {
        Async::detach(_receiverTask(_state, _state->cancellation.token()), [](Res<> res) {
            logError("receiver task exited: {}", res);
        });
    }

    IpcClient(IpcClient&&) = default;

    ~IpcClient() {
        if (_active)
            _state->cancellation.cancel();
    }

    static Async::Task<> _receiverTask(Rc<_State> state, Async::CancellationToken ct) {
        while (true) {
            co_try$(ct.errorIfCanceled());

            auto res = co_await rpcRecvAsync(state->connection, ct);
            if (not res) {
                state->failAllPending(res.none());
                co_return res.none();
            }

            auto& msg = res.unwrap();
            auto header = msg._header;

            if (state->pending.has(header.seq)) {
                auto promise = state->pending.take(header.seq);
                promise.resolve(std::move(msg));
            } else if (header.seq == SEQ_EVENT) {
                state->incoming.enqueue(std::move(msg));
            } else {
                logWarn("unexpected message: {}", header);
            }
        }
    }

    Async::Task<IpcMessage> recvAsync(Async::CancellationToken ct) {
        co_return Ok(co_await _state->incoming.dequeueAsync(ct));
    }

    Opt<IpcMessage> poll() {
        return _state->incoming.tryDequeue();
    }

    template <typename T>
    Async::Task<typename T::Response> callAsync(T const& payload, Async::CancellationToken) {
        // FIXME: Handle cancellation
        auto seq = _state->seq++;
        Async::_Promise<IpcMessage> promise;
        auto future = promise.future();
        _state->pending.put(seq, std::move(promise));

        co_try$(rpcSend<T>(_state->connection, seq, payload));

        IpcMessage msg = co_await future;

        if (msg.is<Error>())
            co_return co_try$(msg.unpack<Error>());

        if (not msg.is<typename T::Response>()) {
            co_return Error::invalidInput("unexpected response");
        }

        co_return Ok(co_try$(msg.unpack<typename T::Response>()));
    }

    template <typename T>
    Res<> notify(T const& payload) {
        return rpcSend<T>(_state->connection, SEQ_EVENT, payload);
    }
};

// MARK: Server ----------------------------------------------------------------

export struct IpcSession {
    IpcConnection _connection;

    IpcSession(IpcConnection conn)
        : _connection(std::move(conn)) {
    }

    virtual ~IpcSession() = default;

    virtual Async::Task<> handleAsync(IpcMessage& msg, Async::CancellationToken ct) = 0;

    Async::Task<> runAsync(Async::CancellationToken ct) {
        while (true) {
            co_try$(ct.errorIfCanceled());
            auto message = co_trya$(rpcRecvAsync(_connection, ct));
            auto res = co_await handleAsync(message, ct);
            if (not res)
                logInfo("message handling error: {}", res);
        }
    }

    template <typename T>
    Res<> notify(T const& payload) {
        return rpcSend<T>(_connection, SEQ_EVENT, payload);
    }

    template <typename T>
    Res<> resp(IpcMessage& msg, Res<typename T::Response> payload) {
        auto header = msg._header;
        if (not payload)
            return rpcSend<Error>(_connection, header.seq, payload.none());
        return rpcSend<typename T::Response>(_connection, header.seq, payload.take());
    }

    bool operator==(IpcSession const& other) const {
        return this == &other;
    }
};

export struct IpcHandler {
    virtual ~IpcHandler() = default;

    virtual Async::Task<Rc<IpcSession>> acceptSessionAsync(IpcConnection conn, Async::CancellationToken ct) = 0;
};

export struct IpcServer : Meta::NoCopy {
    struct _State {
        IpcListener listener;
        Vec<Rc<IpcSession>> sessions = {};
        Async::Cancellation cancellation = {};
    };

    Rc<_State> _state;
    Rc<IpcHandler> _handler;
    Opt<bool> _active = true; // HACK: Abuse the fact that when an Opt is moved from it become NONE

    static Async::Task<IpcServer> createAsync(Ref::Url url, Rc<IpcHandler> handler) {
        auto listener = co_try$(IpcListener::listen(url));
        co_return Ok<IpcServer>(std::move(listener), handler);
    }

    IpcServer(IpcListener listener, Rc<IpcHandler> handler)
        : _state(makeRc<_State>(std::move(listener))), _handler(handler) {
    }

    IpcServer(IpcServer&&) = default;

    ~IpcServer() {
        if (_active)
            _state->cancellation.cancel();
    }

    template <typename T>
    void broadcast(T const& payload) {
        for (auto s : _state->sessions) {
            (void)s->notify(payload);
        }
    }

    Async::Task<> servAsync(Async::CancellationToken ct) {
        while (true) {
            auto conn = co_trya$(_state->listener.acceptAsync(ct));
            auto session = co_trya$(_handler->acceptSessionAsync(std::move(conn), ct));
            _state->sessions.pushBack(session);
            Async::detach(
                session->runAsync(_state->cancellation.token()),
                [state = _state, session](Res<> res) mutable {
                    logInfo("session close: {}", res);
                    state->sessions.removeAll(session);
                }
            );
        }
    }
};

} // namespace Karm::Sys
