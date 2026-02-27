module;

#include <karm/macros>

export module Karm.Core:base.map;

import :base.cursor;
import :base.vec;
import :base.tuple;
import :base.hashTable;

namespace Karm {

export template <typename K, typename V>
struct KvPair {
    K key;
    V value;

    u64 hash() const {
        return Karm::hash(key);
    }

    bool operator==(Meta::Equatable<Meta::RemoveConstVolatileRef<K>> auto const& other) const {
        return key == other;
    }
};

export template <typename K, typename V>
struct Map {
    using Item = KvPair<K, V>;
    using Items = HashTable<Item>;

    Items _items;

    Map(usize cap = 0) {
        _items.ensure(cap);
    };

    Map(std::initializer_list<Item> items) {
        _items.ensure(items.size());
        for (auto& i : items)
            put(i.key, i.value);
    }

    void ensure(usize len) {
        _items.ensure(len);
    }

    [[nodiscard]] bool contains(Meta::Equatable<K> auto const& key) const {
        if (auto it = _items.lookup(key); it and it->state == Items::USED)
            return true;
        return false;
    }

    void put(Meta::Equatable<K> auto const& key, Meta::Convertible<V> auto&& value) {
        _items.ensureForInsert();
        auto* slot = _items.lookup(key);
        _items.put(slot, key, std::forward<decltype(value)>(value));
    }

    Opt<V> remove(Meta::Equatable<K> auto const& key) {
        if (auto* slot = _items.lookup(key); slot and slot->state == Items::USED) {
            V res = std::move(slot->unwrap().value);
            _items.clear(slot);
            return res;
        }
        return NONE;
    }

    bool removeValue(Meta::Equatable<V> auto const& v) {
        bool res = false;
        for (auto& slot : _items.mutIterUsed()) {
            if (slot.unwrap().value == v) {
                _items.clear(&slot);
                res = true;
            }
        }
        return res;
    }

    void clear() {
        _items.clear();
    }

    [[nodiscard]] Opt<V const&> lookup(Meta::Equatable<K> auto const& key) const lifetimebound {
        if (auto* slot = _items.lookup(key);
            slot and slot->state == Items::USED)
            return slot->unwrap().value;
        return NONE;
    }

    [[nodiscard]] V& lookupOrPut(Meta::Equatable<K> auto const& key, Meta::Callable<> auto&& build) lifetimebound {
        auto* slot = _items.lookup(key);
        if (slot and slot->state == Items::USED) {
            return slot->unwrap().value;
        }

        if (not slot) {
            _items.ensureForInsert();
            slot = _items.lookup(key);
        }

        _items.put(slot, key, build());
        return slot->unwrap().value;
    }

    [[nodiscard]] V& lookupOrPutDefault(Meta::Equatable<K> auto const& key, V const& defaultValue = {}) lifetimebound {
        return lookupOrPut(key, [&]() {
            return defaultValue;
        });
    }

    [[nodiscard]] V& lookupOrPutDefault(Meta::Equatable<K> auto const& key, Meta::Convertible<V> auto&& defaultValue = V{}) lifetimebound {
        auto* slot = _items.lookup(key);
        if (slot and slot->state == Items::USED) {
            return slot->unwrap().value;
        }

        if (not slot) {
            _items.ensureForInsert();
            slot = _items.lookup(key);
        }

        _items.put(slot, key, std::forward<decltype(defaultValue)>(defaultValue));
        return slot->unwrap().value;
    }

    [[nodiscard]] auto iter() const lifetimebound {
        return _items.iterUsed() |
               Select([](auto const& s) -> auto const& {
                   return s.unwrap().key;
               });
    }

    [[nodiscard]] auto mutIterValue() lifetimebound {
        return _items.mutIterUsed() |
               Select([](auto& s) -> auto& {
                   return s.unwrap().value;
               });
    }

    [[nodiscard]] auto iterValue() const lifetimebound {
        return _items.iterUsed() |
               Select([](auto const& s) -> auto const& {
                   return s.unwrap().value;
               });
    }

    [[nodiscard]] auto iterItems() const lifetimebound {
        return _items.iterUsed() |
               Select([](auto const& s) -> auto const& {
                   return s.unwrap();
               });
    }

    [[nodiscard]] usize len() const {
        return _items.len();
    }

    [[nodiscard]] u64 hash() const {
        u64 res = 0;
        for (auto const& [k, v] : iterItems())
            res += Karm::hash(k) + Karm::hash(v);
        return res;
    }

    bool operator==(Map const& other) const {
        if (len() != other.len())
            return false;

        for (auto const& [k, v] : iterItems()) {
            auto it = other.lookup(k);
            if (not it or it.unwrap() != v) {
                return false;
            }
        }
        return true;
    }

    operator bool() const {
        return _items.len() != 0;
    }
};

} // namespace Karm
