export module Karm.Md:html;

import Karm.Logger;

import :base;
import :parser;

using namespace Karm::Literals;

namespace Karm::Md {

void _renderParagraphContent(Paragraph const& p, Io::Emit& e);
void _renderBlock(Node const& node, Io::Emit& e);

export String htmlEscape(Str str) {
    StringBuilder sw;
    sw.ensure(str.len());
    for (auto r : iterRunes(str)) {
        if (r == '&')
            sw.append("&amp;"s);
        else if (r == '<')
            sw.append("&lt;"s);
        else if (r == '>')
            sw.append("&gt;"s);
        else if (r == '"')
            sw.append("&quot;"s);
        else if (r == '\'')
            sw.append("&#39;"s);
        else
            sw.append(r);
    }
    return sw.take();
}

void _renderSpan(Node const& node, Io::Emit& e) {
    node.visit(
        [&](String const& s) {
            e("{}", htmlEscape(s));
        },
        [&](InlineCode const& s) {
            e("<code>{}</code>", htmlEscape(s.text));
        },
        [&](Html const& s) {
            e("{}", s.text);
        },
        [&](Italic const& s) {
            e("<em>");
            _renderParagraphContent(s.children, e);
            e("</em>");
        },
        [&](Bold const& s) {
            e("<strong>");
            _renderParagraphContent(s.children, e);
            e("</strong>");
        },
        [&](Link const& s) {
            e("<a href=\"{}\">", htmlEscape(s.href.str()));
            _renderParagraphContent(s.children, e);
            e("</a>");
        },
        [&](Image const& s) {
            e("<img src=\"{}\" alt=\"{}\"/>", htmlEscape(s.src.str()), htmlEscape(s.alt));
        },
        [&](auto const& n) {
            logWarn("could not render {} as a span", n);
        }
    );
}

void _renderParagraphContent(Paragraph const& p, Io::Emit& e) {
    for (auto const& n : p)
        _renderSpan(n, e);
}

void _renderListItem(ListItem const& item, Io::Emit& e) {
    for (usize index = 0; index < item.children.len(); index++) {
        auto const& child = item.children[index];

        child.visit(
            [&](Paragraph const& p) {
                if (index == 0)
                    _renderParagraphContent(p, e);
                else {
                    e("<p>");
                    _renderParagraphContent(p, e);
                    e("</p>");
                }
            },
            [&](auto const&) {
                _renderBlock(child, e);
            }
        );
    }
}

void _renderBlock(Node const& node, Io::Emit& e) {
    node.visit(
        [&](Heading const& h) {
            e("<h{}>", h.level);
            _renderParagraphContent(h.children, e);
            e("</h{}>", h.level);
        },
        [&](Paragraph const& p) {
            e("<p>");
            _renderParagraphContent(p, e);
            e("</p>");
        },
        [&](Code const& p) {
            e("<pre><code>{}</code></pre>", htmlEscape(p.text));
        },
        [&](Html const& h) {
            e("{}", h.text);
        },
        [&](List const& l) {
            if (l.ordered)
                e("<ol>");
            else
                e("<ul>");

            for (auto const& item : l.items) {
                e("<li>");
                _renderListItem(item, e);
                e("</li>");
            }

            if (l.ordered)
                e("</ol>");
            else
                e("</ul>");
        },
        [&](Quote const& q) {
            e("<blockquote>");
            for (auto const& paragraph : q.children) {
                e("<p>");
                _renderParagraphContent(paragraph, e);
                e("</p>");
            }
            e("</blockquote>");
        },
        [&](Hr const&) {
            e("<hr/>");
        },
        [&](auto const& n) {
            logWarn("could not render {} as a block", n);
        }
    );
}

export void renderHtmlFragment(Document const& doc, Io::Emit& e) {
    for (auto const& n : doc.children)
        _renderBlock(n, e);
}

export String renderHtmlFragment(Document const& doc) {
    Io::StringWriter sw;
    Io::Emit e{sw};
    renderHtmlFragment(doc, e);
    return sw.take();
}

export String md2htmlFragment(Str markdown) {
    return renderHtmlFragment(parse(markdown));
}

export void renderHtml(Document const& doc, Io::Emit& e) {
    e(R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"></head><body>)");
    renderHtmlFragment(doc, e);
    e(R"(</body></html>)");
}

export String renderHtml(Document const& doc) {
    Io::StringWriter sw;
    Io::Emit e{sw};
    renderHtml(doc, e);
    return sw.take();
}

export String md2html(Str markdown) {
    return renderHtml(parse(markdown));
}

} // namespace Karm::Md
