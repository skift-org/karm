module;

#include <karm/macros>

export module Karm.Ipc:protocol;

import Karm.Sys;
import :message;

namespace Karm::Ipc {

export template <typename T>
Res<> send(Sys::IpcConnection& con, u64 seq, T const& payload) {
    auto msg = try$(Message::packReq<T>(seq, payload));
    try$(con.send(msg->bytes(), msg->handles()));
    return Ok();
}

export Async::Task<Box<Message>> recvAsync(Sys::IpcConnection& con, Async::CancellationToken ct) {
    Box<Message> msg = makeBox<Message>();
    auto [bufLen, hndsLen] = co_trya$(con.recvAsync(msg->_buf, msg->_hnds, ct));
    if (bufLen < sizeof(Header))
        co_return Error::invalidData("invalid message");
    msg->_len = bufLen;
    msg->_hndsLen = hndsLen;
    co_return msg;
}

} // namespace Karm::Ipc
