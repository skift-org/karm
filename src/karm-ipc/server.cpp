module;

#include <karm/macros>

export module Karm.Ipc:server;

import Karm.Sys;
import Karm.Logger;
import Karm.Ref;
import :message;
import :protocol;

namespace Karm::Ipc {

export struct Session {
    Sys::IpcConnection _connection;

    Session(Sys::IpcConnection conn)
        : _connection(std::move(conn)) {
    }

    virtual ~Session() = default;

    virtual Async::Task<> handleAsync(Message& msg, Async::CancellationToken ct) = 0;

    Async::Task<> runAsync(Async::CancellationToken ct) {
        while (true) {
            co_try$(ct.errorIfCanceled());
            auto message = co_trya$(Ipc::recvAsync(_connection, ct));
            auto res = co_await handleAsync(*message, ct);
            if (not res)
                logInfo("message handling error: {}", res);
        }
    }

    template <typename T>
    Res<> notify(T const& payload) {
        return Ipc::send<T>(_connection, SEQ_EVENT, payload);
    }

    template <typename T>
    Res<> resp(Message& msg, Res<typename T::Response> payload) {
        auto header = msg._header;
        if (not payload)
            return Ipc::send<Error>(_connection, header.seq, payload.none());
        return Ipc::send<typename T::Response>(_connection, header.seq, payload.take());
    }

    bool operator==(Session const& other) const {
        return this == &other;
    }
};

export struct Handler {
    virtual ~Handler() = default;

    virtual Async::Task<Rc<Session>> acceptSessionAsync(Sys::IpcConnection conn, Async::CancellationToken ct) = 0;
};

export struct Server : Meta::NoCopy {
    struct _State {
        Sys::IpcListener listener;
        Vec<Rc<Session>> sessions = {};
        Async::Cancellation cancellation = {};
    };

    Rc<_State> _state;
    Rc<Handler> _handler;
    Opt<bool> _active = true; // HACK: Abuse the fact that when an Opt is moved from it become NONE

    static Async::Task<Server> createAsync(Ref::Url url, Rc<Handler> handler) {
        auto listener = co_try$(Sys::IpcListener::listen(url));
        co_return Ok<Server>(std::move(listener), handler);
    }

    Server(Sys::IpcListener listener, Rc<Handler> handler)
        : _state(makeRc<_State>(std::move(listener))), _handler(handler) {
    }

    Server(Server&&) = default;

    ~Server() {
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

} // namespace Karm::Ipc