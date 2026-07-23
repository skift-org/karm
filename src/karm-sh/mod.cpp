module;

#include <karm/macros>

export module Karm.Sh;

import Karm.Core;
import Karm.Sys;
import Karm.Diag;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Re::Literals;

namespace Karm::Sh {

static constexpr auto RE_SEGMENT = Re::oneOrMore(Re::alnum() | '_'_re | '-'_re | '/'_re | ':'_re | '.'_re);

export struct Context {
    Ref::Url pwd;
};

export struct Command {
    Io::LocSpan span;
    Vec<String> segments;

    Res<> eval() {
        Sys::Command c{
            segments[0],
            next(segments, 1),
        };
        auto proc = try$(c.spawn());
        return proc.wait();
    }

    void repr(Io::Emit& e) const {
        e("(command {:#})", segments);
    }
};

export struct Pipeline {
    Vec<Command> commands;

    Res<> eval() {
        Vec<Sys::Process> procs;
        Rc<Sys::Fd> prev = Sys::in().fd();
        for (auto& c : mutSub(commands, 0, commands.len() - 1)) {
            auto [in, out] = try$(Sys::Pipe::create());
            Sys::Command cmd = {
                .exe = c.segments[0],
                .args = next(c.segments, 1),
                .in = prev,
                .out = in.fd(),
            };
            procs.pushBack(try$(cmd.spawn()));
            prev = out.fd();
        }

        auto& c = last(commands);
        Sys::Command cmd = {
            .exe = c.segments[0],
            .args = next(c.segments, 1),
            .in = prev,
            .out = Sys::out().fd(),
        };

        procs.pushBack(try$(cmd.spawn()));

        return last(procs).wait();
    }

    void repr(Io::Emit& e) const {
        e("(pipeline {:#})", commands);
    }
};

static void _eatWhitespace(Io::SScan& s) {
    s.eat(Re::blank());
}

static Res<String> _parseSegment(Io::SScan& s, Diag::Collector& c) {
    auto seg = s.token(RE_SEGMENT);

    if (not seg) {
        c.emit(
            Diag::Diagnostic::error("expected segment"s)
                .withLabel(Diag::Label::here(s.loc()))
        );
        return Error::invalidInput("expect segment");
    }

    return Ok(seg);
}

static Res<Command> _parseCommand(Io::SScan& s, Diag::Collector& c) {
    Command command;
    command.span = Io::LocSpan::single(s.loc());

    while (not s.ended() and s.ahead(RE_SEGMENT)) {
        command.segments.pushBack(try$(_parseSegment(s, c)));
        _eatWhitespace(s);
    }

    if (not command.segments.len()) {
        c.emit(
            Diag::Diagnostic::error("expected command"s)
                .withLabel(Diag::Label::here(s.loc()))
        );
        return Error::invalidInput("expect command");
    }

    command.span.end = s.loc();
    return Ok(std::move(command));
}

static Res<Pipeline> _parsePipeline(Io::SScan& s, Diag::Collector& c) {
    Pipeline p;
    p.commands.pushBack(try$(_parseCommand(s, c)));
    while (not s.ended() and s.skip("|")) {
        _eatWhitespace(s);
        p.commands.pushBack(try$(_parseCommand(s, c)));
    }
    return Ok(std::move(p));
}

Res<Pipeline> parseLine(Io::SScan& s, Diag::Collector& c) {
    _eatWhitespace(s);
    auto pipeline = try$(_parsePipeline(s, c));
    _eatWhitespace(s);
    if (not s.ended()) {
        c.emit(
            Diag::Diagnostic::error("unexpected command end"s)
                .withLabel(Diag::Label::here(s.loc()))
        );
        return Error::invalidInput("expect command");
    }
    return Ok(std::move(pipeline));
}

export Res<Pipeline> parse(Str str, Diag::Collector& c) {
    Io::SScan s{str};
    return parseLine(s, c);
}

} // namespace Karm::Sh
