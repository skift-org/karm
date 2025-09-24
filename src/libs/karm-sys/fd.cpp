export module Karm.Sys:fd;

import Karm.Core;
import Karm.Ref;

import :addr;
import :message;
import :stat;
import :types;

namespace Karm::Sys {

export struct Fd;

export using _Accepted = Pair<Rc<Fd>, SocketAddr>;
export using _Sent = Pair<usize, usize>;
export using _Received = Tuple<usize, usize, SocketAddr>;

export struct Fd : Meta::NoCopy {
    virtual ~Fd() = default;

    virtual Handle handle() const = 0;

    virtual Res<usize> read(MutBytes) = 0;

    virtual Res<usize> write(Bytes) = 0;

    virtual Res<usize> seek(Io::Seek) = 0;

    virtual Res<> flush() = 0;

    virtual Res<Rc<Fd>> dup() = 0;

    virtual Res<_Accepted> accept() = 0;

    virtual Res<Stat> stat() = 0;

    virtual Res<_Sent> send(Bytes, Slice<Handle>, SocketAddr) = 0;

    virtual Res<_Received> recv(MutBytes, MutSlice<Handle>) = 0;

    virtual Res<> pack(MessageWriter& e) = 0;

    static Res<Rc<Fd>> unpack(MessageReader& s)  {
        return _Embed::unpackFd(s);
    }
};

export struct BlobFd : Fd {
    Rc<Ref::Blob> _blob;
    usize _offset;

    BlobFd(Rc<Ref::Blob> const& blob)
        : _blob(blob) {}

    Handle handle() const override {
        return INVALID;
    }

    Res<usize> read(MutBytes bytes) override {
        auto src = sub(_blob->data, _offset, _offset + bytes.len());
        return Ok(copy(src, bytes));
    }

    Res<usize> write(Bytes) override {
        return Error::readOnlyFilesystem();
    }

    Res<usize> seek(Io::Seek seek) override {
        _offset = seek.apply(_offset, _blob->len());
        return Ok(_offset);
    }

    Res<> flush() override {
        return Ok();
    }

    Res<Rc<Fd>> dup() override {
        return Ok(makeRc<BlobFd>(_blob));
    }

    Res<_Accepted> accept() override {
        return Error::notImplemented();
    }

    Res<Stat> stat() override {
        return Ok(Stat{
            .type = Type::FILE,
            .size = _blob->data.len(),
            .accessTime = SystemTime::END_OF_TIME,
            .modifyTime = SystemTime::END_OF_TIME,
            .changeTime = SystemTime::END_OF_TIME,
        });
    }

    Res<_Sent> send(Bytes, Slice<Handle>, SocketAddr) override {
        return Error::notImplemented();
    }

    Res<_Received> recv(MutBytes, MutSlice<Handle>) override {
        return Error::notImplemented();
    }

    Res<> pack(MessageWriter&) override {
        return Error::notImplemented();
    }
};

export struct NullFd : Fd {
    Handle handle() const override {
        return INVALID;
    }

    Res<usize> read(MutBytes) override  {
        return Ok(0uz);
    }

    Res<usize> write(Bytes) override {
        return Ok(0uz);
    }

    Res<usize> seek(Io::Seek) override {
        return Ok(0uz);
    }

    Res<> flush() override {
        return Ok();
    }

    Res<Rc<Fd>> dup() override {
        return Ok(makeRc<NullFd>());
    }

    Res<_Accepted> accept() override {
        return Ok<_Accepted>(
            makeRc<NullFd>(),
            Ip4::unspecified(0)
        );
    }

    Res<Stat> stat() override  {
        return Ok(Stat{});
    }

    Res<_Sent> send(Bytes, Slice<Handle>, SocketAddr) override {
        return Ok<_Sent>(0uz, 0uz);
    }

    Res<_Received> recv(MutBytes, MutSlice<Handle>) override  {
        return Ok<_Received>(0uz, 0uz, Ip4::unspecified(0));
    }

    Res<> pack(MessageWriter& e) override  {
        return Sys::pack(e, INVALID);
    }
};

export template <typename T>
concept AsFd = requires(T t) {
    { t.fd() } -> Meta::Same<Rc<Fd>>;
};

} // namespace Karm::Sys
