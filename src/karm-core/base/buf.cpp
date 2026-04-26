module;

#include <karm/macros>

export module Karm.Core:base.buf;

import :base.array;
import :base.clamp;
import :base.manual;

namespace Karm {

#pragma clang unsafe_buffer_usage begin

/// A dynamically sized array of elements.
/// Often used as a backing store for other data structures. (e.g. `Vec`)
export template <typename T>
struct Buf {
    using Inner = T;

    usize _cap{};
    usize _len{};
    Manual<T>* _buf{};

    static Buf init(usize len, T fill = {}) {
        Buf buf;
        buf.ensure(len);

        buf._len = len;
        for (usize i = 0; i < len; i++)
            buf._buf[i].ctor(fill);
        return buf;
    }

    Buf(usize cap = 0) {
        ensure(cap);
    }

    Buf(Move, T* buf, usize len)
        : _cap(len),
          _len(len),
          _buf(reinterpret_cast<Manual<T>*>(buf)) {
    }

    Buf(std::initializer_list<T> other) {
        ensure(other.size());

        _len = other.size();
        for (usize i = 0; i < _len; i++)
            _buf[i].ctor(std::move(other.begin()[i]));
    }

    Buf(Sliceable<T> auto const& other) {
        ensure(other.len());

        _len = other.len();
        for (usize i = 0; i < _len; i++)
            _buf[i].ctor(other[i]);
    }

    Buf(Buf const& other) {
        ensure(other._len);

        _len = other._len;
        for (usize i = 0; i < _len; i++)
            _buf[i].ctor(other[i]);
    }

    Buf(Buf&& other) {
        std::swap(_buf, other._buf);
        std::swap(_cap, other._cap);
        std::swap(_len, other._len);
    }

    ~Buf() {
        if (not _buf)
            return;
        for (usize i = 0; i < _len; i++)
            _buf[i].dtor();
        delete[] _buf;
    }

    Buf& operator=(Buf const& other) {
        *this = Buf(other);
        return *this;
    }

    Buf& operator=(Buf&& other) {
        std::swap(_buf, other._buf);
        std::swap(_cap, other._cap);
        std::swap(_len, other._len);
        return *this;
    }

    constexpr T& operator[](usize i) lifetimebound {
        return _buf[i].unwrap();
    }

    constexpr T const& operator[](usize i) const lifetimebound {
        return _buf[i].unwrap();
    }

    void ensure(usize desired) {
        if (desired <= _cap)
            return;

        if (not _buf) {
            _buf = new Manual<T>[desired];
            _cap = desired;
            return;
        }

        usize newCap = max(_cap * 2, desired);

        Manual<T>* tmp = new Manual<T>[newCap];
        for (usize i = 0; i < _len; i++) {
            tmp[i].ctor(_buf[i].take());
        }

        delete[] _buf;
        _buf = tmp;
        _cap = newCap;
    }

    void fit() {
        if (_len == _cap)
            return;

        Manual<T>* tmp = nullptr;

        if (_len) {
            tmp = new Manual<T>[_len];
            for (usize i = 0; i < _len; i++)
                tmp[i].ctor(_buf[i].take());
        }

        delete[] _buf;
        _buf = tmp;
        _cap = _len;
    }

