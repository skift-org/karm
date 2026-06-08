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
    Opt<bool> _active = true; // HACK: Abuse the fact that when an Opt is moved from, it becomes NONE

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
        for (auto& s : _state->sessions) {
            (void)s->notify(payload);
        }
    }

    static Async::Task<> acceptSessionAsync(Sys::IpcConnection connection, Rc<_State> state, Rc<Handler> handler, Async::CancellationToken ct) {
        auto session = co_trya$(handler->acceptSessionAsync(connection, ct));
        state->sessions.pushBack(session);
        Defer _ = [&] {
            state->sessions.removeAll(session);
        };

        while (true) {
            co_try$(ct.errorIfCanceled());
            auto message = co_trya$(Ipc::recvAsync(connection, ct));
            auto res = co_await session->handleAsync(*message, ct);
            if (not res)
                logInfo("message handling error: {}", res);
        }

        co_return Ok();
    }

    Async::Task<> servAsync(Async::CancellationToken ct) {
        while (true) {
            auto connection = co_trya$(_state->listener.acceptAsync(ct));
            Async::detach(acceptSessionAsync(std::move(connection), _state, _handler, _state->cancellation.token()), [](Res<> r) {
                logDebug("connection closed {}", r);
            });
        }
    }
};

} // namespace Karm::Ipc
