export module Karm.Test:test;

import Karm.Core;

namespace Karm::Test {

export struct Test;
export struct Driver;

static Test* _first = nullptr;
static Test* _last = nullptr;

export struct Test : Meta::Pinned {
    enum struct Kind {
        SYNC,
        ASYNC,
    };

    using enum Kind;
    using Func = Res<> (*)(Driver&);
    using FuncAsync = Async::Task<> (*)(Driver&);

    Str _name;
    Kind _kind;

    union {
        Func _func;
        FuncAsync _funcAsync;
    };

    Loc _loc;
    Test* next = nullptr;

    static Test* first() {
        return _first;
    }

    static usize len() {
        usize n = 0;
        Test* test = _first;
        while (test) {
            n++;
            test = test->next;
        }
        return n;
    }

    Test(Str name, Func func, Loc loc = Loc::current())
        : _name(name), _kind(SYNC), _func(func), _loc(loc) {
        if (not _first) {
            _first = this;
            _last = this;
        } else {
            _last->next = this;
            _last = this;
        }
    }

    Test(Str name, FuncAsync func, Loc loc = Loc::current())
        : _name(name), _kind(ASYNC), _funcAsync(func), _loc(loc) {
        if (not _first) {
            _first = this;
            _last = this;
        } else {
            _last->next = this;
            _last = this;
        }
    }

    Async::Task<> runAsync(Driver& driver) {
        if (_kind == ASYNC) {
            co_return co_await _funcAsync(driver);
        } else {
            co_return _func(driver);
        }
    }
};

} // namespace Karm::Test
