#include <karm/test>

import Karm.Md;

using namespace Karm::Literals;

namespace Karm::Md::Tests {

test$("markdown-inline-formatting") {
    auto html = renderHtml(parse("## *Italic* and **bold**\n\nParagraph with `code`."));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><h2><em>Italic</em> and <strong>bold</strong></h2><p>Paragraph with <code>code</code>.</p></body></html>"s
    );

    return Ok();
}

test$("markdown-paragraph-starting-with-inline-code") {
    auto html = renderHtml(parse("`./ck test`: Run all tests."));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><p><code>./ck test</code>: Run all tests.</p></body></html>"s
    );

    return Ok();
}

test$("markdown-block-splitting-before-inline") {
    auto html = renderHtml(parse("Paragraph with\na soft line and **bold**.\n\n---\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><p>Paragraph with\na soft line and <strong>bold</strong>.</p><hr/></body></html>"s
    );

    return Ok();
}

test$("markdown-inline-links") {
    auto html = renderHtml(parse("See [**docs**](https://example.com/docs) now."));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><p>See <a href=\"https://example.com/docs\"><strong>docs</strong></a> now.</p></body></html>"s
    );

    return Ok();
}

test$("markdown-inline-images") {
    auto html = renderHtml(parse("Logo: ![Paper Muncher](./doc/assets/logo-light.png)."));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><p>Logo: <img src=\"./doc/assets/logo-light.png\" alt=\"Paper Muncher\"/>.</p></body></html>"s
    );

    return Ok();
}

test$("markdown-lists") {
    auto html = renderHtml(parse("- One\n- Two with [link](./two)\n\n1. First\n2. Second with `code`\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><ul><li>One</li><li>Two with <a href=\"./two\">link</a></li></ul><ol><li>First</li><li>Second with <code>code</code></li></ol></body></html>"s
    );

    return Ok();
}

test$("markdown-nested-lists") {
    auto html = renderHtml(parse("- Parent\n  - Child A\n  - Child B\n- Sibling\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><ul><li>Parent<ul><li>Child A</li><li>Child B</li></ul></li><li>Sibling</li></ul></body></html>"s
    );

    return Ok();
}

test$("markdown-nested-lists-four-space-indent") {
    auto html = renderHtml(parse("* Parent\n    * Child A\n    * Child B\n* Sibling\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><ul><li>Parent<ul><li>Child A</li><li>Child B</li></ul></li><li>Sibling</li></ul></body></html>"s
    );

    return Ok();
}

test$("markdown-inline-html") {
    auto html = renderHtml(parse("Line with <br/> raw HTML."));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><p>Line with <br/> raw HTML.</p></body></html>"s
    );

    return Ok();
}

test$("markdown-html-block") {
    auto html = renderHtml(parse("<p align=\"center\">\n<img src=\"logo.png\" />\n</p>\n\n# Title\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><p align=\"center\">\n<img src=\"logo.png\" />\n</p><h1>Title</h1></body></html>"s
    );

    return Ok();
}

test$("markdown-blockquote") {
    auto html = renderHtml(parse("> **Warning**<br> Here be dragons!\n\nAfter quote.\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><blockquote><p><strong>Warning</strong><br> Here be dragons!</p></blockquote><p>After quote.</p></body></html>"s
    );

    return Ok();
}

test$("markdown-blockquote-multiple-paragraphs") {
    auto html = renderHtml(parse("> First line\n> still first paragraph\n>\n> Second paragraph\n"));

    expectEq$(
        html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><blockquote><p>First line\nstill first paragraph</p><p>Second paragraph</p></blockquote></body></html>"s
    );

    return Ok();
}

} // namespace Karm::Md::Tests