    template <typename... Args>
    auto& emplace(usize index, Args&&... args) {
        ensure(_len + 1);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - 1].take());
        }

        _buf[index].ctor(std::forward<Args>(args)...);
        _len++;
        return _buf[index].unwrap();
    }

    void insert(usize index, T&& value) {
        ensure(_len + 1);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - 1].take());
        }

        _buf[index].ctor(std::move(value));
        _len++;
    }

    void replace(usize index, T&& value) {
        if (index >= _len) {
            insert(index, std::move(value));
            return;
        }

        _buf[index].dtor();
        _buf[index].ctor(std::move(value));
    }

    void insert(Copy, usize index, T const* first, usize count) {
        ensure(_len + count);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - count].take());
        }

        for (usize i = 0; i < count; i++) {
            _buf[index + i].ctor(first[i]);
        }

        _len += count;
    }

    void insert(Move, usize index, T* first, usize count) {
        ensure(_len + count);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - count].take());
        }

        for (usize i = 0; i < count; i++) {
            _buf[index + i].ctor(std::move(first[i]));
        }

        _len += count;
    }

    T removeAt(usize index) {
        if (index >= _len) [[unlikely]]
            panic("index out of bounds");

        T ret = _buf[index].take();
        for (usize i = index; i < _len - 1; i++) {
            _buf[i].ctor(_buf[i + 1].take());
        }
        _len--;
        return ret;
    }

    void removeRange(usize index, usize count) {
        if (index > _len) [[unlikely]]
            panic("index out of bounds");

        if (index + count > _len) [[unlikely]]
            panic("index + count out of bounds");

        for (usize i = index; i < _len - count; i++)
            _buf[i].ctor(_buf[i + count].take());

        _len -= count;
    }

    void resize(usize newLen, T fill = {}) {
        if (newLen > _len) {
            ensure(newLen);
            for (usize i = _len; i < newLen; i++) {
                _buf[i].ctor(fill);
            }
        } else if (newLen < _len) {
            for (usize i = newLen; i < _len; i++) {
                _buf[i].dtor();
            }
        }
        _len = newLen;
    }

    void trunc(usize newLen) {
        if (newLen >= _len)
            return;

        for (usize i = newLen; i < _len; i++) {
            _buf[i].dtor();
        }

        _len = newLen;
    }

    T* take() {
        T* ret = buf();
        _buf = nullptr;
        _cap = 0;
        _len = 0;

        return ret;
    }

    T* buf() lifetimebound {
        if (_buf == nullptr)
            return nullptr;
        return &_buf->unwrap();
    }

    T const* buf() const lifetimebound {
        if (_buf == nullptr)
            return nullptr;

        return &_buf->unwrap();
    }

    usize len() const {
        return _len;
    }

    usize cap() const {
        return _cap;
    }

    usize size() const {
        return _len * sizeof(T);
    }

    void leak() {
        _buf = nullptr;
        _cap = 0;
        _len = 0;
    }
};

export template <typename T>
struct Niche<Buf<T>> {
    struct Content {
        usize _cap;
        usize _len;
        char const* ptr;

        always_inline constexpr Content() : ptr(NICHE_PTR) {}

        always_inline constexpr bool has() const {
            return ptr != NICHE_PTR;
        }
    };
};

static_assert(offsetof(Buf<int>, _buf) == offsetof(Niche<Buf<int>>::Content, ptr));

/// A buffer that uses inline storage, great for small buffers.
export template <typename T, usize N>
struct InlineBuf {
    using Inner = T;

    usize _len = {};
    Array<Manual<T>, N> _buf = {};

    constexpr InlineBuf() = default;

    InlineBuf(usize cap) {
        if (cap > N) [[unlikely]]
            panic("cap too large");
    }

    InlineBuf(T const* buf, usize len) {
        if (len > N) [[unlikely]]
            panic("len too large");

        _len = len;
        for (usize i = 0; i < _len; i++)
            _buf[i].ctor(buf[i]);
    }

    InlineBuf(std::initializer_list<T> other)
        : InlineBuf(other.begin(), other.size()) {
    }

    InlineBuf(Sliceable<T> auto const& other)
        : InlineBuf(other.buf(), other.len()) {
    }

    InlineBuf(InlineBuf const& other)
        : InlineBuf(other.buf(), other.len()) {
    }

    InlineBuf(InlineBuf&& other) {
        for (usize i = 0; i < N; i++) {
            _buf[i].ctor(std::move(other._buf[i].take()));
        }
        _len = other._len;
    }

    ~InlineBuf() {
        for (usize i = 0; i < _len; i++) {
            _buf[i].dtor();
        }
        _len = 0;
    }

    InlineBuf& operator=(InlineBuf const& other) {
        *this = InlineBuf(other);
        return *this;
    }

    InlineBuf& operator=(InlineBuf&& other) {
        for (usize i = 0; i < min(_len, other._len); i++)
            buf()[i] = std::move(other.buf()[i]);

        for (usize i = _len; i < other._len; i++)
            _buf[i].ctor(std::move(other._buf[i].take()));

        for (usize i = other._len; i < _len; i++)
            _buf[i].dtor();

        _len = other._len;

        return *this;
    }

    constexpr T& operator[](usize i) lifetimebound {
        return _buf[i].unwrap();
    }

    constexpr T const& operator[](usize i) const lifetimebound {
        return _buf[i].unwrap();
    }

    void ensure(usize len) {
        if (len > N) [[unlikely]]
            panic("cap too large");
    }

    void fit() {
        // no-op
    }

