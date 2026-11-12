export module Karm.Core:base.ring;

import :base.limits;
import :base.manual;
import :base.panic;
import :base.slice;

namespace Karm {

export template <typename T>
struct Ring {
    Manual<T>* _buf{};
    usize _cap{};                     //< Number of slots allocated, at most _limit
    usize _limit{Limits<usize>::MAX}; //< Maximum number of elements
    usize _len{};
    usize _head{};
    usize _tail{};

    constexpr Ring() = default;

    Ring(usize limit)
        : _limit(limit) {
    }

    Ring(Ring const& other)
        : _limit(other._limit) {
        _ensure(other._len);
        for (usize i = 0; i < other._len; i++)
            pushBack(other.peekFront(i));
    }

    Ring(Ring&& other)
        : _buf(std::exchange(other._buf, nullptr)),
          _cap(std::exchange(other._cap, 0)),
          _limit(std::exchange(other._limit, Limits<usize>::MAX)),
          _len(std::exchange(other._len, 0)),
          _head(std::exchange(other._head, 0)),
          _tail(std::exchange(other._tail, 0)) {
    }

    ~Ring() {
        for (usize i = 0; i < _len; i++)
            _buf[(_tail + i) % _cap].dtor();
        delete[] _buf;
    }

    Ring& operator=(Ring const& other) {
        *this = Ring(other);
        return *this;
    }

    Ring& operator=(Ring&& other) {
        std::swap(_buf, other._buf);
        std::swap(_cap, other._cap);
        std::swap(_limit, other._limit);
        std::swap(_len, other._len);
        std::swap(_head, other._head);
        std::swap(_tail, other._tail);
        return *this;
    }

    void _ensure(usize desired) {
        if (desired <= _cap)
            return;

        usize cap = _cap * 2;
        if (cap < 16)
            cap = 16;
        if (cap < desired)
            cap = desired;
        if (cap > _limit)
            cap = _limit;

        Manual<T>* buf = new Manual<T>[cap];
        for (usize i = 0; i < _len; i++)
            buf[i].ctor(_buf[(_tail + i) % _cap].take());

        delete[] _buf;
        _buf = buf;
        _cap = cap;
        _tail = 0;
        _head = _len % cap;
    }

    void pushBack(T value) {
        if (_len == _limit) [[unlikely]]
            panic("push on full ring");

        if (_len == _cap) [[unlikely]]
            _ensure(_len + 1);

        _buf[_head].ctor(value);
        _head = (_head + 1) % _cap;
        _len++;
    }

    T popBack() {
        if (_len == 0) [[unlikely]]
            panic("pop on empty ring");

        _head = (_head + (_cap - 1)) % _cap;
        T value = _buf[_head].take();
        _len--;
        return value;
    }

    T popFront() {
        if (_len == 0) [[unlikely]]
            panic("dequeue on empty ring");

        T value = _buf[_tail].take();
        _tail = (_tail + 1) % _cap;
        _len--;
        return value;
    }

    void clear() {
        for (usize i = 0; i < _len; i++)
            _buf[(_tail + i) % _cap].dtor();

        _head = 0;
        _tail = 0;
        _len = 0;
    }

    T& peekFront(usize index) {
        if (index >= _len) [[unlikely]]
            panic("peek out of bounds");

        return _buf[(_tail + index) % _cap].unwrap();
    }

    T const& peekFront(usize index) const {
        if (index >= _len) [[unlikely]]
            panic("peek out of bounds");

        return _buf[(_tail + index) % _cap].unwrap();
    }

    T& peekBack(usize index) {
        if (index >= _len) [[unlikely]]
            panic("peek out of bounds");

        return _buf[(_head + _cap - 1 - index) % _cap].unwrap();
    }

    T const& peekBack(usize index) const {
        if (index >= _len) [[unlikely]]
            panic("peek out of bounds");

        return _buf[(_head + _cap - 1 - index) % _cap].unwrap();
    }

    void trunc(usize newLen) {
        if (newLen > _len) [[unlikely]]
            panic("trunc to longer length");

        for (usize i = newLen; i < _len; i++)
            _buf[(_tail + i) % _cap].dtor();

        _head = (_tail + newLen) % (_cap ? _cap : 1);
        _len = newLen;
    }

    usize head() const {
        return _head;
    }

    usize tail() const {
        return _tail;
    }

    usize len() const {
        return _len;
    }

    usize cap() const {
        return _cap;
    }

    usize limit() const {
        return _limit;
    }

    usize rem() const {
        return _limit - _len;
    }

    bool operator==(Sliceable<T> auto const& other) const {
        if (len() != other.len())
            return false;

        for (usize i = 0; i < len(); ++i)
            if (peekFront(i) != other[i])
                return false;

        return true;
    }
};

} // namespace Karm