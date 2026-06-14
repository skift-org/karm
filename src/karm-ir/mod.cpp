export module Karm.Ir;

import Karm.Core;
import Karm.Diag;

namespace Karm::Ir {

export enum struct Primitive {
    VOID,

    U8,
    U16,
    U32,
    U64,

    I8,
    I16,
    I32,
    I64,

    F32,
    F64
};

export struct Vector {
    Primitive type;
    usize size;
};

using Type = Union<
    Primitive,
    Vector>;

// MARK: Operand ---------------------------------------------------------------

using Jump = Union<usize, Symbol>;

export struct Reg {
    u32 id;
};

export struct Imm {
    u64 value;
};

export struct Arg {
    u8 index;
};

export using Value = Union<Reg, Imm, Arg>;

// MARK: Instructions ----------------------------------------------------------

export struct Infix {
    enum struct _Op {
        ADD,
    };

    using enum _Op;
    _Op op;
    Reg dest;
    Type type;
    Value lhs;
    Value rhs;
};

export using Instruction = Union<Infix>;

// MARK: Terminator ------------------------------------------------------------

export struct Return {
    Value value;
};

export using Terminator = Union<Return>;

// MARK: Block -----------------------------------------------------------------

export struct Block {
    Vec<Instruction> instructions;
    Terminator terminator;
};

// MARK: Function --------------------------------------------------------------

export struct Function {
    Map<Jump, Block> blocks;
};

// MARK: Module ----------------------------------------------------------------

export struct Module {
    Map<Jump, Function> functions;
};

// MARK: Emit ------------------------------------------------------------------

export struct Chunk {
    Vec<u8> data;

    void emit(u8 b) {
        data.pushBack(b);
    }
};

void emit(Chunk& out, Instruction const& ir) {
    ir.visit([&](Infix const& i) {
        if (i.op == Infix::ADD) {
            if (i.lhs.is<Arg>()) {
                auto idx = i.lhs.unwrap<Arg>().index;
                if (idx == 0) {
                    // MOV RAX, RDI (Arg 0)
                    out.emit(0x48);
                    out.emit(0x89);
                    out.emit(0xF8);
                } else if (idx == 1) {
                    // MOV RAX, RSI (Arg 1)
                    out.emit(0x48);
                    out.emit(0x89);
                    out.emit(0xF0);
                }
            } else if (i.lhs.is<Imm>()) {
                // MOV RAX, Imm64
                out.emit(0x48);
                out.emit(0xB8);
                u64 v = i.lhs.unwrap<Imm>().value;
                for (int b = 0; b < 8; b++)
                    out.emit((v >> (b * 8)) & 0xFF);
            }

            if (i.rhs.is<Arg>()) {
                auto idx = i.rhs.unwrap<Arg>().index;
                if (idx == 1) {
                    out.emit(0x48);
                    out.emit(0x01);
                    out.emit(0xF0);
                } else if (idx == 0) {
                    out.emit(0x48);
                    out.emit(0x01);
                    out.emit(0xF8);
                }
            } else if (i.rhs.is<Imm>()) {
                out.emit(0x48);
                out.emit(0x05);
                u32 v = (u32)i.rhs.unwrap<Imm>().value;
                for (int b = 0; b < 4; b++)
                    out.emit((v >> (b * 8)) & 0xFF);
            }
        }
    });
}

void emit(Chunk& out, Terminator const& ir) {
    ir.visit([&](Return const&) {
        out.emit(0xC3);
    });
}

void emit(Chunk& out, Block const& ir) {
    for (auto& i : ir.instructions) {
        emit(out, i);
    }
    emit(out, ir.terminator);
}

void emit(Chunk& out, Function const& ir) {
    for (auto& [jump, block] : ir.blocks.iterItems()) {
        emit(out, block);
    }
}

export Chunk emit(Module const& ir) {
    Chunk out;
    for (auto& [jump, function] : ir.functions.iterItems()) {
        emit(out, function);
    }
    return out;
}

} // namespace Karm::Ir