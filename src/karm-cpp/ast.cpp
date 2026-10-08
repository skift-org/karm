export module Karm.Cpp:ast;

import Karm.Core;
import Karm.Gc;

namespace Karm::Cpp {

// MARK: Expr ------------------------------------------------------------------

export struct Expr {};

export struct ValueExpr {};

export struct IdentExpr {};

export struct PrefixExpr {};

export struct PostfixExpr {};

export struct InfixExpr {};

export struct CallExpr {};

export struct CastExpr {};

export struct TernaryExpr {};

// MARK: Stmt ------------------------------------------------------------------

// https://eel.is/c++draft/stmt
export struct Stmt {};

// https://eel.is/c++draft/stmt.label
export struct LabelStmt : Stmt {};

// https://eel.is/c++draft/stmt.expr
export struct ExprStmt : Stmt {};

// https://eel.is/c++draft/stmt.block
export struct BlockStmt : Stmt {};

// https://eel.is/c++draft/stmt.if
export struct IfStmt : Stmt {};

// https://eel.is/c++draft/stmt.switch
export struct SwitchStmt : Stmt {};

export struct CaseStmt : Stmt {};

// https://eel.is/c++draft/stmt.while
export struct WhileStmt : Stmt {};

// https://eel.is/c++draft/stmt.do
export struct DoStmt : Stmt {};

// https://eel.is/c++draft/stmt.for
export struct ForStmt : Stmt {};

// https://eel.is/c++draft/stmt.ranged
export struct RangeForStmt : Stmt {};

// https://eel.is/c++draft/stmt.expand
export struct ExpandStmts : Stmt {};

// https://eel.is/c++draft/stmt.break
export struct BreakStmt : Stmt {};

// https://eel.is/c++draft/stmt.cont
export struct ContinueStmt : Stmt {};

// https://eel.is/c++draft/stmt.return
export struct ReturnStmt : Stmt {};

// https://eel.is/c++draft/stmt.return.coroutine
export struct CoReturnStmt : Stmt {};

// https://eel.is/c++draft/stmt.goto
export struct GotoStmt : Stmt {};

export struct DeclStmt : Stmt {};

// MARK: Type ------------------------------------------------------------------

export struct Type : Base {
    enum struct Attr {
        CONST,
        MUTABLE,
    };
};

// https://eel.is/c++draft/dcl#type.simple
export struct SimpleType : Type {
    enum struct Simple {
        VOID,
        BOOL,

        U8,
        U16,
        U32,
        U64,

        I8,
        I16,
        I32,
        I64,

        USIZE,
        ISIZE,
    };
};

export struct PtrType : Type {
    Gc::Ref<Type> type;
};

export struct RefType : Type {
    Gc::Ref<Type> type;
};

export struct ArrayType : Type {
    Gc::Ref<Type> type;
    usize size;
};

export struct VecType : Type {
    SimpleType::Simple type;
    usize width;
};

// includes class, struct, union
export struct StructType : Type {
    struct Member {};

    Vec<Member> members;
};

export struct EnumType : Type {
    struct Member {};

    Vec<Member> members;
};

export struct FuncType : Type {
    struct Argument {};

    Gc::Ref<Type> ret;

    Vec<Argument> arguments;
};

// MARK: Decl ------------------------------------------------------------------

// https://eel.is/c++draft/dcl#decl
export struct Decl : Base {
    enum struct Attr {
        AUTO,
        STATIC,
        REGISTER,
        INLINE,
        EXTERN,
        THREAD,
        NO_RETURN,
    };

    Flags<Attr> attr;
    String name;
};

export struct TypeDecl : Decl {
    Gc::Ref<Type> type;
};

export struct VarDecl : Decl {
    Gc::Ref<Type> type;
};

export struct FuncDecl : Decl {
    Gc::Ref<FuncType> type;
};

} // namespace Karm::Cpp
