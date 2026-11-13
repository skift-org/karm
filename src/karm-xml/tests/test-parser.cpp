#include <karm/test>

import Karm.Xml;

using namespace Karm::Literals;

namespace Karm::Xml::Tests {

// MARK: Helpers ---------------------------------------------------------------

static Vec<Event> collect(Str source, Opt<Symbol> ns = NONE, Mode mode = Mode::DOCUMENT) {
    Io::SScan s{source};
    return Parser::parse(s, ns, mode) | Collect<Vec<Event>>();
}

static QualifiedName html(Str name) {
    return {Some(Xml::HTML_NAMESPACE), Symbol::from(name)};
}

static QualifiedName svg(Str name) {
    return {Some(Xml::SVG_NAMESPACE), Symbol::from(name)};
}

static QualifiedName bare(Str name) {
    return {NONE, Symbol::from(name)};
}

static bool start(Event const& event, QualifiedName const& name) {
    if (auto it = event.is<StartElement>())
        return it->name == name;
    return false;
}

static bool end(Event const& event, QualifiedName const& name) {
    if (auto it = event.is<EndElement>())
        return it->name == name;
    return false;
}

static bool text(Event const& event, Str data) {
    if (auto it = event.is<Text>())
        return it->data == data;
    return false;
}

static bool comment(Event const& event, Str data) {
    if (auto it = event.is<Comment>())
        return it->data == data;
    return false;
}

static bool rror(Event const& event) {
    return event.is<Error>() != NONE;
}

static usize countAttrs(Event const& event) {
    if (auto it = event.is<StartElement>())
        return it->attrs.len();
    return 0;
}

static Opt<Str> attr(Event const& event, QualifiedName const& name) {
    if (auto it = event.is<StartElement>())
        for (auto& a : it->attrs)
            if (a.name == name)
                return Some(Str{a.value});
    return NONE;
}

// MARK: Elements --------------------------------------------------------------

test$("xml-events-empty-document") {
    // An empty document is invalid
    auto events = collect(""s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-open-close-tag") {
    auto events = collect("<html></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], html("html")));
    assert$(end(events[1], html("html")));

    return Ok();
}

test$("xml-events-empty-tag") {
    // An empty-element tag is reported just like an open/close pair
    auto events = collect("<html/>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], html("html")));
    assert$(end(events[1], html("html")));

    return Ok();
}

test$("xml-events-nested-tags") {
    auto events = collect("<html><head></head><body></body></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 6uz);
    assert$(start(events[0], html("html")));
    assert$(start(events[1], html("head")));
    assert$(end(events[2], html("head")));
    assert$(start(events[3], html("body")));
    assert$(end(events[4], html("body")));
    assert$(end(events[5], html("html")));

    return Ok();
}

test$("xml-events-deeply-nested-tags") {
    // Nesting lives in an explicit stack, depth shouldn't be an issue
    StringBuilder sb;
    for (usize i = 0; i < 2000; i++)
        sb.append("<a>"s);
    for (usize i = 0; i < 2000; i++)
        sb.append("</a>"s);
    auto source = sb.take();

    auto events = collect(source);

    assertEq$(events.len(), 4000uz);
    assert$(start(events[0], bare("a")));
    assert$(start(events[1999], bare("a")));
    assert$(end(events[2000], bare("a")));
    assert$(end(events[3999], bare("a")));

    return Ok();
}

// MARK: Attributes ------------------------------------------------------------

test$("xml-events-attr") {
    auto events = collect("<html lang=\"en\"/>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], html("html")));
    assertEq$(countAttrs(events[0]), 1uz);
    assert$(attr(events[0], bare("lang")) == "en"s);
    assert$(attr(events[0], bare("dir")) == NONE);

    return Ok();
}

test$("xml-events-attr-whitespace-around-eq") {
    auto events = collect("<html lang = \"en\" dir =\"ltr\"/>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 2uz);
    assertEq$(countAttrs(events[0]), 2uz);
    assert$(attr(events[0], bare("lang")) == "en"s);
    assert$(attr(events[0], bare("dir")) == "ltr"s);

    return Ok();
}

test$("xml-events-attr-single-quotes") {
    auto events = collect("<a b='it\"s'/>"s);

    assertEq$(events.len(), 2uz);
    assert$(attr(events[0], bare("b")) == "it\"s"s);

    return Ok();
}

test$("xml-events-attr-document-order") {
    auto events = collect("<a z=\"1\" y=\"2\" x=\"3\"/>"s);

    assertEq$(events.len(), 2uz);

    auto start = events[0].is<StartElement>();
    assert$(start != NONE);
    assertEq$(start->attrs.len(), 3uz);
    assert$(start->attrs[0].name == bare("z"));
    assert$(start->attrs[1].name == bare("y"));
    assert$(start->attrs[2].name == bare("x"));

    return Ok();
}

test$("xml-events-attr-references") {
    auto events = collect("<a b=\"&lt;&amp;&gt;&quot;&apos;&#65;&#x42;\"/>"s);

    assertEq$(events.len(), 2uz);
    assert$(attr(events[0], bare("b")) == "<&>\"'AB"s);

    return Ok();
}

test$("xml-events-attr-stray-ampersand") {
    // NOSPEC: A stray '&' in an attribute value is kept as is
    auto events = collect("<a href=\"?x=1&y=2\"/>"s);

    assertEq$(events.len(), 2uz);
    assert$(attr(events[0], bare("href")) == "?x=1&y=2"s);

    return Ok();
}

// MARK: Text ------------------------------------------------------------------

test$("xml-events-text") {
    auto events = collect("<html>text</html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 3uz);
    assert$(start(events[0], html("html")));
    assert$(text(events[1], "text"s));
    assert$(end(events[2], html("html")));

    return Ok();
}

test$("xml-events-text-before-tag") {
    auto events = collect("<html>text<div/></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 5uz);
    assert$(start(events[0], html("html")));
    assert$(text(events[1], "text"s));
    assert$(start(events[2], html("div")));
    assert$(end(events[3], html("div")));
    assert$(end(events[4], html("html")));

    return Ok();
}

test$("xml-events-text-after-tag") {
    auto events = collect("<html><div/>text</html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 5uz);
    assert$(start(events[0], html("html")));
    assert$(start(events[1], html("div")));
    assert$(end(events[2], html("div")));
    assert$(text(events[3], "text"s));
    assert$(end(events[4], html("html")));

    return Ok();
}

test$("xml-events-text-between-tags") {
    auto events = collect("<html><div/>text<div/></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 7uz);
    assert$(start(events[0], html("html")));
    assert$(start(events[1], html("div")));
    assert$(end(events[2], html("div")));
    assert$(text(events[3], "text"s));
    assert$(start(events[4], html("div")));
    assert$(end(events[5], html("div")));
    assert$(end(events[6], html("html")));

    return Ok();
}

test$("xml-events-text-between-tags-and-before") {
    auto events = collect("<html>test2<div>text</div></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 6uz);
    assert$(start(events[0], html("html")));
    assert$(text(events[1], "test2"s));
    assert$(start(events[2], html("div")));
    assert$(text(events[3], "text"s));
    assert$(end(events[4], html("div")));
    assert$(end(events[5], html("html")));

    return Ok();
}

test$("xml-events-text-whitespace-is-reported") {
    // Whitespace-only text is up to the consumer to deal with
    auto events = collect("<a> <b/>\n</a>"s);

    assertEq$(events.len(), 6uz);
    assert$(start(events[0], bare("a")));
    assert$(text(events[1], " "s));
    assert$(start(events[2], bare("b")));
    assert$(end(events[3], bare("b")));
    assert$(text(events[4], "\n"s));
    assert$(end(events[5], bare("a")));

    return Ok();
}

test$("xml-events-text-references") {
    auto events = collect("<a>&lt;b&gt; &amp; &#65;&#x42;&nbsp;</a>"s);

    assertEq$(events.len(), 3uz);
    assert$(text(events[1], "<b> & AB\u00A0"s));

    return Ok();
}

test$("xml-events-text-cdata") {
    auto events = collect("<a><![CDATA[<b> & ]] > </b>]]></a>"s);

    assertEq$(events.len(), 3uz);
    assert$(text(events[1], "<b> & ]] > </b>"s));

    return Ok();
}

test$("xml-events-text-empty-cdata") {
    // No Text event for an empty run
    auto events = collect("<a><![CDATA[]]></a>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(end(events[1], bare("a")));

    return Ok();
}

test$("xml-events-text-runs-are-merged") {
    // Character data, CDATA sections and references end up in a single event
    auto events = collect("<a>x<![CDATA[<y>]]>&amp;z</a>"s);

    assertEq$(events.len(), 3uz);
    assert$(text(events[1], "x<y>&z"s));

    return Ok();
}

test$("xml-events-text-split-by-markup") {
    // ...but comments and processing instructions do split it
    auto events = collect("<a>x<!--c-->y<?pi?>z</a>"s);

    assertEq$(events.len(), 6uz);
    assert$(start(events[0], bare("a")));
    assert$(text(events[1], "x"s));
    assert$(comment(events[2], "c"s));
    assert$(text(events[3], "y"s));
    assert$(text(events[4], "z"s));
    assert$(end(events[5], bare("a")));

    return Ok();
}

// MARK: Comments & Processing Instructions ------------------------------------

test$("xml-events-comment") {
    auto events = collect("<html><!-- comment --></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 3uz);
    assert$(start(events[0], html("html")));
    assert$(comment(events[1], " comment "s));
    assert$(end(events[2], html("html")));

    return Ok();
}

test$("xml-events-comment-with-gt-symb") {
    auto events = collect(
        "<title>im a title!</title>"
        "<!-- a b <meta> c d -->"s,
        Some(Xml::HTML_NAMESPACE)
    );

    assertEq$(events.len(), 4uz);
    assert$(start(events[0], html("title")));
    assert$(text(events[1], "im a title!"s));
    assert$(end(events[2], html("title")));
    assert$(comment(events[3], " a b <meta> c d "s));

    return Ok();
}

test$("xml-events-comment-in-prolog") {
    auto events = collect("<!--a--><!DOCTYPE html><!--b--><html/>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 5uz);
    assert$(comment(events[0], "a"s));
    assert$(events[1].is<Doctype>() != NONE);
    assert$(comment(events[2], "b"s));
    assert$(start(events[3], html("html")));
    assert$(end(events[4], html("html")));

    return Ok();
}

test$("xml-events-processing-instructions-are-ignored") {
    auto events = collect("<?before a?><a><?inside b?></a><?after c?>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(end(events[1], bare("a")));

    return Ok();
}

// MARK: Prolog ----------------------------------------------------------------

test$("xml-events-doctype") {
    auto events = collect("<!DOCTYPE html><html></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 3uz);

    auto doctype = events[0].is<Doctype>();
    assert$(doctype != NONE);
    assert$(doctype->name == "html"s);
    assertEq$(doctype->publicId, ""s);
    assertEq$(doctype->systemId, ""s);

    assert$(start(events[1], html("html")));
    assert$(end(events[2], html("html")));

    return Ok();
}

test$("xml-events-doctype-public") {
    auto events = collect(
        "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">"
        "<svg/>"s
    );

    assertEq$(events.len(), 3uz);

    auto doctype = events[0].is<Doctype>();
    assert$(doctype != NONE);
    assert$(doctype->name == "svg"s);
    assertEq$(doctype->publicId, "-//W3C//DTD SVG 1.1//EN"s);
    assertEq$(doctype->systemId, "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"s);

    return Ok();
}

test$("xml-events-doctype-system") {
    auto events = collect("<!DOCTYPE a SYSTEM \"a.dtd\"><a/>"s);

    assertEq$(events.len(), 3uz);

    auto doctype = events[0].is<Doctype>();
    assert$(doctype != NONE);
    assertEq$(doctype->publicId, ""s);
    assertEq$(doctype->systemId, "a.dtd"s);

    return Ok();
}

test$("xml-events-xml-decl") {
    auto events = collect("<?xml version='1.0' encoding='UTF-8'?><html></html>"s, Some(Xml::HTML_NAMESPACE));

    assertEq$(events.len(), 3uz);

    auto decl = events[0].is<XmlDecl>();
    assert$(decl != NONE);
    assertEq$(decl->version, "1.0"s);
    assert$(decl->encoding == "UTF-8"s);
    // NOTE: Defaulting standalone to "no" is up to the consumer
    assert$(decl->standalone == NONE);

    assert$(start(events[1], html("html")));
    assert$(end(events[2], html("html")));

    return Ok();
}

test$("xml-events-xml-decl-standalone") {
    auto events = collect("<?xml version=\"1.0\" standalone=\"yes\" ?>\n<a/>"s);

    assertEq$(events.len(), 3uz);

    auto decl = events[0].is<XmlDecl>();
    assert$(decl != NONE);
    assertEq$(decl->version, "1.0"s);
    assert$(decl->encoding == NONE);
    assert$(decl->standalone == "yes"s);

    return Ok();
}

// MARK: Namespaces ------------------------------------------------------------

test$("xml-events-no-default-namespace") {
    auto events = collect("<a b=\"c\"/>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(attr(events[0], bare("b")) == "c"s);

    return Ok();
}

test$("xml-events-different-namespace") {
    auto events = collect(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 0 0\">"
        "<rect/>"
        "</svg>"s,
        Some(Xml::HTML_NAMESPACE)
    );

    assertEq$(events.len(), 4uz);
    assert$(start(events[0], svg("svg")));
    assert$(start(events[1], svg("rect")));
    assert$(end(events[2], svg("rect")));
    assert$(end(events[3], svg("svg")));

    // The default namespace doesn't apply to attributes, and namespace
    // declarations are reported along the other attributes.
    assertEq$(countAttrs(events[0]), 2uz);
    assert$(attr(events[0], bare("viewBox")) == "0 0 0 0"s);
    assert$(attr(events[0], bare("xmlns")) == "http://www.w3.org/2000/svg"s);

    return Ok();
}

test$("xml-events-prefixed-names") {
    auto events = collect(
        "<root xmlns:a=\"http://www.example.org/a\">"
        "<child a:foo=\"bar\"/>"
        "<a:item/>"
        "</root>"s
    );

    auto exampleNs = "http://www.example.org/a"_sym;

    assertEq$(events.len(), 6uz);
    assert$(start(events[0], bare("root")));
    assert$(attr(events[0], QualifiedName{Some(XMLNS_NAMESPACE), "a"_sym}) == "http://www.example.org/a"s);

    assert$(start(events[1], bare("child")));
    assert$(attr(events[1], QualifiedName{Some(exampleNs), "foo"_sym}) == "bar"s);
    assert$(attr(events[1], bare("foo")) == NONE);
    assert$(end(events[2], bare("child")));

    assert$(start(events[3], QualifiedName{Some(exampleNs), "item"_sym}));
    assert$(end(events[4], QualifiedName{Some(exampleNs), "item"_sym}));

    assert$(end(events[5], bare("root")));

    return Ok();
}

test$("xml-events-namespace-scoping") {
    // A declaration applies to the element and its descendants only
    auto events = collect(
        "<a xmlns=\"urn:x\">"
        "<b xmlns=\"urn:y\"><c/></b>"
        "<d/>"
        "</a>"s
    );

    auto x = "urn:x"_sym;
    auto y = "urn:y"_sym;

    assertEq$(events.len(), 8uz);
    assert$(start(events[0], QualifiedName{Some(x), "a"_sym}));
    assert$(start(events[1], QualifiedName{Some(y), "b"_sym}));
    assert$(start(events[2], QualifiedName{Some(y), "c"_sym}));
    assert$(end(events[3], QualifiedName{Some(y), "c"_sym}));
    assert$(end(events[4], QualifiedName{Some(y), "b"_sym}));
    assert$(start(events[5], QualifiedName{Some(x), "d"_sym}));
    assert$(end(events[6], QualifiedName{Some(x), "d"_sym}));
    assert$(end(events[7], QualifiedName{Some(x), "a"_sym}));

    return Ok();
}

test$("xml-events-prefix-scoping") {
    // The same prefix can be rebound by a descendant
    auto events = collect(
        "<p:a xmlns:p=\"urn:x\">"
        "<p:b xmlns:p=\"urn:y\"/>"
        "<p:c/>"
        "</p:a>"s
    );

    auto x = "urn:x"_sym;
    auto y = "urn:y"_sym;

    assertEq$(events.len(), 6uz);
    assert$(start(events[0], QualifiedName{Some(x), "a"_sym}));
    assert$(start(events[1], QualifiedName{Some(y), "b"_sym}));
    assert$(end(events[2], QualifiedName{Some(y), "b"_sym}));
    assert$(start(events[3], QualifiedName{Some(x), "c"_sym}));
    assert$(end(events[4], QualifiedName{Some(x), "c"_sym}));
    assert$(end(events[5], QualifiedName{Some(x), "a"_sym}));

    return Ok();
}

test$("xml-events-namespace-declared-after-use") {
    // Declarations apply to the whole tag, no matter the attribute order
    auto events = collect("<p:a p:b=\"c\" xmlns:p=\"urn:p\"/>"s);

    auto p = "urn:p"_sym;

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], QualifiedName{Some(p), "a"_sym}));
    assert$(attr(events[0], QualifiedName{Some(p), "b"_sym}) == "c"s);
    assert$(end(events[1], QualifiedName{Some(p), "a"_sym}));

    return Ok();
}

test$("xml-events-xml-prefix-is-builtin") {
    auto events = collect("<a xml:lang=\"en\"/>"s);

    assertEq$(events.len(), 2uz);
    assert$(attr(events[0], QualifiedName{Some(NAMESPACE), "lang"_sym}) == "en"s);

    return Ok();
}

// MARK: Errors ----------------------------------------------------------------

// NOTE: An Error is always the last event, what comes before it is whatever
//       was well-formed up to that point.

test$("xml-events-error-text-only") {
    auto events = collect("just some text"s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-end-tag-only") {
    auto events = collect("</a>"s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-unclosed-element") {
    auto events = collect("<a><b>text"s);

    assertEq$(events.len(), 4uz);
    assert$(start(events[0], bare("a")));
    assert$(start(events[1], bare("b")));
    assert$(text(events[2], "text"s));
    assert$(rror(events[3]));

    return Ok();
}

test$("xml-events-error-unterminated-tag") {
    auto events = collect("<a b=\"c\""s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-mismatched-end-tag") {
    auto events = collect("<a><b></a></b>"s);

    assertEq$(events.len(), 3uz);
    assert$(start(events[0], bare("a")));
    assert$(start(events[1], bare("b")));
    assert$(rror(events[2]));

    return Ok();
}

test$("xml-events-error-mismatched-end-tag-namespace") {
    // Same local name, different namespace
    auto events = collect("<p:a xmlns:p=\"urn:p\"></a>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], QualifiedName{Some("urn:p"_sym), "a"_sym}));
    assert$(rror(events[1]));

    return Ok();
}

test$("xml-events-error-unknown-element-prefix") {
    auto events = collect("<p:a/>"s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-unknown-attribute-prefix") {
    auto events = collect("<a p:b=\"c\"/>"s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-attr-without-value") {
    auto events = collect("<a b/>"s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-attr-unquoted-value") {
    auto events = collect("<a b=c/>"s);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

test$("xml-events-error-unknown-entity") {
    auto events = collect("<a>&nope;</a>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(rror(events[1]));

    return Ok();
}

test$("xml-events-error-cdata-end-in-text") {
    auto events = collect("<a>x]]>y</a>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(rror(events[1]));

    return Ok();
}

test$("xml-events-error-unterminated-comment") {
    auto events = collect("<a><!-- oops</a>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(rror(events[1]));

    return Ok();
}

test$("xml-events-error-unterminated-cdata") {
    auto events = collect("<a><![CDATA[oops</a>"s);

    assertEq$(events.len(), 2uz);
    assert$(start(events[0], bare("a")));
    assert$(rror(events[1]));

    return Ok();
}

test$("xml-events-trailing-garbage-is-ignored") {
    // NOSPEC: Whatever follows the root that isn't a comment, a processing
    //         instruction or whitespace is left alone in the scanner.
    Io::SScan s{"<a/> <!--c--> garbage"s};
    Vec<Event> events;
    auto parser = Parser::parse(s);
    while (auto event = parser.next())
        events.pushBack(event.take());

    assertEq$(events.len(), 3uz);
    assert$(start(events[0], bare("a")));
    assert$(end(events[1], bare("a")));
    assert$(comment(events[2], "c"s));
    assertEq$(s.remStr(), "garbage"s);

    return Ok();
}

// MARK: Modes -----------------------------------------------------------------

test$("xml-events-element-mode") {
    auto events = collect("<html lang=\"en\">text</html>"s, Some(Xml::HTML_NAMESPACE), Mode::ELEMENT);

    assertEq$(events.len(), 3uz);
    assert$(start(events[0], html("html")));
    assert$(text(events[1], "text"s));
    assert$(end(events[2], html("html")));

    return Ok();
}

test$("xml-events-element-mode-stops-after-end-tag") {
    Io::SScan s{"<a><b/></a><!--c--><d/>"s};
    Vec<Event> events;
    auto parser = Parser::parse(s, NONE, Mode::ELEMENT);
    while (auto event = parser.next())
        events.pushBack(event.take());

    assertEq$(events.len(), 4uz);
    assert$(start(events[0], bare("a")));
    assert$(start(events[1], bare("b")));
    assert$(end(events[2], bare("b")));
    assert$(end(events[3], bare("a")));
    assertEq$(s.remStr(), "<!--c--><d/>"s);

    return Ok();
}

test$("xml-events-element-mode-has-no-prolog") {
    auto events = collect("<?xml version=\"1.0\"?><a/>"s, NONE, Mode::ELEMENT);

    assertEq$(events.len(), 1uz);
    assert$(rror(events[0]));

    return Ok();
}

// MARK: Streaming -------------------------------------------------------------

test$("xml-events-are-produced-lazily") {
    // The scanner only advances as events are pulled, so a consumer can
    // stop early without paying for the rest of the document.
    Io::SScan s{"<a><b/></a>"s};
    auto parser = Parser::parse(s);

    assertEq$(s.remStr(), "<a><b/></a>"s);

    auto first = parser.next();
    assert$(first != NONE);
    assert$(start(*first, bare("a")));
    assertEq$(s.remStr(), "<b/></a>"s);

    auto second = parser.next();
    assert$(second != NONE);
    assert$(start(*second, bare("b")));
    assertEq$(s.remStr(), "</a>"s);

    return Ok();
}

test$("xml-events-svg-icon") {
    // The kind of document the lightweight renderer is going to be fed
    auto events = collect(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 24 24\">\n"
        "  <defs><path id=\"p\" d=\"M0 0h24v24H0z\"/></defs>\n"
        "  <use xlink:href=\"#p\" fill=\"#f00\"/>\n"
        "</svg>\n"s
    );

    auto xlink = "http://www.w3.org/1999/xlink"_sym;

    usize starts = 0;
    usize ends = 0;
    for (auto& event : events) {
        assert$(not rror(event));
        if (event.is<StartElement>())
            starts++;
        else if (event.is<EndElement>())
            ends++;
    }
    assertEq$(starts, 4uz);
    assertEq$(ends, 4uz);

    assert$(events[0].is<XmlDecl>() != NONE);
    assert$(start(events[1], svg("svg")));
    assert$(attr(events[1], bare("viewBox")) == "0 0 24 24"s);

    // svg, "\n  ", defs, path
    assert$(start(events[3], svg("defs")));
    assert$(start(events[4], svg("path")));
    assert$(attr(events[4], bare("id")) == "p"s);
    assert$(attr(events[4], bare("d")) == "M0 0h24v24H0z"s);

    // /path, /defs, "\n  ", use
    assert$(start(events[8], svg("use")));
    assert$(attr(events[8], QualifiedName{Some(xlink), "href"_sym}) == "#p"s);
    assert$(attr(events[8], bare("fill")) == "#f00"s);

    return Ok();
}

} // namespace Karm::Xml::Tests
