export module Karm.Md:parser;

import Karm.Core;

import :base;

using namespace Karm::Re::Literals;

namespace Karm::Md {

Document _parseDocument(Io::SScan& s);

struct _ParagraphBlock {
    String text;
};

struct _HeadingBlock {
    usize level;
    String text;
};

struct _ListBlock {
    bool ordered;
    Vec<String> items;
};

struct _HtmlBlock {
    String text;
};

struct _QuoteBlock {
    Vec<String> paragraphs;
};

using _Block = Union<
    _ParagraphBlock,
    _HeadingBlock,
    _ListBlock,
    _HtmlBlock,
    _QuoteBlock,
    Code,
    Hr>;

struct _ListMarker {
    bool ordered;
    usize width;
};

bool _skipBlankLine(Io::SScan& s) {
    auto next = s;
    next.eat(' ');
    next.eat('\t');
    if (next.skip('\n')) {
        s = next;
        return true;
    }
    return false;
}

void _skipBlankLines(Io::SScan& s) {
    while (_skipBlankLine(s))
        ;
}

bool _isHr(Io::SScan s) {
    auto marker = s.peek();
    if (marker != '*' and marker != '-' and marker != '_')
        return false;

    usize count = 0;
    while (s.skip(marker))
        count++;

    return count >= 3 and (s.ahead('\n') or s.ended());
}

bool _isHtmlTagStart(Str text, usize index) {
    if (index >= text.len() or text[index] != '<' or index + 1 >= text.len())
        return false;

    auto next = text[index + 1];
    return isAsciiAlpha(next) or next == '/' or next == '!' or next == '?';
}

bool _isHtmlBlockStart(Io::SScan s) {
    if (not s.ahead('<'))
        return false;

    s.next();
    auto next = s.peek();
    return isAsciiAlpha(next) or next == '/' or next == '!' or next == '?';
}

bool _isQuoteBlockStart(Io::SScan s) {
    return s.ahead('>');
}

Opt<_ListMarker> _listMarker(Io::SScan s) {
    if (_isHr(s))
        return NONE;

    if (s.ahead('-') or s.ahead('*') or s.ahead('+')) {
        s.next();
        if (s.ahead(' ') or s.ahead('\t'))
            return _ListMarker{.ordered = false, .width = 1};
    }

    usize width = 0;
    while (isAsciiDigit(s.peek())) {
        s.next();
        width++;
    }

    if (width == 0)
        return NONE;

    if (s.skip('.') and (s.ahead(' ') or s.ahead('\t')))
        return _ListMarker{.ordered = true, .width = width + 1};

    return NONE;
}

bool _isBlockStart(Io::SScan s) {
    return s.ahead('#') or s.ahead("```") or _isHr(s) or _listMarker(s) != NONE or _isHtmlBlockStart(s) or _isQuoteBlockStart(s);
}

bool _matchesAt(Str text, usize index, Str marker) {
    return index + marker.len() <= text.len() and
           sub(text, index, index + marker.len()) == marker;
}

struct _InlineParseResult {
    Paragraph children;
    usize next;
    bool closed;
};

_InlineParseResult _parseInline(Str text, usize start, Str closing) {
    Paragraph children;
    usize textStart = start;
    usize index = start;

    auto flushText = [&] {
        if (textStart < index)
            children.pushBack(String{sub(text, textStart, index)});
    };

    while (index < text.len()) {
        if (closing.len() and _matchesAt(text, index, closing)) {
            flushText();
            return {
                .children = std::move(children),
                .next = index + closing.len(),
                .closed = true,
            };
        }

        if (_matchesAt(text, index, "**") or _matchesAt(text, index, "__")) {
            auto marker = sub(text, index, index + 2);
            auto inner = _parseInline(text, index + 2, marker);
            if (inner.closed) {
                flushText();
                children.pushBack(Bold{std::move(inner.children)});
                index = inner.next;
                textStart = index;
                continue;
            }
        }

        if (_matchesAt(text, index, "![")) {
            auto inner = _parseInline(text, index + 2, "]");
            if (inner.closed and inner.next < text.len() and text[inner.next] == '(') {
                usize close = inner.next + 1;
                while (close < text.len() and text[close] != ')')
                    close++;

                if (close < text.len()) {
                    flushText();
                    children.pushBack(Image{
                        .src = String{sub(text, inner.next + 1, close)},
                        .alt = String{sub(text, index + 2, inner.next - 1)},
                    });
                    index = close + 1;
                    textStart = index;
                    continue;
                }
            }
        }

        if (_isHtmlTagStart(text, index)) {
            usize close = index + 1;
            while (close < text.len() and text[close] != '>')
                close++;

            if (close < text.len()) {
                flushText();
                children.pushBack(Html{String{sub(text, index, close + 1)}});
                index = close + 1;
                textStart = index;
                continue;
            }
        }

        if (text[index] == '[') {
            auto inner = _parseInline(text, index + 1, "]");
            if (inner.closed and inner.next < text.len() and text[inner.next] == '(') {
                usize close = inner.next + 1;
                while (close < text.len() and text[close] != ')')
                    close++;

                if (close < text.len()) {
                    flushText();
                    children.pushBack(Link{
                        .href = String{sub(text, inner.next + 1, close)},
                        .children = std::move(inner.children),
                    });
                    index = close + 1;
                    textStart = index;
                    continue;
                }
            }
        }

        if (text[index] == '*' or text[index] == '_') {
            auto marker = sub(text, index, index + 1);
            auto inner = _parseInline(text, index + 1, marker);
            if (inner.closed) {
                flushText();
                children.pushBack(Italic{std::move(inner.children)});
                index = inner.next;
                textStart = index;
                continue;
            }
        }

        if (text[index] == '`') {
            usize close = index + 1;
            while (close < text.len() and text[close] != '`')
                close++;

            if (close < text.len()) {
                flushText();
                children.pushBack(InlineCode{String{sub(text, index + 1, close)}});
                index = close + 1;
                textStart = index;
                continue;
            }
        }

        index++;
    }

    if (closing.len()) {
        return {
            .children = {},
            .next = start,
            .closed = false,
        };
    }

    flushText();
    return {
        .children = std::move(children),
        .next = index,
        .closed = true,
    };
}

Paragraph _parseInline(Str text) {
    return _parseInline(text, 0, "").children;
}

String _takeLine(Io::SScan& s) {
    s.begin();
    while (not s.ahead('\n') and not s.ended())
        s.next();

    auto line = String{s.end()};
    s.skip('\n');
    return line;
}

bool _isIndentedListContinuation(Io::SScan s) {
    usize spaces = 0;
    while (s.ahead(' ')) {
        s.next();
        spaces++;
    }

    return spaces >= 2 or s.ahead('\t');
}

String _stripListContinuationIndent(Str line) {
    Io::SScan s{line};

    while (s.skip(' ') or s.skip('\t'))
        ;

    return String{s.remStr()};
}

String _parseParagraphText(Io::SScan& s) {
    s.begin();
    while (not s.ended()) {
        if (s.ahead('\n')) {
            auto next = s;
            next.skip('\n');

            if (next.ended() or _skipBlankLine(next) or _isBlockStart(next))
                break;
        }

        s.next();
    }

    auto text = String{s.end()};
    s.skip('\n');
    return text;
}

_ParagraphBlock _parseParagraph(Io::SScan& s) {
    return _ParagraphBlock{_parseParagraphText(s)};
}

_ListBlock _parseList(Io::SScan& s) {
    auto firstMarker = _listMarker(s).unwrap();
    Vec<String> items;

    while (not s.ended()) {
        auto marker = _listMarker(s);
        if (not marker or marker->ordered != firstMarker.ordered)
            break;

        s.next(marker->width);
        s.eat(' ');
        s.eat('\t');

        StringBuilder item;
        item.append(_takeLine(s));

        while (not s.ended()) {
            auto next = s;
            if (next.ahead('\n') or not _isIndentedListContinuation(next))
                break;

            auto line = _stripListContinuationIndent(_takeLine(s));
            item.append('\n');
            item.append(line);
        }

        items.pushBack(item.take());
    }

    return _ListBlock{
        .ordered = firstMarker.ordered,
        .items = std::move(items),
    };
}

_HtmlBlock _parseHtmlBlock(Io::SScan& s) {
    s.begin();

    while (not s.ended()) {
        while (not s.ahead('\n') and not s.ended())
            s.next();

        if (s.ended())
            break;

        auto next = s;
        next.skip('\n');
        if (_skipBlankLine(next))
            break;

        s.next();
    }

    auto text = String{s.end()};
    s.skip('\n');
    return _HtmlBlock{std::move(text)};
}

_QuoteBlock _parseQuote(Io::SScan& s) {
    Vec<String> paragraphs;
    StringBuilder paragraph;
    bool hasText = false;

    auto flushParagraph = [&] {
        if (hasText) {
            paragraphs.pushBack(paragraph.take());
            hasText = false;
        }
    };

    while (not s.ended() and _isQuoteBlockStart(s)) {
        s.skip('>');
        s.skip(' ');
        s.skip('\t');

        s.begin();
        while (not s.ahead('\n') and not s.ended())
            s.next();

        auto line = s.end();
        if (line.len() == 0) {
            flushParagraph();
        } else {
            if (hasText)
                paragraph.append('\n');
            paragraph.append(line);
            hasText = true;
        }

        s.skip('\n');
    }

    flushParagraph();
    return _QuoteBlock{std::move(paragraphs)};
}

_HeadingBlock _parseHeading(Io::SScan& s) {
    usize level = 0;
    while (s.peek() == '#') {
        level++;
        s.next();
    }

    s.skip(' ');
    s.begin();
    while (not s.ahead('\n') and not s.ended())
        s.next();

    auto text = String{s.end()};
    s.skip('\n');

    return _HeadingBlock{
        .level = level,
        .text = std::move(text),
    };
}

Code _parseCode(Io::SScan& s) {
    s.skip("```");

    while (not s.ahead('\n') and not s.ended())
        s.next();
    s.skip('\n');

    s.begin();
    while (not s.ended()) {
        if (s.ahead("```")) {
            auto code = s.end();
            s.skip("```");
            s.skip('\n');
            return Code{String{code}};
        }
        s.next();
    }

    return Code{String{s.end()}};
}

Hr _parseHr(Io::SScan& s) {
    auto marker = s.peek();
    while (s.skip(marker))
        ;
    s.skip('\n');
    return Hr{};
}

_Block _parseBlock(Io::SScan& s) {
    if (s.peek(0) == '#')
        return _parseHeading(s);
    else if (s.ahead("```"))
        return _parseCode(s);
    else if (_isHr(s))
        return _parseHr(s);
    else if (_listMarker(s))
        return _parseList(s);
    else if (_isHtmlBlockStart(s))
        return _parseHtmlBlock(s);
    else if (_isQuoteBlockStart(s))
        return _parseQuote(s);
    else
        return _parseParagraph(s);
}

Node _lowerBlock(_Block const& block) {
    return block.visit(
        [&](Code const& code) -> Node {
            return code;
        },
        [&](Hr const& hr) -> Node {
            return hr;
        },
        [&]( _HtmlBlock const& html) -> Node {
            return Html{html.text};
        },
        [&]( _QuoteBlock const& quote) -> Node {
            Vec<Paragraph> children;
            for (auto const& paragraph : quote.paragraphs)
                children.pushBack(_parseInline(paragraph));

            return Quote{std::move(children)};
        },
        [&]( _ListBlock const& list) -> Node {
            Vec<ListItem> items;
            for (auto const& item : list.items) {
                Io::SScan scan{item};
                items.pushBack(ListItem{.children = _parseDocument(scan).children});
            }

            return List{
                .ordered = list.ordered,
                .items = std::move(items),
            };
        },
        [&]( _HeadingBlock const& heading) -> Node {
            return Heading{
                .level = heading.level,
                .children = _parseInline(heading.text),
            };
        },
        [&]( _ParagraphBlock const& paragraph) -> Node {
            return _parseInline(paragraph.text);
        }
    );
}

Document _parseDocument(Io::SScan& s) {
    Vec<Node> children;
    _skipBlankLines(s);
    while (not s.ended()) {
        children.pushBack(_lowerBlock(_parseBlock(s)));
        _skipBlankLines(s);
    }
    return Document{children};
}

export Document parse(Io::SScan& s) {
    return _parseDocument(s);
}

export Document parse(Str str) {
    Io::SScan s{str};
    return parse(s);
}

} // namespace Karm::Md
