module;

#include <stdio.h>

export module Karm.Core:base.map;

import :base.cursor;
import :base.vec;
import :base.set;

namespace Karm {

export template <typename K, typename V>
struct Map {

    struct MapNode {
        K key;
        V value;

        u64 hash() const {
            return Karm::hash(key);
        }

        bool operator==(MapNode const& other) const {
            return hash() == other.hash();
        }
    };

    struct MapQueryNode {
        K key;

        u64 hash() const {
            return Karm::hash(key);
        }

        bool operator==(MapNode const& other) const {
            return hash() == other.hash();
        }
    };

    Set<MapNode> _set;

    Map() = default;

    Set<MapNode>::Slot* _lookup(MapQueryNode const& query) const {
        auto it = _set.lookup(query);
        if (not it)
            return nullptr;

        return it->state == Set<MapNode>::State::USED ? it : nullptr;
    }

    void put(K const& key, V value) {
        if (auto it = _lookup({key})) {
            printf("oshi \n");
            it->unwrap() = MapNode{key, std::move(value)};
        } else {
            _set.put(MapNode{key, std::move(value)});
        }
    }

    static Map fromList(std::initializer_list<Pair<K, V>>&& list) {
        Map map;
        for (auto const& [key, value] : list) {
            map.put(key, value);
        }
        return map;
    }

    bool has(K const& key) const {
        return _set.has(MapQueryNode{key});
    }

    V& get(K const& key) {
        MapQueryNode node{key};
        if (auto it = _lookup(node)) {
            return it->unwrap().value;
        } else {
            panic("key not found");
        }
    }

    V& getOrDefault(K const& key, V const& defaultValue = {}) {
        MapQueryNode node{key};
        if (auto it = _lookup(node)) {
            return it->unwrap().value;
        }
        _set.put({key, defaultValue});

        return _lookup(node)->unwrap().value;
    }

    MutCursor<V> access(K const& key) {
        if (auto ptrToSlot = _lookup(MapQueryNode{key}))
            return &ptrToSlot->unwrap().value;
        return nullptr;
    }

    Cursor<V> access(K const& key) const {
        if (auto ptrToSlot = _lookup(MapQueryNode{key}))
            return &ptrToSlot->unwrap().value;
        return nullptr;
    }

    // V take(K const& key) {
    //     for (usize i = 0; i < _els.len(); i++) {
    //         if (_els[i].v0 == key) {
    //             V value = std::move(_els[i].v1);
    //             _els.removeAt(i);
    //             return value;
    //         }
    //     }

    //     panic("key not found");
    // }

    Opt<V> tryGet(K const& key) const {
        if (auto it = _lookup(MapQueryNode{key})) {
            return it->unwrap().value;
        }

        return NONE;
    }

    bool del(K const& key) {
        return _set.del(MapQueryNode{key});
    }

    // bool removeAll(V const& value) {
    //     bool changed = false;

    //     for (usize i = 1; i < _els.len() + 1; i++) {
    //         if (_els[i - 1].v1 == value) {
    //             _els.removeAt(i - 1);
    //             changed = true;
    //             i--;
    //         }
    //     }

    //     return changed;
    // }

    // bool removeFirst(V const& value) {
    //     for (usize i = 1; i < _els.len() + 1; i++) {
    //         if (_els[i - 1].v1 == value) {
    //             _els.removeAt(i - 1);
    //             return true;
    //         }
    //     }

    //     return false;
    // }

    // auto iter() {
    //     return Iter{[&] mutable -> MapNode* {
    //         for (auto& node : _set.iter()) {
    //             return &node;
    //         }

    //         return nullptr;
    //     }};
    // }

    auto iter() const {
        return _set.iter();
    }

    // V at(usize index) const {
    //     return _els[index].v1;
    // }

    usize len() const {
        return _set.len();
    }

    void clear() {
        _set.clear();
    }

    bool operator==(Map const& other) const {
        if (len() != other.len())
            return false;

        for (auto const& [key, value] : iter()) {
            if (auto ov = other.tryGet(key)) {
                if (value != *ov)
                    return false;
            } else {
                return false;
            }
        }

        return true;
    }
};

} // namespace Karm
