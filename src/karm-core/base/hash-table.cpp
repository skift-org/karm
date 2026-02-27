export module Karm.Core:base.hashTable;

import :base.manual;
import :base.iter;
import :base.slice;
import :base.hash;

namespace Karm {

export template <typename T>
struct HashTable {
    enum State : u8 {
        FREE,
        USED,
        DEAD,
    };

    struct Slot {
        State state = State::FREE;
        Manual<T> _manual;

        T& unwrap() {
            if (state != State::USED)
                panic("slot is free");
            return _manual.unwrap();
        }

        T const& unwrap() const {
            if (state != State::USED)
                panic("slot is free");
            return _manual.unwrap();
        }

        T take() {
            if (state != State::USED)
                panic("slot is free");
            state = State::DEAD;
            return _manual.take();
        }
    };

    Slot* _slots = nullptr;
    usize _cap = 0;
    usize _len = 0;
    usize _dead = 0;

    HashTable() = default;

    HashTable(HashTable const& other) {
        _cap = other._cap;
        _slots = new Slot[_cap];
        _dead = other._dead;
        for (usize i = 0; i < _cap; i++) {
            if (other._slots[i].state == State::USED) {
                _slots[i]._manual.ctor(other._slots[i].unwrap());
                _slots[i].state = State::USED;
            } else {
                _slots[i].state = other._slots[i].state;
            }
        }
    }

    HashTable(HashTable&& other)
        : _slots(std::exchange(other._slots, nullptr)),
          _cap(std::exchange(other._cap, 0)) {
    }

    ~HashTable() {
        if (_slots) {
            clear();
            delete[] _slots;
            _slots = nullptr;
            _cap = 0;
        }
    }

    HashTable& operator=(HashTable const& other) {
        *this = HashTable(other);
        return *this;
    }

    HashTable& operator=(HashTable&& other) {
        std::swap(_slots, other._slots);
        std::swap(_cap, other._cap);
        std::swap(_len, other._len);
        std::swap(_dead, other._dead);
        return *this;
    }

    usize usage() const {
        if (not _cap)
            return 100;
        return ((_len + _dead) * 100) / _cap;
    }

    void rehash(usize desired) {
        if (desired <= _cap)
            return;

        if (not _slots) {
            _slots = new Slot[desired];
            for (usize i = 0; i < desired; i++)
                _slots[i].state = State::FREE;
            _cap = desired;
            return;
        }

        auto* oldSlots = std::exchange(_slots, new Slot[desired]);
        usize oldCap = std::exchange(_cap, desired);

        _len = 0;
        _dead = 0;

        for (usize i = 0; i < _cap; i++)
            _slots[i].state = State::FREE;

        for (usize i = 0; i < oldCap; i++) {
            auto& old = oldSlots[i];
            if (old.state != State::USED)
                continue;
            put(lookup(old.unwrap()), old.take());
        }

        delete[] oldSlots;
    }

    void ensure(usize len) {
        if (len) {
            // NOTE: 60% utilization is ideal for lookup speed and avoiding colision
            len = (len * 10) / 6;
            if (len > _cap)
                rehash(len);
        }
    }

    void ensureForInsert() {
        if (usage() >= 60)
            rehash(max(_cap * 2, 16uz));
    }

    template <typename Self, Meta::Equatable<T> U>
    auto lookup(this Self& self, U const& u) -> Meta::CopyConst<Self, Slot>* {
        if (not self._slots)
            return nullptr;

        usize start = hash(u) % self._cap;
        usize i = start;
        Meta::CopyConst<Self, Slot>* deadSlot = nullptr;
        while (self._slots[i].state != State::FREE) {
            auto& s = self._slots[i];

            if (s.state == State::USED and
                s.unwrap() == u)
                return &s;

            if (s.state == State::DEAD and not deadSlot)
                deadSlot = &s;

            i = (i + 1) % self._cap;
            if (i == start)
                return nullptr;
        }

        if (deadSlot)
            return deadSlot;

        return &self._slots[i];
    }

    auto iterUsed() const {
        auto slots = Slice(_slots, _cap);
        return Karm::iter(slots) |
               Where([](auto const& s) {
                   return s.state == USED;
               });
    }

    auto mutIterUsed() {
        auto slots = MutSlice(_slots, _cap);
        return Karm::mutIter(slots) |
               Where([](auto const& s) {
                   return s.state == USED;
               });
    }

    template <typename... Args>
    bool put(Slot* slot, Args&&... args) {
        if (slot->state == State::USED) {
            slot->unwrap() = T(std::forward<Args>(args)...);
            return false;
        }
        _len++;
        slot->_manual.ctor(std::forward<Args>(args)...);
        slot->state = State::USED;
        return true;
    }

    bool clear(Slot* slot) {
        if (slot->state != State::USED)
            return false;
        slot->state = State::DEAD;
        slot->_manual.dtor();
        _len--;
        _dead++;
        return true;
    }

    void clear() {
        for (usize i = 0; i < _cap; i++) {
            auto& slot = _slots[i];
            if (slot.state == State::USED)
                slot._manual.dtor();
            slot.state = State::FREE;
        }
        _len = 0;
        _dead = 0;
    }

    usize len() const {
        return _len;
    }
};

} // namespace Karm
