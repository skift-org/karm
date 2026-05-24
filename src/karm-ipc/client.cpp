module;

#include <karm/macros>

export module Karm.Ipc:client;

import Karm.Core;
import Karm.Sys;
import Karm.Logger;
import Karm.Ref;

import :protocol;
import :message;

namespace Karm::Ipc {

export struct Client : Meta::NoCopy {
    struct _State {
        Sys::IpcConnection connection;
        Async::Queue<Message> incoming{};
        Map<u64, Async::_Promise<Message>> pending{};
        Async::Cancellation cancellation;
        u64 seq = 1;

        void failAllPending(Error error) {
            Message const msg{};
            (void)msg.packReq<Error>(SEQ_EVENT, error);
            for (auto& v : pending.mutIterValue())
                v.resolve(msg);
        }
    };

    Rc<_State> _state;
    Opt<bool> _active = true; // HACK: Abuse the fact that when an Opt is moved from it become NONE

    static Async::Task<Client> connectAsync(Ref::Url url, Async::CancellationToken) {
        auto conn = co_try$(Sys::IpcConnection::connect(url));
        co_return Ok<Client>(std::move(conn));
    }

    Client(Sys::IpcConnection con) : _state(makeRc<_State>(std::move(con))) {
        Async::detach(_receiverTask(_state, _state->cancellation.token()), [](Res<> res) {
            logError("receiver task exited: {}", res);
        });
    }

    Client(Client&&) = default;

    ~Client() {
        if (_active)
            _state->cancellation.cancel();
    }

    static Async::Task<> _receiverTask(Rc<_State> state, Async::CancellationToken ct) {
        while (true) {
            co_try$(ct.errorIfCanceled());

            auto res = co_await Ipc::recvAsync(state->connection, ct);
            if (not res) {
                state->failAllPending(res.none());
                co_return res.none();
            }

            auto& msg = res.unwrap();
            auto header = msg._header;

            if (state->pending.contains(header.seq)) {
                auto promise = state->pending.remove(header.seq);
                promise->resolve(std::move(msg));
            } else if (header.seq == SEQ_EVENT) {
                state->incoming.enqueue(std::move(msg));
            } else {
                logWarn("unexpected message: {}", header);
            }
        }
    }

    Async::Task<Message> recvAsync(Async::CancellationToken ct) {
        co_return Ok(co_await _state->incoming.dequeueAsync(ct));
    }

    Opt<Message> poll() {
        return _state->incoming.tryDequeue();
    }

    template <typename T>
    Async::Task<typename T::Response> callAsync(T const& payload, Async::CancellationToken) {
        // FIXME: Handle cancellation
        auto seq = _state->seq++;
        Async::_Promise<Message> promise;
        auto future = promise.future();
        _state->pending.put(seq, std::move(promise));

        co_try$(Ipc::send<T>(_state->connection, seq, payload));

        Message msg = co_await future;

        if (msg.is<Error>())
            co_return co_try$(msg.unpack<Error>());

        if (not msg.is<typename T::Response>()) {
            co_return Error::invalidInput("unexpected response");
        }

        co_return Ok(co_try$(msg.unpack<typename T::Response>()));
    }

    template <typename T>
    Res<> notify(T const& payload) {
        return Ipc::send<T>(_state->connection, SEQ_EVENT, payload);
    }
};

} // namespace Karm::Ipc