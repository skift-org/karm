export module Karm.Dl:stack;

import Karm.Core;

namespace Karm::Dl {

export struct Stack {
    MutBytes _buf;
    usize _bsp;
    usize _sp = _bsp;

    Stack(MutBytes buf, usize bsp)
        : _buf(buf), _bsp(bsp) {}

    urange vrange() {
        return urange::fromStartEnd(_bsp - _buf.len(), _bsp);
    }

    MutBytes alloc(usize size, usize align) {
        _sp = alignDown(_sp - size, align);
        auto base = _sp - vrange().start;
        return mutSub(_buf, base, base + size);
    }

    usize push(Bytes v, usize align = 1) {
        copy(v, alloc(v.len(), align));
        return _sp;
    }

    usize push(Str v) {
        Array zero = {'\0'};
        push(bytes(zero));
        return push(bytes(v));
    }

    usize push(usize v) {
        Array data = {v};
        return push(bytes(data), alignof(usize));
    }

    usize push(Slice<usize> v) {
        return push(bytes(v), alignof(usize));
    }
};

} // namespace Karm::Dl