    template <typename... Args>
    void emplace(usize index, Args&&... args) {
        if (_len == N) [[unlikely]]
            panic("cap too large");

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - 1].take());
        }

        _buf[index].ctor(std::forward<Args>(args)...);
        _len++;
    }

    void insert(usize index, T&& value) {
        if (_len == N) [[unlikely]]
            panic("cap too large");

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - 1].take());
        }

        _buf[index].ctor(std::move(value));
        _len++;
    }

    void insert(Copy, usize index, T* first, usize count) {
        if (_len + count > N) [[unlikely]]
            panic("cap too large");

        for (usize i = _len; i > index; i--) {
            _buf[i] = _buf[i - count];
        }

        for (usize i = 0; i < count; i++) {
            _buf[index + i] = first[i];
        }

        _len += count;
    }

    void insert(Move, usize index, T* first, usize count) {
        if (_len + count > N) [[unlikely]]
            panic("cap too large");

        for (usize i = _len; i > index; i--) {
            _buf[i] = std::move<T>(_buf[i - count]);
        }

        for (usize i = 0; i < count; i++) {
            _buf[index + i] = std::move(first[i]);
        }

        _len += count;
    }

    void replace(usize index, T&& value) {
        if (index >= _len) {
            insert(index, std::move(value));
            return;
        }

        _buf[index].dtor();
        _buf[index].ctor(std::move(value));
    }

    T removeAt(usize index) {
        T tmp = _buf[index].take();
        for (usize i = index; i < _len - 1; i++) {
            _buf[i].ctor(_buf[i + 1].take());
        }
        _len--;
        return tmp;
    }

    void removeRange(usize index, usize count) {
        if (index > _len) [[unlikely]]
            panic("index out of bounds");

        if (index + count > _len) [[unlikely]]
            panic("index + count out of bounds");

        for (usize i = index; i < index + count; i++) {
            _buf[i].dtor();
        }

        for (usize i = index; i < _len - count; i++) {
            _buf[i].ctor(_buf[i + count].take());
        }

        _len -= count;
    }

    void resize(usize newLen, T fill = {}) {
        if (newLen > _len) {
            ensure(newLen);
            for (usize i = _len; i < newLen; i++) {
                _buf[i].ctor(fill);
            }
        } else if (newLen < _len) {
            for (usize i = newLen; i < _len; i++) {
                _buf[i].dtor();
            }
        }
        _len = newLen;
    }

    void trunc(usize newLen) {
        if (newLen >= _len)
            return;

        for (usize i = newLen; i < _len; i++) {
            _buf[i].dtor();
        }

        _len = newLen;
    }

    T* buf() lifetimebound {
        return &_buf[0].unwrap();
    }

    T const* buf() const lifetimebound {
        return &_buf[0].unwrap();
    }

    usize len() const {
        return _len;
    }

    usize cap() const {
        return N;
    }
};

export template <typename T, usize N>
union SmallBuf {
    using Inner = T;

    InlineBuf<T, N> _inlineBuf;
    Buf<T> _heapBuf;

    SmallBuf() : _inlineBuf() {}

    SmallBuf(usize cap) {
        if (cap > N) {
            new (&_heapBuf) Buf<T>(cap);
        } else {
            new (&_inlineBuf) InlineBuf<T, N>();
        }
    }

    SmallBuf(std::initializer_list<T> other) {
        if (other.size() > N) {
            new (&_heapBuf) Buf<T>(other);
        } else {
            new (&_inlineBuf) InlineBuf<T, N>(other);
        }
    }

    SmallBuf(Sliceable<T> auto const& other) {
        if (other.len() > N) {
            new (&_heapBuf) Buf<T>(other);
        } else {
            new (&_inlineBuf) InlineBuf<T, N>(other);
        }
    }

    SmallBuf(SmallBuf const& other) {
        if (other.spilled()) {
            new (&_heapBuf) Buf<T>(other._heapBuf);
        } else {
            new (&_inlineBuf) InlineBuf<T, N>(other._inlineBuf);
        }
    }

    SmallBuf(SmallBuf&& other) {
        if (other.spilled()) {
            new (&_heapBuf) Buf<T>(std::move(other._heapBuf));
        } else {
            new (&_inlineBuf) InlineBuf<T, N>(std::move(other._inlineBuf));
        }
    }

    ~SmallBuf() {
        if (spilled())
            _heapBuf.~Buf();
        else
            _inlineBuf.~InlineBuf();
    }

