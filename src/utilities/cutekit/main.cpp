#include <karm/entry>

import Karm.Icu;
import Karm.Logger;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Re::Literals;

namespace Cutekit {

// MARK: Tags ------------------------------------------------------------------

static Symbol MT_ROOT = "%root"_sym;
static Symbol MT_CONFIG = "%config"_sym;
static Symbol MT_BUILD = "%build"_sym;
static Symbol MT_LOCK = "%lock"_sym;

static Symbol T_AGGREGATE = "aggregate"_sym;
static Symbol T_APPEND = "append"_sym;
static Symbol T_BUNDLES = "bundles"_sym;
static Symbol T_CONFIG = "config"_sym;
static Symbol T_DEFAULT = "default"_sym;
static Symbol T_DISABLED = "disabled"_sym;
static Symbol T_ENABLE_IF = "enable-if"_sym;
static Symbol T_EXECUTABLE = "executable"_sym;
static Symbol T_EXPORT = "export"_sym;
static Symbol T_EXTERN = "extern"_sym;
static Symbol T_GETENV = "getenv"_sym;
static Symbol T_IMPORT = "import"_sym;
static Symbol T_INCLUDES = "includes"_sym;
static Symbol T_WHOLE = "whole"_sym;
static Symbol T_LIBRARBY = "librarby"_sym; // is that a undertale reference ???
static Symbol T_LIBRARY = "library"_sym;
static Symbol T_LOCK = "lock"_sym;
static Symbol T_MATCH = "match"_sym;
static Symbol T_NAME = "name"_sym;
static Symbol T_NOT = "not"_sym;
static Symbol T_PRODUCT = "product"_sym;
static Symbol T_PROVIDES = "provides"_sym;
static Symbol T_REF = "ref"_sym;
static Symbol T_REQUIRES = "requires"_sym;
static Symbol T_RUNTIME = "runtime"_sym;
static Symbol T_SOURCES = "sources"_sym;
static Symbol T_TAG = "tag"_sym;
static Symbol T_TARGET = "target"_sym;
static Symbol T_TOOLCHAIN = "toolchain"_sym;
static Symbol T_URL = "url"_sym;

static bool _isSpaceNoNewline(Rune c) {
    return c != '\n' and Icu::isSpace(c);
}

struct Tag {
    Symbol key;
    String value = ""s;
    Vec<Tag> children = {};

    static Res<Tag> parseTag(Io::SScan& s) {
        Tag tag;
        if (s.ahead("\"")) {
            tag.key = Symbol::from(try$(Json::parseStr(s)));
        } else {
            tag.key = Symbol::from(s.token(
                ('%'_re | Re::icuProperty(&Icu::Properties::idStart)) &
                Re::zeroOrMore(('-'_re | Re::icuProperty(&Icu::Properties::idContinue)))
            ));

            if (not tag.key)
                return Error::invalidData("expected key");
        }

        s.eat(Re::ctype(_isSpaceNoNewline));
        if (s.ahead('\n'))
            return Ok(std::move(tag));
        s.eat(Re::space());

        if (s.ahead(","))
            return Ok(std::move(tag));

        if (s.ahead("\"")) {
            tag.value = try$(Json::parseStr(s));
        } else {
            tag.value = s.token(
                Re::icuProperty(&Icu::Properties::idStart) &
                Re::zeroOrMore(Re::icuProperty(&Icu::Properties::idContinue))
            );
        }

        s.eat(Re::space());

        if (not s.skip("{"))
            return Ok(std::move(tag));

        while (not s.ended()) {
            s.eat(Re::space());
            if (s.skip("}"))
                return Ok(std::move(tag));

            tag.children.pushBack(try$(parseTag(s)));
            s.eat(Re::space());
            s.skip(",");
        }

        return Error::invalidData("expected '}'");
    }

    static Res<Tag> parse(Io::SScan& s) {
        Tag root = {MT_ROOT};
        s.eat(Re::space());
        if (s.ahead("%"))
            root = try$(parseTag(s));
        s.eat(Re::space());
        while (not s.ended()) {
            root.children.pushBack(try$(parseTag(s)));
            s.eat(Re::space());
            s.skip(",");
        }
        return Ok(std::move(root));
    }

    static Res<Tag> parse(Str text) {
        Io::SScan s{text};
        return parse(s);
    }

    void repr(Io::Emit& e) const {
        e("(tag {} {} {})", key, value, children);
    }
};

// MARK: Model -----------------------------------------------------------------

// MARK: Eval ------------------------------------------------------------------

struct BuildContext {
    Ref::Url rootUrl;
    Ref::Url currUrl = rootUrl;
    Ref::Url externUrl = rootUrl / "externs"s;

    Ref::Url resolve(Str path) {
        if (startWith(path, "//"s) != Match::NO)
            return rootUrl / Str{next(path, 2)};
        return currUrl / path;
    }
};

Res<> evalImport(BuildContext& ctx, Str path);

Res<> evalBuild(BuildContext& ctx, Tag& expr) {
    for (auto& c : expr.children) {
        if (c.key == T_IMPORT)
            try$(evalImport(ctx, c.value.str()));
        else
            logWarn("unknow directive {:#}", c.key);
    }

    return Ok();
}

Res<> evalImport(BuildContext& ctx, Str path) {
    auto url = ctx.resolve(path);
    auto text = try$(Sys::readAllText(url));
    auto expr = try$(Tag::parse(text));
    auto inner = ctx;
    inner.currUrl = url.parent(1);
    return evalBuild(inner, expr);
}

} // namespace Cutekit

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    Cutekit::BuildContext ctx{
        .rootUrl = env.cwd()
    };
    co_try$(Cutekit::evalImport(ctx, "build.ck"s));
    co_return Ok();
}
