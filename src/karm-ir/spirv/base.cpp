export module Karm.Ir.Spirv:base;

import Karm.Core;

namespace Karm::Ir::Spirv {

export enum struct Op {
#define INSTRUCTION(name, opcode, _) name = opcode,
#include "defs/instructions.inc"

#undef INSTRUCTION
};

export Str toStr(Op op) {
#define INSTRUCTION(name, _, opname) \
    case Op::name:                   \
        return #opname;
    switch (op) {
#include "defs/instructions.inc"

    default:
        return "Unknown";
    }
}

} // namespace Karm::Ir::Spirv

export template <>
struct Karm::Io::Formatter<Karm::Ir::Spirv::Op> {
    Res<> format(TextWriter& w, Ir::Spirv::Op v) {
        return w.writeStr(toStr(v));
    }
};
