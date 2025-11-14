module;

#include <karm-core/macros.h>

export module Karm.Template;

import Karm.Core;

namespace Karm::Template {

struct Expr {
    virtual ~Expr() = default;

    virtual Res<Serde::Value> eval(Serde::Object const& self) = 0;
};

struct SelfExpr : Expr {
    Res<Serde::Value> eval(Serde::Object const& self) override {
        return Ok(self);
    }
};

struct PrefixExpr : Expr {
    enum struct Op {
        NOT,
    };

    using enum Op;

    Op _op;
    Rc<Expr> _expr;

    PrefixExpr(Op op, Rc<Expr> expr)
        : _op(op), _expr(expr) {}

    Res<Serde::Value> eval(Serde::Object const& self) override {
        auto val = try$(_expr->eval(self));

        if (_op == NOT)
            return Ok(not val.asBool());

        unreachable();
    }
};

struct InfixExpr : Expr {
    enum struct Op {
        AND,
        OR
    };

    using enum Op;

    Op _op;
    Rc<Expr> _lhs;
    Rc<Expr> _rhs;

    InfixExpr(Op op, Rc<Expr> lhs, Rc<Expr> rhs)
        : _op(op), _lhs(lhs), _rhs(rhs) {}

    Res<Serde::Value> eval(Serde::Object const& self) override {
        auto lhs = try$(_lhs->eval(self));
        auto rhs = try$(_rhs->eval(self));

        if (_op == AND)
            return Ok(lhs.asBool() and rhs.asBool());
        else if (_op == OR)
            return Ok(lhs.asBool() or rhs.asBool());

        unreachable();
    }
};

struct ValueExpr : Expr {
    Serde::Value _value;

    ValueExpr(Serde::Value value) : _value(value) {}

    Res<Serde::Value> eval(Serde::Object const&) override {
        return Ok(_value);
    }
};

struct DotExpr : Expr {
    Rc<Expr> _lhs;
    String _field;

    DotExpr(Rc<Expr> lhs, Str field)
        : _lhs(lhs),
          _field(field) {}

    Res<Serde::Value> eval(Serde::Object const& self) override {
        auto val = try$(_lhs->eval(self));

        if (not val.isObject())
            return Error::invalidInput("cannot access field on non-object");

        if (not val.has(_field))
            return Error::invalidInput("object don't have field");

        return Ok(val.get(_field));
    }
};

struct Node {
    virtual ~Node() = default;

    virtual Res<> eval(Serde::Object const& self, Io::TextWriter& w) = 0;
};

using Body = Vec<Rc<Node>>;

struct TextNode : Node {
    String text;

    explicit TextNode(Str text)
        : text(text) {}

    Res<> eval(Serde::Object const&, Io::TextWriter& w) override {
        return w.writeStr(text.str());
    }
};

struct PrintNode : Node {
    Rc<Expr> expr;

    explicit PrintNode(Rc<Expr> expr)
        : expr(expr) {}

    Res<> eval(Serde::Object const& self, Io::TextWriter& w) override {
        auto res = try$(expr->eval(self)).asStr();
        return w.writeStr(res.str());
    }
};

struct IfNode : Node {
    struct Branch {
        Rc<Expr> expr;
        Body body;
    };

    Vec<Branch> branches;
    Body elseBody;

    IfNode(Vec<Branch> branches, Body elseBody)
        : branches(std::move(branches)),
          elseBody(std::move(elseBody)) {}

    Res<> eval(Serde::Object const& self, Io::TextWriter& w) override {
        for (auto& b : branches) {
            auto res = try$(b.expr->eval(self));
            if (res.asBool()) {
                for (auto& n : b.body)
                    try$(n->eval(self, w));
                return Ok();
            }
        }

        for (auto& n : elseBody)
            try$(n->eval(self, w));

        return Ok();
    }
};

struct ForNode : Node {
    String var;
    Rc<Expr> rangeExpr;
    Body loopBody;

    ForNode(String var, Rc<Expr> rangeExpr, Body loopBody)
        : var(var),
          rangeExpr(rangeExpr),
          loopBody(loopBody) {}

