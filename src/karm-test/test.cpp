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
    using FuncAsync = Async::Task<> (*)(Driver&, Async::CancellationToken);

    Str name;
    Kind _kind;

    union {
        Func _func;
        FuncAsync _funcAsync;
    };

    SourceLocation sourceLocation;
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

    Test(Str name, Func func, SourceLocation loc = SourceLocation::current())
        : name(name), _kind(SYNC), _func(func), sourceLocation(loc) {
        if (not _first) {
            _first = this;
            _last = this;
        } else {
            _last->next = this;
            _last = this;
        }
    }

    Test(Str name, FuncAsync func, SourceLocation loc = SourceLocation::current())
        : name(name), _kind(ASYNC), _funcAsync(func), sourceLocation(loc) {
        if (not _first) {
            _first = this;
            _last = this;
        } else {
            _last->next = this;
            _last = this;
        }
    }

    Async::Task<> runAsync(Driver& driver, Async::CancellationToken ct) {
        if (_kind == ASYNC) {
            co_return co_await _funcAsync(driver, ct);
        } else {
            co_return _func(driver);
        }
    }
};

} // namespace Karm::Test
