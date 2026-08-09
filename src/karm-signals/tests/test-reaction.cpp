import Karm.Core;
import Karm.Signals;

#include <karm/test>

namespace Karm::Signals::Tests {

struct Counter {
    struct Cell : Node {
        Signal<int> _n;
        usize _runs = 0;
        int _last = 0;

        Cell(Signal<int> n) : _n(n) {}

        void recompute() override {
            observe([&] {
                _runs++;
                _last = _n.value();
            });
        }

        bool reaction() override {
            return true;
        }
    };

    mutable Rc<Cell> _state;

    Counter(Signal<int> n) : _state(makeRc<Cell>(n)) {
        _state->update();
    }

    usize runs() const {
        return _state->_runs;
    }

    int last() const {
        return _state->_last;
    }

    void stop() const {
        _state->stop();
    }
};

struct Clamp {
    struct Cell : Node {
        Signal<int> _in;
        int _lo;
        int _hi;
        int _out = 0;

        Cell(Signal<int> in, int lo, int hi) : _in(in), _lo(lo), _hi(hi) {}

        void recompute() override {
            int next = observe([&] {
                return _in.value();
            });

            next = next < _lo ? _lo : (next > _hi ? _hi : next);

            if (next != _out) {
                _out = next;
                notify();
            }
        }

        bool reaction() override {
            return true;
        }
    };

    mutable Rc<Cell> _state;

    Clamp(Signal<int> in, int lo, int hi) : _state(makeRc<Cell>(in, lo, hi)) {
        _state->update();
    }

    int get() const {
        trackDependency(_state);
        return _state->_out;
    }
};

test$("reaction-runs-on-adoption") {
    Signal n{1};
    Counter c{n};

    expectEq$(c.runs(), 1uz);
    expectEq$(c.last(), 1);

    return Ok();
}

test$("reaction-tracks-what-it-reads") {
    Signal n{1};
    Signal other{1};
    Counter c{n};

    n.update(2);
    expectEq$(c.runs(), 2uz);
    expectEq$(c.last(), 2);

    n.update(2);
    expectEq$(c.runs(), 2uz);

    other.update(9);
    expectEq$(c.runs(), 2uz);

    return Ok();
}

test$("reaction-stops") {
    Signal n{1};
    Counter c{n};

    c.stop();
    n.update(2);

    expectEq$(c.runs(), 1uz);
    expectEq$(c.last(), 1);

    return Ok();
}

test$("reaction-can-be-observed-by-a-computed") {
    Signal n{1};
    Clamp c{n, 0, 10};
    Vec<int> seen;

    Computed doubled{[&] {
        return c.get() * 2;
    }};

    Effect watch = [&] {
        seen.pushBack(doubled.value());
    };

    n.update(5);
    n.update(50);
    n.update(99);
    n.update(-4);

    expectEq$(seen.len(), 4uz);
    expectEq$(seen[0], 2);
    expectEq$(seen[1], 10);
    expectEq$(seen[2], 20);
    expectEq$(seen[3], 0);

    return Ok();
}

test$("reaction-unlinks-itself-when-destroyed") {
    Signal n{1};
    usize live = 0;

    Effect keep = [&] {
        live++;
        (void)n.value();
    };

    {
        Counter tmp{n};
        n.update(2);
        expectEq$(tmp.runs(), 2uz);
    }

    n.update(3);

    expectEq$(live, 3uz);

    return Ok();
}

} // namespace Karm::Signals::Tests