    Res<> eval(Serde::Object const& self, Io::TextWriter& w) override {
        auto range = try$(rangeExpr->eval(self));
        if (not range.isArray())
            return Error::invalidInput("expected array");

        for (auto& item : range.asArray()) {
            Serde::Object scope = self;
            scope.put(var, item);
            for (auto& n : loopBody) {
                try$(n->eval(scope, w));
            }
        }

        return Ok();
    }
};

// MARK: Parser ----------------------------------------------------------------

static Res<Body> _parseBody(Io::SScan& s);

static Str _skipKeywordStart(Io::SScan& s) {
    if (not s.skip("{{"))
        return "";
    s.eat(Re::blank());
    auto keyword = s.token(Re::oneOrMore(Re::alpha()));
    s.skip(Re::blank());
    return keyword;
}

static Str _skipKeywordEnd(Io::SScan& s) {
    s.begin();
    while (not s.ended() and not s.ahead("}}"))
        s.next();
    auto expr = s.end();
    s.skip("}}");
    return expr;
}

static bool _skipKeyword(Io::SScan& s, Str keyword) {
    auto rollback = s.rollbackPoint();
    if (_skipKeywordStart(s) == keyword) {
        auto rest = _skipKeywordEnd(s);
        if (rest.len() != 0) {
            // Not a plain {{keyword}}, e.g. {{else if ...}}
            return false;
        }
        rollback.disarm();
        return true;
    }
    return false;
}

static Str _peekKeyword(Io::SScan s) {
    return _skipKeywordStart(s);
}

static bool _peekElseIf(Io::SScan s) {
    if (not s.skip("{{"))
        return false;

    s.eat(Re::blank());
    if (not s.skip("else"))
        return false;

    s.eat(Re::blank());
    if (not s.skip("if"))
        return false;

    return true;
}

static Opt<Rc<Node>> _parseTextNode(Io::SScan& s) {
    s.begin();
    while (not s.ended() and not s.ahead("{{"))
        s.next();
    auto text = s.end();
    if (not text)
        return NONE;
    return makeRc<TextNode>(text);
}

static Res<Str> _parseIdent(Io::SScan& s) {
    auto ident = s.token((Re::alpha() | '_'_re) & Re::zeroOrMore(Re::alnum() | '_'_re));
    if (not ident)
        return Error::invalidInput("expected ident");
    return Ok(ident);
}

static Res<Rc<Expr>> _parseExprInfix(Io::SScan& s);

static Res<Rc<Expr>> _parseExprPrefix(Io::SScan& s) {
    if (s.ahead("."))
        return Ok(makeRc<SelfExpr>());
    else if (s.skip("not")) {
        auto rhs = try$(_parseExprInfix(s));
        return Ok(makeRc<PrefixExpr>(PrefixExpr::NOT, rhs));
    } else if (s.skip("true"))
        return Ok(makeRc<ValueExpr>(true));
    else if (s.skip("false"))
        return Ok(makeRc<ValueExpr>(false));
    else if (s.skip("none"))
        return Ok(makeRc<ValueExpr>(NONE));
    else
        return Error::invalidInput("expected expression");
}

static Res<Rc<Expr>> _parseExprInfix(Io::SScan& s) {
    s.eat(Re::blank());
    Rc<Expr> lhs = try$(_parseExprPrefix(s));
    while (not s.ended()) {
        s.eat(Re::blank());
        if (s.skip(".")) {
            auto ident = try$(_parseIdent(s));
            lhs = makeRc<DotExpr>(lhs, ident);
        } else if (s.skip("and")) {
            auto rhs = try$(_parseExprInfix(s));
            return Ok(makeRc<InfixExpr>(InfixExpr::AND, lhs, rhs));
        } else if (s.skip("or")) {
            auto rhs = try$(_parseExprInfix(s));
            return Ok(makeRc<InfixExpr>(InfixExpr::OR, lhs, rhs));
        } else {
            break;
        }
    }
    return Ok(lhs);
}

static Res<Rc<Expr>> _parseExpr(Io::SScan& s) {
    return _parseExprInfix(s);
}

static Res<Rc<Node>> _parseIfNode(Io::SScan& s) {
    // Parse: {{if <expr>}}
    _skipKeywordStart(s);
    auto firstExpr = try$(_parseExpr(s));
    _skipKeywordEnd(s);

    Vec<IfNode::Branch> branches;
    Body elseBody;

    IfNode::Branch firstBranch{
        .expr = firstExpr,
        .body = try$(_parseBody(s)),
    };
    branches.pushBack(std::move(firstBranch));

    while (true) {
        if (_peekElseIf(s)) {
            // Parse: {{else if <expr>}}
            s.skip("{{");
            s.eat(Re::blank());
            if (not s.skip("else"))
                return Error::invalidInput("expected else");
            s.eat(Re::blank());
            if (not s.skip("if"))
                return Error::invalidInput("expected if");
            s.eat(Re::blank());

            auto expr = try$(_parseExpr(s));
            s.eat(Re::blank());
            s.skip("}}");

            IfNode::Branch branch{
                .expr = expr,
                .body = try$(_parseBody(s)),
            };
            branches.pushBack(std::move(branch));
            continue;
        }

        if (_skipKeyword(s, "else")) {
            elseBody = try$(_parseBody(s));
            break;
        }

        if (_skipKeyword(s, "end")) {
            break;
        }

        return Error::invalidInput("expected {{else if}}, {{else}}, or {{end}}");
    }

    return Ok(makeRc<IfNode>(std::move(branches), std::move(elseBody)));
}

static Res<Rc<Node>> _parseForNode(Io::SScan& s) {
    _skipKeywordStart(s);

    auto var = try$(_parseIdent(s));
    if (not s.skip(Re::zeroOrMore(Re::blank()) & "in"_re & Re::zeroOrMore(Re::blank())))
        return Error::invalidInput("expected in keyword");

    auto expr = try$(_parseExpr(s));
    _skipKeywordEnd(s);

    auto loopBody = try$(_parseBody(s));

    if (not _skipKeyword(s, "end"))
        return Error::invalidInput("expected {{end}}");

    return Ok(makeRc<ForNode>(var, expr, loopBody));
}

static Res<Rc<Node>> _parsePrintNode(Io::SScan& s) {
    s.skip("{{");
    s.eat(Re::blank());
    auto expr = try$(_parseExpr(s));
    s.eat(Re::blank());
    s.skip("}}");
    return Ok(makeRc<PrintNode>(expr));
}

static Res<Body> _parseBody(Io::SScan& s) {
    Body body;
    while (true) {
        if (auto text = _parseTextNode(s))
            body.pushBack(text.take());

        if (s.ended())
            break;

        auto kw = _peekKeyword(s);
        if (kw == "if") {
            body.pushBack(try$(_parseIfNode(s)));
        } else if (kw == "else") {
            break;
        } else if (kw == "for") {
            body.pushBack(try$(_parseForNode(s)));
        } else if (kw == "end") {
            break;
        } else {
            body.pushBack(try$(_parsePrintNode(s)));
        }
    }
    return Ok(std::move(body));
}

// MARK: Template --------------------------------------------------------------

export struct Document {
    Body _body;

    Document(Body body)
        : _body(std::move(body)) {}

    static Res<Document> parse(Io::SScan& s) {
        return Ok(try$(_parseBody(s)));
    }

    static Res<Document> parse(Str s) {
        Io::SScan scan{s};
        return parse(scan);
    }

    Res<> eval(Serde::Object const& self, Io::TextWriter& w) {
        for (auto& n : _body)
            try$(n->eval(self, w));
        return Ok();
    }

    Res<String> eval(Serde::Object const& self) {
        Io::StringWriter sw;
        try$(eval(self, sw));
        return Ok(sw.take());
    }

    static Res<String> eval(Str tmpl, Serde::Object const& self) {
        auto t = try$(parse(tmpl));
        return t.eval(self);
    }
};

} // namespace Karm::Template
