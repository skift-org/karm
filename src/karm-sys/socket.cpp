module;

#include <karm/macros>

export module Karm.Sys:socket;

import :_embed;
import :addr;
import :async;
import :fd;
import :proc;

namespace Karm::Sys {

// MARK: Abstract Socket -------------------------------------------------------

export struct _Connection :
    Io::Reader,
    Io::Writer,
    Io::Flusher,
    Aio::Writer,
    Aio::Reader,
    Meta::NoCopy {

    virtual Async::Task<> flushAsync(Async::CancellationToken ct) = 0;
};

export struct Connection :
    _Connection {

    Rc<Fd> _fd;

    Connection(Rc<Fd> fd)
        : _fd(std::move(fd)) {}

    Res<usize> read(MutBytes buf) override {
        return _fd->read(buf);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> readAsync(MutBytes buf, Async::CancellationToken ct) override {
        return globalSched().readAsync(_fd, buf, ct);
    }

    Res<usize> write(Bytes buf) override {
        return _fd->write(buf);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> writeAsync(Bytes buf, Async::CancellationToken ct) override {
        return globalSched().writeAsync(_fd, buf, ct);
    }

    Res<> flush() override {
        return _fd->flush();
    }

    [[clang::coro_wrapper]]
    Async::Task<> flushAsync(Async::CancellationToken ct) override {
        return globalSched().flushAsync(_fd, ct);
    }

    Rc<Fd> fd() { return _fd; }
};

template <typename C>
struct _Listener :
    Meta::NoCopy {

    Rc<Fd> _fd;

    _Listener(Rc<Fd> fd)
        : _fd(std::move(fd)) {}

    Res<C> accept() {
        return Ok(C(try$(_fd->accept())));
    }

    Async::Task<C> acceptAsync(Async::CancellationToken ct) {
        co_return Ok(C(co_trya$(globalSched().acceptAsync(_fd, ct))));
    }

    Rc<Fd> fd() { return _fd; }
};

// MARK: Udp Socket ------------------------------------------------------------

export struct UdpConnection :
    Meta::NoCopy {

    Rc<Fd> _fd;
    SocketAddr _addr;

    static Res<UdpConnection> listen(SocketAddr addr) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::listenUdp(addr));
        return Ok(UdpConnection({std::move(fd), addr}));
    }

    UdpConnection(_Accepted accepted)
        : _fd(std::move(accepted.fd)), _addr(accepted.addr) {}

    Res<usize> send(Bytes buf, SocketAddr addr) {
        auto [nbytes, _] = try$(_fd->send(buf, {}, addr));
        return Ok(nbytes);
    }

    [[clang::coro_wrapper]]
    Async::Task<_Sent> sendAsync(Bytes buf, SocketAddr addr, Async::CancellationToken ct) {
        return globalSched().sendAsync(_fd, buf, {}, addr, ct);
    }

    Res<Pair<usize, SocketAddr>> recv(MutBytes buf) {
        auto [nbytes, _, addr] = try$(_fd->recv(buf, {}));
        return Ok<Pair<usize, SocketAddr>>(nbytes, addr);
    }

    [[clang::coro_wrapper]]
    Async::Task<_Received> recvAsync(MutBytes buf, Async::CancellationToken ct) {
        return globalSched().recvAsync(_fd, buf, {}, ct);
    }
};

// MARK: Tcp Socket ------------------------------------------------------------

export struct TcpConnection :
    Connection {

    SocketAddr _addr;

    static Res<TcpConnection> connect(SocketAddr addr) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::connectTcp(addr));
        return Ok(TcpConnection({std::move(fd), addr}));
    }

    TcpConnection(_Accepted accepted)
        : Connection(std::move(accepted.fd)), _addr(accepted.addr) {}

    SocketAddr addr() const {
        return _addr;
    }
};

export struct TcpListener :
    _Listener<TcpConnection> {

    SocketAddr _addr;

    static Res<TcpListener> listen(SocketAddr addr) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::listenTcp(addr));
        return Ok(TcpListener(std::move(fd), addr));
    }

    TcpListener(Rc<Fd> fd, SocketAddr addr)
        : _Listener(std::move(fd)), _addr(addr) {}

    SocketAddr addr() const {
        return _addr;
    }
};

// MARK: Ipc Socket -----------------------------------------------------------

export struct IpcConnection {
    Rc<Fd> _fd;
    bool _brokered;

    static Res<IpcConnection> connect(Ref::Url url) {
        auto connected = try$(_Embed::connectIpc(url));
        return Ok(IpcConnection{std::move(connected.fd), connected.brokered});
    }

    IpcConnection(Rc<Fd> fd, bool brokered = false)
        : _fd(std::move(fd)), _brokered(brokered) {}

    // Whether a broker established this connection and delivered the target
    // url to the server on our behalf; if not, we are expected to introduce
    // ourselves with an in-channel hello.
    bool brokered() const {
        return _brokered;
    }

    Res<> send(Bytes buf, Slice<Handle> hnds) {
        try$(_fd->send(buf, hnds, {Ip4::unspecified(), 0}));
        return Ok();
    }

    Res<Pair<usize>> recv(MutBytes buf, MutSlice<Handle> hnds) {
        auto [nbytes, nhnds, _] = try$(_fd->recv(buf, hnds));
        return Ok<Pair<usize>>(nbytes, nhnds);
    }

    Async::Task<> sendAsync(Bytes buf, Slice<Handle> hnds, Async::CancellationToken ct) {
        co_trya$(globalSched().sendAsync(_fd, buf, hnds, Ip4::unspecified(0), ct));
        co_return Ok();
    }

    Async::Task<Pair<usize>> recvAsync(MutBytes buf, MutSlice<Handle> hnds, Async::CancellationToken ct) {
        auto [nbytes, nhnds, _] = co_trya$(globalSched().recvAsync(_fd, buf, hnds, ct));
        co_return Ok<Pair<usize>>(nbytes, nhnds);
    }
};

export struct IpcAccepted {
    IpcConnection connection;

    // Target url pre-verified by a broker; absent on raw transports, where
    // the server expects an in-channel hello instead.
    Opt<Ref::Url> url;
};

export struct IpcListener :
    Meta::NoCopy {

    Rc<Fd> _fd;

    static Res<IpcListener> listen(Ref::Url url) {
        try$(ensureUnrestricted());
        auto fd = try$(_Embed::listenIpc(url));
        return Ok(IpcListener(std::move(fd)));
    }

    IpcListener(Rc<Fd> fd)
        : _fd(std::move(fd)) {}

    Async::Task<IpcAccepted> acceptAsync(Async::CancellationToken ct) {
        auto accepted = co_trya$(globalSched().acceptAsync(_fd, ct));
        bool brokered = accepted.url.has();
        co_return Ok(IpcAccepted{{std::move(accepted.fd), brokered}, std::move(accepted.url)});
    }

    Rc<Fd> fd() { return _fd; }
};

} // namespace Karm::Sys
