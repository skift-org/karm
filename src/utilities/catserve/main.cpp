#include <karm/entry>

import Karm.Cli;
import Karm.Sys;
import Karm.Md;
import Karm.Http;
import Karm.Template;
import Karm.Logger;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Fmt::Literals;
using namespace Karm::Ref::Literals;

static Str CAT = "ᓚ₍ ^. .^₎";

struct Manifest {
    String title;
    String header;
    String favicon;
    String navbar;
    String footer;
};

struct Site : Http::Handler {
    Ref::Url baseurl;
    Manifest manifest;
    String theme;
    String style;

    static Res<Rc<Site>> load(Ref::Url baseurl, Opt<Str> theme = "default"s) {
        auto site = makeRc<Site>();
        site->baseurl = baseurl;
        auto json = try$(Json::parse(try$(Sys::readAllUtf8(baseurl / "site.json"))));
        auto& manifest = site->manifest;
        manifest.title = json.get("title").asStr();
        manifest.header = json.get("header").asStr();
        manifest.favicon = json.getOr("favicon", "🐱"s).asStr();
        manifest.navbar = json.get("navbar").asStr();
        manifest.footer = json.get("footer").asStr();

        site->theme = try$(Sys::readAllUtf8("bundle://catserve/themes"_url / "{}.css"_f(theme)).wrapErr(Error::notFound("could not load theme")));

        auto style = Sys::readAllUtf8(baseurl / "style.css");
        site->style = style.unwrapOr(""s);

        return Ok(site);
    }

    String renderFavicon() const {
        auto svg =
            "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'>"
            "<text y='.9em' font-size='90'>{}</text>"
            "</svg>"_f(manifest.favicon);
        return "data:image/svg+xml,{}"_f(Ref::urlEncode(svg));
    }

    String renderHeader() const {
        auto header = Md::md2htmlFragment(manifest.header ?: manifest.title);
        return "<a class=\"title\" href=\"/\"><h1>{}</h1></a>"_f(header);
    }

    Ref::Url resolve(Ref::Url const& url) {
        auto path = url.path;
        path.absolutize();
        path.normalize();
        path.relativize();
        return baseurl / path;
    }

    Res<String> renderPage(Ref::Url const& url) const {
        auto md = Md::parse(try$(Sys::readAllUtf8(url)));
        md.walk(Visitor{
            [](Md::Link& link) {
                auto href = link.href;
                if (href.path.suffix() == "md")
                    link.href.path = href.path.parent(1) / "{}.html"_f(href.path.stem());
            },
            [](auto&) {
            }, // ignore
        });

        auto pageTitle = md.frontmatter.get("title");

        Serde::Object env{
            {"title"s, pageTitle ? "{} - {}"_f(pageTitle.asStr(), manifest.title) : manifest.title},
            {"favicon"s, renderFavicon()},
            {"theme"s, theme},
            {"style"s, style},
            {"header"s, renderHeader()},
            {"navbar"s, Md::md2htmlFragment(manifest.navbar)},
            {"main"s, Md::renderHtmlFragment(md)},
            {"footer"s, Md::md2htmlFragment(manifest.footer)},
        };

        String templ = R"html(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{.title}}</title>
    <link rel="icon" href="{{.favicon}}">
    <style>{{.theme}}</style>
    <style>{{.style}}</style>
</head>
<body>
    <header>
        {{.header}}
        <nav>{{.navbar}}</nav>
    </header>
    <main>
        {{.main}}
    </main>
    <footer>
        {{.footer}}
    </footer>
</body>
</html>)html"s;

        return Template::Document::eval(templ, env);
    }

    Async::Task<> handleAsync(Rc<Http::Request> req, Rc<Http::ResponseWriter> resp, Async::CancellationToken ct) override {
        if (req->method == Http::Method::GET) {
            auto url = resolve(req->url);

            if (Sys::isDir(url).unwrapOr(false)) {
                if (req->url.path.len() and not req->url.path.trailingSlash())
                    co_return co_await resp->redirectAsync(Http::Code::MOVED_PERMANENTLY, req->url / "."_path, ct);
                url = url / "index.html";
            }

            if (Sys::isFile(url).unwrapOr(false)) {
                resp->code = Http::Code::OK;
                co_return co_await resp->writeFileAsync(url, ct);
            }

            auto rewriten = url.parent(1) / "{}.md"_f(url.path.stem());
            auto result = renderPage(rewriten);

            if (not result)
                co_return co_await resp->notFoundAsync(ct);

            resp->code = Http::Code::OK;
            co_return co_await resp->writeStrAsync(result.take(), ct);
        } else {
            co_return co_await resp->notFoundAsync(ct);
        }
    }
};

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto inputArg = Cli::operand<Ref::Url>("input"s, "Path to the directory containing the site"s, "bundle://catserve/public/site"_url);
    auto themeArg = Cli::option<Str>(NONE, "theme"s, "Theme to use."s, "default");
    auto portArg = Cli::option<isize>('p', "port"s, "TCP Port to serve the website on."s, 8080);

    Cli::Command cmd{
        "catserve"s,
        "Tiny static markdown server"s,
        {
            Cli::Section{
                "Site"s,
                {inputArg, themeArg, portArg}
            },
        }
    };

    co_trya$(cmd.execAsync(env));
    if (not cmd)
        co_return Ok();

    logInfo("{} starting...", CAT);
    auto site = co_try$(Site::load(inputArg, themeArg.value()));
    co_return co_await Http::serveAsync(
        site,
        {
            .name = "Cat"s,
            .addr = Sys::Ip4::localhost(portArg.value()),
        },
        ct
    );
}