    SmallBuf& operator=(SmallBuf const& other) {
        if (this == &other)
            return *this;

        this->~SmallBuf();
        if (other.spilled()) {
            new (&_heapBuf) Buf<T>(other._heapBuf);
        } else {
            new (&_inlineBuf) InlineBuf<T, N>(other._inlineBuf);
        }
        return *this;
    }

    SmallBuf& operator=(SmallBuf&& other) {
        if (this == &other)
            return *this;

        this->~SmallBuf();
        if (other.spilled()) {
            new (&_heapBuf) Buf<T>(std::move(other._heapBuf));
        } else {
            new (&_inlineBuf) InlineBuf<T, N>(std::move(other._inlineBuf));
        }
        return *this;
    }

    bool spilled() const {
        return _heapBuf._cap > N;
    }

    void spill() {
        if (spilled())
            return;

        Buf<T> newBuf{};
        newBuf.ensure(N * 2);

        for (usize i = 0; i < _inlineBuf.len(); i++) {
            newBuf.emplace(i, std::move(_inlineBuf[i]));
        }

        _inlineBuf.~InlineBuf();
        new (&_heapBuf) Buf<T>(std::move(newBuf));
    }

    constexpr T& operator[](usize i) lifetimebound {
        return spilled() ? _heapBuf[i] : _inlineBuf[i];
    }

    constexpr T const& operator[](usize i) const lifetimebound {
        return spilled() ? _heapBuf[i] : _inlineBuf[i];
    }

    void ensure(usize cap) {
        if (spilled()) {
            _heapBuf.ensure(cap);
        } else if (cap > N) {
            spill();
            _heapBuf.ensure(cap);
        }
    }

    void fit() {
        if (spilled())
            _heapBuf.fit();
    }

    template <typename... Args>
    void emplace(usize index, Args&&... args) {
        ensure(len() + 1);
        if (spilled()) {
            _heapBuf.emplace(index, std::forward<Args>(args)...);
        } else {
            _inlineBuf.emplace(index, std::forward<Args>(args)...);
        }
    }

    void insert(usize index, T&& value) {
        ensure(len() + 1);
        if (spilled()) {
            _heapBuf.insert(index, std::move(value));
        } else {
            _inlineBuf.insert(index, std::move(value));
        }
    }

    void replace(usize index, T&& value) {
        if (spilled()) {
            _heapBuf.replace(index, std::move(value));
        } else {
            _inlineBuf.replace(index, std::move(value));
        }
    }

    void insert(Copy, usize index, T const* first, usize count) {
        ensure(len() + count);
        if (spilled()) {
            _heapBuf.insert(Copy{}, index, first, count);
        } else {
            _inlineBuf.insert(Copy{}, index, first, count);
        }
    }

    void insert(Move, usize index, T* first, usize count) {
        ensure(len() + count);
        if (spilled()) {
            _heapBuf.insert(Move{}, index, first, count);
        } else {
            _inlineBuf.insert(Move{}, index, first, count);
        }
    }

    T removeAt(usize index) {
        return spilled() ? _heapBuf.removeAt(index) : _inlineBuf.removeAt(index);
    }

    void removeRange(usize index, usize count) {
        if (spilled()) {
            _heapBuf.removeRange(index, count);
        } else {
            _inlineBuf.removeRange(index, count);
        }
    }

    void resize(usize newLen, T fill = {}) {
        ensure(newLen);
        if (spilled()) {
            _heapBuf.resize(newLen, fill);
        } else {
            _inlineBuf.resize(newLen, fill);
        }
    }

    void trunc(usize newLen) {
        if (spilled()) {
            _heapBuf.trunc(newLen);
        } else {
            _inlineBuf.trunc(newLen);
        }
    }

    T* buf() lifetimebound {
        return spilled() ? _heapBuf.buf() : _inlineBuf.buf();
    }

    T const* buf() const lifetimebound {
        return spilled() ? _heapBuf.buf() : _inlineBuf.buf();
    }

    usize len() const {
        return spilled() ? _heapBuf.len() : _inlineBuf.len();
    }

    usize cap() const {
        return spilled() ? _heapBuf.cap() : _inlineBuf.cap();
    }

    usize size() const {
        return len() * sizeof(T);
    }
};

/// A buffer that does not own its backing storage.
export template <typename T>
struct ViewBuf {
    using Inner = T;

