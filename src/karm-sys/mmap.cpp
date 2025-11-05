module;

#include <karm-core/macros.h>

export module Karm.Sys:mmap;

import Karm.Core;

import :_embed;
import :types;
import :fd;

namespace Karm::Sys {

export struct Mmap :
    Meta::NoCopy {
    using enum MmapOption;

    usize _paddr{};
    void const* _buf{};
    usize _size{};
    bool _owned{true};

    static Res<Mmap> createUnowned(void const* buf, usize size) {
        return Ok(Mmap{0, buf, size, false});
    }

    Mmap(usize paddr, void const* buf, usize size, bool owned = true)
        : _paddr(paddr), _buf(buf), _size(size), _owned(owned) {}

    Mmap(Mmap&& other) {
        std::swap(_paddr, other._paddr);
        std::swap(_buf, other._buf);
        std::swap(_size, other._size);
    }

    Mmap& operator=(Mmap&& other) {
        std::swap(_paddr, other._paddr);
        std::swap(_buf, other._buf);
        std::swap(_size, other._size);
        return *this;
    }

    ~Mmap() {
        unmap().unwrap("unmap failed");
    }

    Res<> unmap() {
        if (_buf and _owned) {
            try$(_Embed::memUnmap(std::exchange(_buf, nullptr), _size));
            _paddr = 0;
            _size = 0;
        }
        return Ok();
    }

    usize vaddr() const { return reinterpret_cast<usize>(_buf); }

    usize paddr() const { return _paddr; }

    urange vrange() const { return {vaddr(), _size}; }

    urange prange() const { return {_paddr, _size}; }

    template <typename T>
    T const* as() const {
        return static_cast<T const*>(_buf);
    }

    template <typename T>
    Cursor<T> cursor() const {
        return Cursor<T>{(T*)_buf, _size / sizeof(T)};
    }

    Bytes bytes() const {
        return {static_cast<u8 const*>(_buf), _size};
    }

    void leak() {
        _owned = false;
    }
};

export struct MutMmap :
    Io::Flusher,
    Meta::NoCopy {
    using enum MmapOption;

    usize _paddr{};
    void* _buf{};
    usize _size{};
    bool _owned{true};

    static Res<MutMmap> createUnowned(void* buf, usize size) {
        return Ok(MutMmap{0, buf, size, false});
    }

    MutMmap(usize paddr, void* buf, usize size, bool owned = true)
        : _paddr(paddr), _buf(buf), _size(size), _owned(owned) {
    }

    Res<> flush() override {
        try$(_Embed::memFlush(_buf, _size));
        return Ok();
    }

    MutMmap(MutMmap&& other) {
        std::swap(_paddr, other._paddr);
        std::swap(_buf, other._buf);
        std::swap(_size, other._size);
    }

    MutMmap& operator=(MutMmap&& other) {
        std::swap(_paddr, other._paddr);
        std::swap(_buf, other._buf);
        std::swap(_size, other._size);
        return *this;
    }

    ~MutMmap() {
        unmap().unwrap("unmap failed");
    }

    Res<> unmap() {
        if (_buf and _owned) {
            try$(_Embed::memUnmap(std::exchange(_buf, nullptr), _size));
            _paddr = 0;
            _buf = nullptr;
            _size = 0;
        }
        return Ok();
    }

    usize vaddr() const {
        return (usize)_buf;
    }

    usize paddr() const {
        return _paddr;
    }

    urange vrange() const {
        return {(usize)_buf, _size};
    }

    urange prange() const {
        return {_paddr, _size};
    }

    template <typename T>
    T const* as() const {
        return static_cast<T const*>(_buf);
    }

    template <typename T>
    T* as() {
        return static_cast<T*>(_buf);
    }

    template <typename T>
    Cursor<T> cursor() const {
        return Cursor<T>{(T*)_buf, _size / sizeof(T)};
    }

    template <typename T>
    MutCursor<T> mutCursor() {
        return Cursor<T>{(T*)_buf, _size / sizeof(T)};
    }

    Bytes bytes() const {
        return {static_cast<u8 const*>(_buf), _size};
    }

    MutBytes mutBytes() {
        return {static_cast<u8*>(_buf), _size};
    }

    void leak() {
        _owned = false;
    }

    Mmap seal() {
        Mmap mmap{_paddr, _buf, _size, _owned};
        leak();
        return mmap;
    }
};

export usize pageSize() {
    return _Embed::pageSize();
}

export Res<Mmap> mmap(Opt<Rc<Fd>> fd = NONE, MmapProps props = {}) {
    props.options |= MmapOption::READ;
    MmapResult result;
    if (fd)
        result = try$(_Embed::memMap(props, fd.unwrap()));
    else
        result = try$(_Embed::memMap(props));
    return Ok(Mmap{result.paddr, (void*)result.vaddr, result.size});
}

export Res<Mmap> mmap(AsFd auto& what, MmapProps props = {}) {
    return mmap(what.fd(), props);
}

export Res<MutMmap> mutMmap(Opt<Rc<Fd>> fd = NONE, MmapProps props = {}) {
    props.options |= MmapOption::WRITE;
    MmapResult result;
    if (fd)
        result = try$(_Embed::memMap(props, fd.unwrap()));
    else
        result = try$(_Embed::memMap(props));
    return Ok(MutMmap{result.paddr, (void*)result.vaddr, result.size});
}

export Res<MutMmap> mutMmap(AsFd auto& what, MmapProps props = {}) {
    return mutMmap(what.fd(), props);
}

} // namespace Karm::Sys
