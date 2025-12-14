export module Karm.Idl:ast;

import Karm.Core;

namespace Karm::Idl {

using Identifier = Symbol;

using CompoundIdentifier = Vec<Symbol>;

struct Type {
    CompoundIdentifier name;
    Vec<Type> params;

    void repr(Io::Emit& e) const {
        if (params)
            e("(type {} {})", name, params);
        else
            e("(type {})", name);
    }
};

struct Parameter {
    Identifier name;
    Type type;

    void repr(Io::Emit& e) const {
        e("(parameter {} {})", name, type);
    }
};

using Parameters = Vec<Parameter>;

struct Method {
    Identifier name;
    Union<None, Type, Parameters> request = NONE;
    Union<None, Type, Parameters> response = NONE;

    void repr(Io::Emit& e) const {
        e("(method {} {} {})", name, request, request);
    }
};

struct Event {
    Identifier name;
    Union<None, Type, Parameters> payload = NONE;

    void repr(Io::Emit& e) const {
        e("(event {} {})", name, payload);
    }
};

struct Interface {
    Identifier name;
    Vec<Method> methods;
    Vec<Event> events;

    void repr(Io::Emit& e) const {
        e("(interface {} methods:{} events:{})", name, methods, events);
    }
};

struct Module {
    CompoundIdentifier name;
    Vec<CompoundIdentifier> imports;
    Vec<Interface> interfaces;

    void repr(Io::Emit& e) const {
        e("(module {} imports:{} interfaces:{})", name, imports, interfaces);
    }
};

} // namespace Karm::Idl