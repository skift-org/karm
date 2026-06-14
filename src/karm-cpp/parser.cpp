export module Karm.Cpp:parser;

import Karm.Core;

namespace Karm::Cpp {

struct Expr {
};

// MARK: Statements ------------------------------------------------------------

// https://eel.is/c++draft/stmt
struct Stmt {
};

// https://eel.is/c++draft/stmt.label
struct LabelStmt : Stmt {
};

// https://eel.is/c++draft/stmt.expr
struct ExprStmt : Stmt {
};

// https://eel.is/c++draft/stmt.block
struct BlockStmt : Stmt {
};

// https://eel.is/c++draft/stmt.if
struct IfStmt : Stmt {
};

// https://eel.is/c++draft/stmt.switch
struct SwitchStmt : Stmt {
};

struct CaseStmt : Stmt {
};

// https://eel.is/c++draft/stmt.while
struct WhileStmt : Stmt {
};

// https://eel.is/c++draft/stmt.do
struct DoStmt : Stmt {
};

// https://eel.is/c++draft/stmt.for
struct ForStmt : Stmt {
};

// https://eel.is/c++draft/stmt.ranged
struct RangeForStmt : Stmt {
};

// https://eel.is/c++draft/stmt.expand
struct ExpandStmts : Stmt {
};

// https://eel.is/c++draft/stmt.break
struct BreakStmt : Stmt {
};

// https://eel.is/c++draft/stmt.cont
struct ContinueStmt : Stmt {
};

// https://eel.is/c++draft/stmt.cont
struct ReturnStmt : Stmt {
};

// https://eel.is/c++draft/stmt.return.coroutine
struct CoReturnStmt : Stmt {
};

// https://eel.is/c++draft/stmt.goto
struct GotoStmt : Stmt {
};

struct DeclStmt : Stmt {
};

// MARK: Declarations ----------------------------------------------------------

struct DeclAttr {
};

struct Decl {
};

} // namespace Karm::Cpp
