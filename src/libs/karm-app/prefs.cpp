module;

#include <karm-async/task.h>
#include <karm-json/values.h>

export module Karm.App:prefs;

import :_embed;

namespace Karm::App {

export struct Prefs {
    virtual ~Prefs() = default;

    virtual Async::Task<Json::Value> loadAsync(String key, Json::Value defaultValue = NONE) = 0;

    virtual Async::Task<> saveAsync(String key, Json::Value value) = 0;
};

export struct MockPrefs : Prefs {
    Json::Object _store;

    Async::Task<Json::Value> loadAsync(String key, Json::Value defaultValue = NONE) override {
        auto item = _store.access(key);
        if (item)
            co_return *item;
        co_return defaultValue;
    }

    Async::Task<> saveAsync(String key, Json::Value value) override {
        _store.put(key, value);
        co_return Ok();
    }
};

export Prefs& globalPrefs() {
    return _Embed::globalPrefs();
}

} // namespace Karm::App
