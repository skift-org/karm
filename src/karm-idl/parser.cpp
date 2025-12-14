module;

#include <karm-core/macros.h>

export module Karm.Idl:parser;

import :ast;

namespace Karm::Idl {

static void _eatWhitespace(Io::SScan& s) {
    s.eat(Re::space());
}

// identifier ::= (alpha | "_") (alnum | "_")*
static Res<Identifier> _parseIdentifier(Io::SScan& s) {
    auto iden = s.token((Re::alpha() | '_'_re) & Re::oneOrMore(Re::alnum() | '_'_re));
    if (not iden)
        return Error::invalidData("expected identifier");
    return Ok(Symbol::from(iden));
}

// compound-identifier ::= identifier ("." identifier)*
static Res<CompoundIdentifier> _parseCompoundIdentifier(Io::SScan& s) {
    CompoundIdentifier res;
    do {
        res.pushBack(try$(_parseIdentifier(s)));
    } while (not s.ended() and s.skip("."));

    if (not res.len())
        return Error::invalidData("expected identifier");

    return Ok(std::move(res));
}

// type ::= compound-identifier ("<" type ("," type)* ">")?
static Res<Type> _parseType(Io::SScan& s) {
    auto identifier = try$(_parseCompoundIdentifier(s));
    _eatWhitespace(s);

    Vec<Type> parameters;
    if (s.skip("<")) {
        _eatWhitespace(s);

        do {
            parameters.pushBack(try$(_parseType(s)));
            _eatWhitespace(s);
        } while (not s.ended() and s.skip(","));
        if (not s.skip(">"))
            return Error::invalidData("expected closing '>'");
    }
    return Ok<Type>(std::move(identifier), std::move(parameters));
}

// parameter ::= identifier ":" type
static Res<Parameter> _parseParameter(Io::SScan& s) {
    auto name = try$(_parseIdentifier(s));
    _eatWhitespace(s);

    if (not s.skip(":"))
        return Error::invalidData("expected ':'");
    _eatWhitespace(s);

    auto type = try$(_parseType(s));
    return Ok<Parameter>(name, type);
}

// parameters ::= type | "(" parameter ("," parameter)* ")"
static Res<Union<None, Type, Parameters>> _parseParameters(Io::SScan& s) {
    if (not s.skip("("))
        return Ok(try$(_parseType(s)));
    _eatWhitespace(s);

    Parameters parameters;

    do {
        parameters.pushBack(try$(_parseParameter(s)));
        _eatWhitespace(s);
    } while (not s.ended() and s.skip(","));

    if (not s.skip(")"))
        return Error::invalidData("expected closing ')'");
    return Ok(std::move(parameters));
}

// method ::= "method" identifier parameters? "->" parameters? ";"
static Res<Method> _parseMethod(Io::SScan& s) {
    if (not s.skip("method"))
        return Error::invalidData("expected method");
    _eatWhitespace(s);

    auto name = try$(_parseIdentifier(s));
    _eatWhitespace(s);

    auto maybeRequest = _parseParameters(s);
    _eatWhitespace(s);

    if (not s.skip("->")) {
        if (not maybeRequest)
            return maybeRequest.none();
        return Error::invalidData("expected '->'");
    }
    if (not maybeRequest)
        maybeRequest = Ok(NONE);
    _eatWhitespace(s);

    auto maybeResponse = _parseParameters(s);
    _eatWhitespace(s);

    if (s.ahead(";") and not maybeResponse)
        maybeResponse = Ok(NONE);

    return Ok<Method>(name, try$(maybeRequest), try$(maybeResponse));
}

// method ::= "event" identifier payload? ";"
static Res<Event> _parseEvent(Io::SScan& s) {
    if (not s.skip("event"))
        return Error::invalidData("expected method");
    _eatWhitespace(s);

    auto name = try$(_parseIdentifier(s));
    _eatWhitespace(s);

    auto maybePayload = _parseParameters(s);
    _eatWhitespace(s);

    if (s.ahead(";") and not maybePayload)
        maybePayload = Ok(NONE);

    return Ok<Event>(name, try$(maybePayload));
}

// interface ::= "interface" identifier "{" (method ";")+ "}"
static Res<Interface> _parseInterface(Io::SScan& s) {
    if (not s.skip("interface"))
        return Error::invalidData("expected interface");
    _eatWhitespace(s);

    auto name = try$(_parseIdentifier(s));
    _eatWhitespace(s);

    if (not s.skip("{"))
        return Error::invalidData("expected '{'");
    _eatWhitespace(s);

    Vec<Method> methods;
    Vec<Event> events;
    do {
        _eatWhitespace(s);
        if (s.ahead("}"))
            break;
        if (s.ahead("method"))
            methods.pushBack(try$(_parseMethod(s)));
        else if (s.ahead("event"))
            events.pushBack(try$(_parseEvent(s)));
        else
            return Error::invalidData("expected method or event");
    } while (not s.ended() and s.skip(";"));

    if (not s.skip("}"))
        return Error::invalidData("expected '}'");
    _eatWhitespace(s);

    return Ok<Interface>(name, std::move(methods), std::move(events));
}

// module ::= "module" compound-identifier ";" interface*
export Res<Module> parseModule(Io::SScan& s) {
    if (not s.skip("module"))
        return Error::invalidData("expected method");
    _eatWhitespace(s);

    auto name = try$(_parseCompoundIdentifier(s));
    _eatWhitespace(s);

    if (not s.skip(";"))
        return Error::invalidData("expected ';'");
    _eatWhitespace(s);

    Vec<CompoundIdentifier> imports;
    while (s.skip("import")) {
        _eatWhitespace(s);

        imports.pushBack(try$(_parseCompoundIdentifier(s)));
        _eatWhitespace(s);

        if (not s.skip(";"))
            return Error::invalidData("expected ';'");
        _eatWhitespace(s);
    }

    Vec<Interface> interfaces;

    do {
        interfaces.pushBack(try$(_parseInterface(s)));
        _eatWhitespace(s);
    } while (not s.ended());

    return Ok<Module>(
        name,
        std::move(imports),
        std::move(interfaces)
    );
}

export Res<Module> parseModule(Str text) {
    Io::SScan s{text};
    return parseModule(s);
}

} // namespace Karm::Idl