    usize _cap{};
    usize _len{};
    Manual<T>* _buf{};

    ViewBuf() = default;

    ViewBuf(Manual<T>* buf, usize cap)
        : _cap(cap), _buf(buf) {
    }

    ViewBuf(ViewBuf const& other) {
        _cap = other._cap;
        _len = other._len;
        _buf = other._buf;
    }

    ViewBuf(ViewBuf&& other) {
        std::swap(_buf, other._buf);
        std::swap(_cap, other._cap);
        std::swap(_len, other._len);
    }

    ~ViewBuf() {}

    ViewBuf& operator=(ViewBuf const& other) {
        *this = ViewBuf(other);
        return *this;
    }

    ViewBuf& operator=(ViewBuf&& other) {
        std::swap(_buf, other._buf);
        std::swap(_cap, other._cap);
        std::swap(_len, other._len);
        return *this;
    }

    constexpr T& operator[](usize i) {
        return _buf[i].unwrap();
    }

    constexpr T const& operator[](usize i) const {
        return _buf[i].unwrap();
    }

    void ensure(usize cap) {
        if (cap > _cap) [[unlikely]]
            panic("cap too large");
    }

    void fit() {
    }

    template <typename... Args>
    void emplace(usize index, Args&&... args) {
        ensure(_len + 1);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - 1].take());
        }
        _buf[index].ctor(std::forward<Args>(args)...);
        _len++;
    }

    void insert(usize index, T&& value) {
        ensure(_len + 1);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - 1].take());
        }

        _buf[index].ctor(std::move(value));
        _len++;
    }

    void replace(usize index, T&& value) {
        if (index >= _len) {
            insert(index, std::move(value));
            return;
        }

        _buf[index].dtor();
        _buf[index].ctor(std::move(value));
    }

    void insert(Copy, usize index, T* first, usize count) {
        ensure(_len + count);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - count].take());
        }

        for (usize i = 0; i < count; i++) {
            _buf[index + i].ctor(first[i]);
        }

        _len += count;
    }

    void insert(Move, usize index, T* first, usize count) {
        ensure(_len + count);

        for (usize i = _len; i > index; i--) {
            _buf[i].ctor(_buf[i - count].take());
        }

        for (usize i = 0; i < count; i++) {
            _buf[index + i].ctor(std::move(first[i]));
        }

        _len += count;
    }

    T removeAt(usize index) {
        if (index >= _len) [[unlikely]]
            panic("index out of bounds");

        T ret = _buf[index].take();
        for (usize i = index; i < _len - 1; i++) {
            _buf[i].ctor(_buf[i + 1].take());
        }
        _len--;
        return ret;
    }

    void removeRange(usize index, usize count) {
        if (index > _len) [[unlikely]]
            panic("index out of bounds");

        if (index + count > _len) [[unlikely]]
            panic("index + count out of bounds");

        for (usize i = index; i < _len - count; i++)
            _buf[i].ctor(_buf[i + count].take());

        _len -= count;
    }

    void resize(usize newLen, T fill = {}) {
        if (newLen > _len) {
            ensure(newLen);
            for (usize i = _len; i < newLen; i++) {
                _buf[i].ctor(fill);
            }
        } else if (newLen < _len) {
            for (usize i = newLen; i < _len; i++) {
                _buf[i].dtor();
            }
        }
        _len = newLen;
    }

    void trunc(usize newLen) {
        if (newLen >= _len)
            return;

        for (usize i = newLen; i < _len; i++) {
            _buf[i].dtor();
        }

        _len = newLen;
    }

    T* take() {
        T* ret = buf();
        _buf = nullptr;
        _cap = 0;
        _len = 0;

        return ret;
    }

    T* buf() {
        if (_buf == nullptr)
            return nullptr;

        return &_buf->unwrap();
    }

    T const* buf() const {
        if (_buf == nullptr)
            return nullptr;

        return &_buf->unwrap();
    }

    usize len() const {
        return _len;
    }

    usize cap() const {
        return _cap;
    }

    usize size() const {
        return _len * sizeof(T);
    }
};

export template <typename T>
struct Niche<ViewBuf<T>> {
    struct Content : Niche<Buf<T>>::Content {};
};

static_assert(offsetof(ViewBuf<int>, _buf) == offsetof(Niche<ViewBuf<int>>::Content, ptr));

#pragma clang unsafe_buffer_usage end

} // namespace Karm
