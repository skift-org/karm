module;

#include <karm/macros>

export module Karm.Core:base.set;

import :meta.cvrp;
import :base.clamp;
import :base.hashTable;
import :base.iter;

namespace Karm {

export template <typename T>
struct Set {
    using Items = HashTable<T>;
    Items _items;

    Set(usize cap = 0) {
        _items.ensure(cap);
    };

    Set(std::initializer_list<T> items) {
        _items.ensure(items.size());
        for (auto& i : items)
            add(i);
    }

    void ensure(usize len) {
        _items.ensure(len);
    }

    bool contains(Meta::Equatable<T> auto const& key) const {
        if (auto it = _items.lookup(key);
            it and it->state == Items::USED)
            return true;
        return false;
    }

    void add(Meta::Convertible<T> auto&& value = T{}) {
        auto* slot = _items.lookup(value);
        if (slot and slot->state == Items::USED) {
            return;
        }
        if (not slot) {
            _items.ensureForInsert();
            slot = _items.lookup(value);
        }
        _items.put(slot, std::forward<decltype(value)>(value));
    }

    void addFrom(Iterable auto const& from) {
        for (auto v : from.iter())
            add(v);
    }

    bool remove(Meta::Equatable<T> auto const& key) {
        if (auto slot = _items.lookup(key);
            slot and slot->state == Items::USED) {
            _items.clear(slot);
            return true;
        }
        return false;
    }

    void clear() {
        _items.clear();
    }

    [[nodiscard]] Opt<T const&> lookup(Meta::Equatable<T> auto const& key) const lifetimebound {
        if (auto it = _items.lookup(key); it and it->state == Items::USED)
            return it->unwrap();
        return NONE;
    }

    [[nodiscard]] T const& lookupOrAdd(Meta::Equatable<T> auto const& key, Meta::Callable<> auto&& build) lifetimebound {
        auto* slot = _items.lookup(key);
        if (slot and slot->state == Items::USED) {
            return slot->unwrap();
        }

        if (not slot) {
            _items.ensureForInsert();
            slot = _items.lookup(key);
        }

        _items.put(slot, build());
        return slot->unwrap();
    }

    [[nodiscard]] auto iter() const lifetimebound {
        return _items.iterUsed() |
               Select([](auto const& s) -> auto const& {
                   return s.unwrap();
               });
    }

    [[nodiscard]] usize len() const {
        return _items.len();
    }

    void hash(Meta::Derive<Hasher> auto& h) const {
        u64 sum = 0;
        for (auto const& v : iter()) {
            // NOTE: This will use the default hasher instead of the one given to us
            //       because we cannot easily instantiate a hasher of the same type.
            sum += Karm::hash(v);
        }
        Karm::hash(h, sum);
    }

    Set operator-(Set const& other) const {
        Set res;
        for (auto& v : iter())
            if (not other.contains(v))
                res.add(v);
        return res;
    }

    Set operator|(Set const& other) const {
        Set res;
        res.ensure(max(len(), other.len()));
        res.addFrom(*this);
        res.addFrom(other);
        return res;
    }

    Set operator&(Set const& other) const {
        Set res;
        if (len() <= other.len()) {
            for (auto& v : iter())
                if (other.contains(v))
                    res.add(v);
        } else {
            for (auto& v : other.iter())
                if (contains(v))
                    res.add(v);
        }
        return res;
    }

    Set operator^(Set const& other) const {
        Set res;
        for (auto& v : iter())
            if (not other.contains(v))
                res.add(v);
        for (auto& v : other.iter())
            if (not contains(v))
                res.add(v);
        return res;
    }

    bool operator==(Set const& other) const {
        if (len() != other.len())
            return false;
        for (auto& v : iter()) {
            if (not other.contains(v))
                return false;
        }
        return true;
    }

    operator bool() const {
        return len();
    }
};

} // namespace Karm
