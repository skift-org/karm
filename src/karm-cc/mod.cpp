export module Karm.Cc;

import Karm.Core;
import Karm.Gc;

namespace Karm::Cc {

export struct Base {
    virtual ~Base() = default;

    virtual bool is(Meta::Id id) const {
        return id == Meta::idOf<Base>();
    }

    template <Meta::Derive<Base> T>
    Opt<T&> as() {
        if (is(Meta::idOf<T>()))
            return Some(static_cast<T&>(*this));
        return NONE;
    }

    template <Meta::Derive<Base> T>
    Opt<T const&> as() const {
        if (is(Meta::idOf<T>()))
            return Some(static_cast<T const&>(*this));
        return NONE;
    }
};

// MARK: Expr ------------------------------------------------------------------

export struct Expr {};

export struct ValueExpr {};

export struct IdentExpr {};

export struct PrefixExpr {};

export struct PosfixExpr {};

export struct InfixExpr {};

export struct CallExpr {};

export struct CastExpr {};

export struct TernaryExpr {};

// MARK: Stmt ------------------------------------------------------------------

export struct Stmt {};

export struct DeclStmt : Stmt {};

export struct ExprStmt : Stmt {};

export struct BlockStmt : Stmt {};

export struct IfStmt : Stmt {};

export struct ForStmt : Stmt {};

export struct WhileStmt : Stmt {};

export struct DoStmt : Stmt {};

export struct SwitchStmt : Stmt {};

export struct ReturnStmt : Stmt {};

export struct CaseStmt : Stmt {
};

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

    Vec<Argument> argments;
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

} // namespace Karm::Cc