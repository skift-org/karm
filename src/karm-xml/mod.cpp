module;

#include <karm/macros>

export module Karm.Xml;

import Karm.Core;

using namespace Karm::Literals;
using namespace Karm::Re::Literals;

namespace Karm::Xml {

export Symbol NAMESPACE = "http://www.w3.org/XML/1998/namespace"_sym;
export Symbol XMLNS_NAMESPACE = "http://www.w3.org/2000/xmlns/"_sym;
export Symbol HTML_NAMESPACE = "http://www.w3.org/1999/xhtml"_sym;
export Symbol SVG_NAMESPACE = "http://www.w3.org/2000/svg"_sym;

// MARK: Events ----------------------------------------------------------------

export struct QualifiedName {
    Opt<Symbol> ns;
    Symbol name;

    bool operator==(QualifiedName const& other) const = default;

    void repr(Io::Emit& e) const {
        e("(qualified-name ns:{} name:{})", ns, name);
    }
};

export struct Attr {
    QualifiedName name;
    String value;

    void repr(Io::Emit& e) const {
        e("(attr name:{} value:{})", name, value);
    }
};

/// <?xml version="1.0" encoding="UTF-8" standalone="no"?>
export struct XmlDecl {
    String version;
    Opt<String> encoding;
    Opt<String> standalone;

    void repr(Io::Emit& e) const {
        e("(xml-decl name:{} publicId:{} systemId:{})", version, encoding, standalone);
    }
};

/// <!DOCTYPE name PUBLIC "publicId" "systemId">
export struct Doctype {
    Symbol name = ""_sym;
    String publicId = ""s;
    String systemId = ""s;

    void repr(Io::Emit& e) const {
        e("(doctype name:{} publicId:{} systemId:{})", name, publicId, systemId);
    }
};

/// Attributes are in document order with their names resolved and their
/// values unescaped, xmlns declarations are included.
export struct StartElement {
    QualifiedName name;
    Vec<Attr> attrs;

    void repr(Io::Emit& e) const {
        e("(start-element name:{} attrs:{})", name, attrs);
    }
};

export struct EndElement {
    QualifiedName name;

    void repr(Io::Emit& e) const {
        e("(end-element name:{})", name);
    }
};

export struct Text {
    String data;

    void repr(Io::Emit& e) const {
        e("(text data:{})", data);
    }
};

export struct Comment {
    String data;

    void repr(Io::Emit& e) const {
        e("(comment data:{})", data);
    }
};

export using Event = Union<
    XmlDecl,
    Doctype,
    StartElement,
    EndElement,
    Text,
    Comment,
    Error>;

export enum struct Mode {
    DOCUMENT, //< prolog element Misc*
    ELEMENT,  //< a single element, no prolog, stops right after its end tag
};

// MARK: Namespaces ------------------------------------------------------------

struct UnresolvedQualifiedName {
    Opt<Symbol> prefix;
    Symbol localName;
};

// Namespace bindings are scoped per element, and the default namespace
// applies to element names but not to unprefixed attributes.
// https://www.w3.org/TR/xml-names/#scoping-defaulting
// https://www.w3.org/TR/xml-names/#ns-qualnames
struct NamespaceContext {
    Opt<Symbol> default_;
    Cow<Map<Symbol, Symbol>> prefixes;

    static NamespaceContext make(Opt<Symbol> default_) {
        NamespaceContext context{};
        context.default_ = default_;
        context.prefixes.cow().put("xml"_sym, NAMESPACE);
        context.prefixes.cow().put("xmlns"_sym, XMLNS_NAMESPACE);
        return context;
    }

    Res<QualifiedName> resolveElementName(UnresolvedQualifiedName const& parsedName) const {
        if (not parsedName.prefix)
            return Ok(QualifiedName{default_, parsedName.localName});

        auto ns = prefixes->lookup(parsedName.prefix.expect());
        if (not ns)
            return Error::invalidData("unknown namespace prefix");

        return Ok(QualifiedName{Some(*ns), parsedName.localName});
    }

    Res<QualifiedName> resolveAttributeName(UnresolvedQualifiedName const& parsedName) const {
        if (not parsedName.prefix)
            return Ok(QualifiedName{NONE, parsedName.localName});

        auto ns = prefixes->lookup(parsedName.prefix.expect());
        if (not ns)
            return Error::invalidData("unknown namespace prefix");

        return Ok(QualifiedName{Some(*ns), parsedName.localName});
    }

    void declarePrefix(Symbol prefix, Opt<Symbol> ns) {
        if (prefix == "xml"_sym or prefix == "xmlns"_sym)
            return;

        if (ns)
            prefixes.cow().put(prefix, *ns);
        else
            (void)prefixes.cow().remove(prefix);
    }
};

// MARK: Parser ----------------------------------------------------------------

export struct Parser {
    // 2 MARK: Documents
    // https://www.w3.org/TR/xml/#sec-documents

    static Yield<Event> parse(Io::SScan& s, Opt<Symbol> ns = NONE, Mode mode = Mode::DOCUMENT) {
        if (mode == Mode::DOCUMENT) {
            // prolog ::= XMLDecl? Misc* (doctypedecl Misc*)?

            if (s.ahead(RE_XML_DECL_START)) {
                auto decl = co_tryy$(_parseXmlDecl(s));
                co_yield std::move(decl);
            }

            while (true) {
                auto comment = co_tryy$(_parseMisc(s));
                if (not comment)
                    break;
                co_yield comment.take();
            }

            if (s.ahead(RE_DOCTYPE_START)) {
                auto doctype = co_tryy$(_parseDoctype(s));
                co_yield std::move(doctype);

                while (true) {
                    auto comment = co_tryy$(_parseMisc(s));
                    if (not comment)
                        break;
                    co_yield comment.take();
                }
            }
        }

        // element ::= EmptyElemTag | STag content ETag
        // content ::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*

        if (not s.ahead(RE_TAG_START)) {
            co_yield Error::invalidData("expected element");
            co_return;
        }

        struct Open {
            QualifiedName name;
            NamespaceContext context;
        };

        Vec<Open> stack;
        auto rootContext = NamespaceContext::make(ns);

        do {
            if (s.ended()) {
                co_yield Error::invalidData("unexpected end of input");
                co_return;
            }

            if (s.ahead(RE_END_TAG_START)) {
                auto& open = last(stack);
                co_tryy$(_parseEndTag(s, open.context, open.name));
                co_yield EndElement{stack.popBack().name};
            } else if (s.ahead(RE_COMMENT_START)) {
                auto comment = co_tryy$(_parseComment(s));
                co_yield std::move(comment);
            } else if (s.ahead(RE_PI_START)) {
                // Processing instructions are ignored
                co_tryy$(_parsePi(s));
            } else if (s.ahead(RE_CDATA_START) or not s.ahead('<')) {
                auto text = co_tryy$(_parseText(s));
                if (text)
                    co_yield Text{std::move(text)};
            } else {
                auto tag = co_tryy$(_parseTag(s, stack.len() ? last(stack).context : rootContext));
                co_yield StartElement{tag.name, std::move(tag.attrs)};
                if (tag.empty)
                    co_yield EndElement{tag.name};
                else
                    stack.pushBack(Open{tag.name, tag.context});
            }
        } while (stack.len());

        if (mode == Mode::DOCUMENT) {
            // Misc*
            while (true) {
                auto misc = _parseMisc(s);
                if (not misc)
                    break;
                auto comment = misc.take();
                if (not comment)
                    break;
                co_yield comment.take();
            }
        }
    }

    // 2.2 MARK: Characters
    // https://www.w3.org/TR/xml/#charsets

    static constexpr auto RE_CHAR =
        '\x09'_re | '\x0A'_re | '\x0D'_re | Re::range(0x20, 0xD7FF) |
        Re::range(0xE000, 0xFFFD) | Re::range(0x10000, 0x10FFFF);

    // 2.3 MARK: Common Syntactic Constructs
    // https://www.w3.org/TR/xml/#sec-common-syn

    static constexpr auto RE_S =
        Re::single(' ', '\t', '\r', '\n');

    static bool _isNameStartChar(Rune r) {
        Array RANGES = {
            Pair<Rune, Rune>{':', ':'},
            Pair<Rune, Rune>{'A', 'Z'},
            Pair<Rune, Rune>{'_', '_'},
            Pair<Rune, Rune>{'a', 'z'},
            Pair<Rune, Rune>{0xC0, 0xD6},
            Pair<Rune, Rune>{0xD8, 0xF6},
            Pair<Rune, Rune>{0xF8, 0x2FF},
            Pair<Rune, Rune>{0x370, 0x37D},
            Pair<Rune, Rune>{0x37F, 0x1FFF},
            Pair<Rune, Rune>{0x200C, 0x200D},
            Pair<Rune, Rune>{0x2070, 0x218F},
            Pair<Rune, Rune>{0x2C00, 0x2FEF},
            Pair<Rune, Rune>{0x3001, 0xD7FF},
            Pair<Rune, Rune>{0xF900, 0xFDCF},
            Pair<Rune, Rune>{0xFDF0, 0xFFFD},
            Pair<Rune, Rune>{0x10000, 0xEFFFF},
        };

        for (auto range : RANGES)
            if (r >= range.v0 and r <= range.v1)
                return true;

        return false;
    }

    static constexpr auto RE_NAME_START_CHAR = Re::ctype(_isNameStartChar);

    static bool _isNameChar(Rune r) {
        Array RANGES = {
            Pair<Rune, Rune>{'-', '-'},
            Pair<Rune, Rune>{'.', '.'},
            Pair<Rune, Rune>{'0', '9'},
            Pair<Rune, Rune>{0xB7, 0xB7},
            Pair<Rune, Rune>{0x0300, 0x036F},
            Pair<Rune, Rune>{0x203F, 0x2040},
        };

        for (auto range : RANGES)
            if (r >= range.v0 and r <= range.v1)
                return true;

        return false;
    }

    static constexpr auto RE_NAME_CHAR = RE_NAME_START_CHAR | Re::ctype(_isNameChar);

    static constexpr auto RE_NAME = RE_NAME_START_CHAR & Re::zeroOrMore(RE_NAME_CHAR);

    static Res<UnresolvedQualifiedName> _parseQualifiedName(Str name) {
        Opt<usize> separator = NONE;

        for (usize i = 0; i < name.len(); i++) {
            if (name[i] != ':')
                continue;

            if (separator)
                return Error::invalidData("expected a single namespace separator");

            separator = Some(i);
        }

        if (not separator)
            return Ok(UnresolvedQualifiedName{NONE, Symbol::from(name)});

        auto separatorIndex = separator.expect();
        if (separatorIndex == 0 or separatorIndex + 1 == name.len())
            return Error::invalidData("expected namespace prefix and local name");

        return Ok(UnresolvedQualifiedName{
            Some(Symbol::from(sub(name, 0, separatorIndex))),
            Symbol::from(sub(name, separatorIndex + 1, name.len())),
        });
    }

    static Res<UnresolvedQualifiedName> _parseQualifiedName(Io::SScan& s) {
        return _parseQualifiedName(try$(_parseName(s)));
    }

    static void _parseS(Io::SScan& s) {
        // S ::= (#x20 | #x9 | #xD | #xA)+

        s.eat(RE_S);
    }

    static Res<> _parseEq(Io::SScan& s) {
        // Eq ::= S? '=' S?

        _parseS(s);

        if (not s.skip('='))
            return Error::invalidData("expected '='");

        _parseS(s);

        return Ok();
    }

    static Res<Str> _parseName(Io::SScan& s) {
        // Name ::= NameStartChar (NameChar)*

        auto name = s.token(RE_NAME);
        if (isEmpty(name))
            return Error::invalidData("expected name");
        return Ok(name);
    }

    // 2.4 MARK: Character Data and Markup
    // https://www.w3.org/TR/xml/#syntax

    /// Parse a whole run of CharData, CDSect and Reference into a single
    /// string, stopping at the next piece of markup.
    static Res<String> _parseText(Io::SScan& s) {
        // CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*)

        StringBuilder sb;

        while (not s.ended()) {
            if (s.ahead(RE_CDATA_START)) {
                try$(_parseCDSect(s, sb));
            } else if (s.ahead('<')) {
                break;
            } else if (s.ahead('&')) {
                sb.append(try$(_parseReference(s)));
            } else if (s.ahead(RE_CDATA_END)) {
                return Error::invalidData("unexpected ']]>' in character data");
            } else {
                sb.append(s.next());
            }
        }

        return Ok(sb.take());
    }

    // 2.5 MARK: Comments
    // https://www.w3.org/TR/xml/#sec-comments

    static constexpr auto RE_COMMENT_START = "<!--"_re;
    static constexpr auto RE_COMMENT_END = "-->"_re;

    static Res<Comment> _parseComment(Io::SScan& s) {
        // 	Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'

        if (not s.skip(RE_COMMENT_START))
            return Error::invalidData("expected '<!--'");

        StringBuilder sb;
        while (not s.ahead(RE_COMMENT_END) and not s.ended()) {
            auto chrs = s.token(RE_CHAR);
            if (isEmpty(chrs))
                return Error::invalidData("expected character data");
            sb.append(chrs);
        }

        if (not s.skip(RE_COMMENT_END))
            return Error::invalidData("expected '-->'");

        return Ok(Comment{sb.take()});
    }

    // 2.6 MARK: Processing Instructions
    // https://www.w3.org/TR/xml/#sec-pi

    static constexpr auto RE_PI_START = "<?"_re;
    static constexpr auto RE_PI_END = "?>"_re;

    static Res<> _parsePi(Io::SScan& s) {
        // PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>

        if (not s.skip(RE_PI_START))
            return Error::invalidData("expected '<?'");
        try$(_parsePiTarget(s));

        while (not s.ahead(RE_PI_END) and not s.ended()) {
            auto chrs = s.token(RE_CHAR);
            if (isEmpty(chrs))
                return Error::invalidData("expected character data");
        }

        if (not s.skip(RE_PI_END))
            return Error::invalidData("expected '?>'");

        return Ok();
    }

    static Res<> _parsePiTarget(Io::SScan& s) {
        // PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

        auto name = try$(_parseName(s));
        if (eqCi(name, "xml"s))
            return Error::invalidData("expected name to not be 'xml'");
        return Ok();
    }

    // 2.7 MARK: CDATA Sections
    // https://www.w3.org/TR/xml/#sec-cdata-sect

    static constexpr auto RE_CDATA_START = "<![CDATA["_re;
    static constexpr auto RE_CDATA_END = "]]>"_re;

    static Res<> _parseCDSect(Io::SScan& s, StringBuilder& sb) {
        // CDStart ::= '<![CDATA['
        // CData ::= (Char* - (Char* ']]>' Char*))
        // CDEnd ::= ']]>'

        if (not s.skip(RE_CDATA_START))
            return Error::invalidData("expected '<![CDATA['");

        while (not s.ahead(RE_CDATA_END) and not s.ended())
            sb.append(s.next());

        if (not s.skip(RE_CDATA_END))
            return Error::invalidData("expected ']]>'");

        return Ok();
    }

    // 2.8 MARK: Prolog and Document Type Declaration
    // https://www.w3.org/TR/xml/#sec-prolog-dtd

    static constexpr auto RE_XML_DECL_START = "<?xml"_re;
    static constexpr auto RE_XML_DECL_VERSION = Re::chain("1."_re, Re::oneOrMore(Re::digit()));
    static constexpr auto RE_XML_DECL_ENCODING = Re::chain(Re::alpha(), Re::zeroOrMore(Re::word() | "."_re | "-"_re));
    static constexpr auto RE_XML_DECL_STANDALONE = "yes"_re | "no"_re;

    static Res<Str> _parseXmlDeclAttr(Io::SScan& s, Re::Expr auto name, Re::Expr auto value) {
        auto rollback = s.rollbackPoint();

        _parseS(s);

        if (not s.skip(name))
            return Error::invalidData("expected attribute");

        try$(_parseEq(s));

        auto quote = s.next();
        if (quote != '"' and quote != '\'')
            return Error::invalidData("expected '\"' or '''");

        auto result = s.token(value);
        if (not result)
            return Error::invalidData("expected attribute value");

        if (s.peek() != quote)
            return Error::invalidData("expected closing quote");
        s.next();

        rollback.disarm();
        return Ok(result);
    }

    static Res<XmlDecl> _parseXmlDecl(Io::SScan& s) {
        // XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'

        if (not s.skip(RE_XML_DECL_START))
            return Error::invalidData("expected '<?xml'");

        XmlDecl decl;

        // versionInfo ::= S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')
        decl.version = try$(_parseXmlDeclAttr(s, "version"_re, RE_XML_DECL_VERSION));

        // EncodingDecl ::= S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" )
        auto encoding = _parseXmlDeclAttr(s, "encoding"_re, RE_XML_DECL_ENCODING);
        if (encoding.has())
            decl.encoding = Some(String{encoding.expect()});

        // SDDecl ::= S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'))
        auto standalone = _parseXmlDeclAttr(s, "standalone"_re, RE_XML_DECL_STANDALONE);
        if (standalone.has())
            decl.standalone = Some(String{standalone.expect()});

        _parseS(s);

        if (not s.skip(RE_PI_END))
            return Error::invalidData("expected '?>'");

        return Ok(std::move(decl));
    }

    /// Consume whitespaces and processing instructions up to the next
    /// comment, returns NONE once there's no more Misc to parse.
    static Res<Opt<Comment>> _parseMisc(Io::SScan& s) {
        // Misc ::= Comment | PI | S

        while (true) {
            if (s.ahead(RE_COMMENT_START))
                return Ok(Opt<Comment>{Some(try$(_parseComment(s)))});

            if (s.ahead(RE_PI_START)) {
                try$(_parsePi(s));
                continue;
            }

            if (not s.eat(RE_S))
                return Ok(Opt<Comment>{NONE});
        }
    }

    static constexpr auto RE_DOCTYPE_START = "<!DOCTYPE"_re;

    static Res<Doctype> _parseDoctype(Io::SScan& s) {
        // doctypedecl ::= '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'

        if (not s.skip(RE_DOCTYPE_START))
            return Error::invalidData("expected '<!DOCTYPE'");

        Doctype doctype;

        _parseS(s);

        doctype.name = Symbol::from(try$(_parseName(s)));

        _parseS(s);
        (void)_parseExternalId(s, doctype);

        _parseS(s);
        if (not s.skip('>'))
            return Error::invalidData("expected '>'");

        return Ok(std::move(doctype));
    }

    // 3.1 MARK: Start-Tags, End-Tags, and Empty-Element Tags
    // https://www.w3.org/TR/xml/#sec-starttags

    static constexpr auto RE_TAG_START = '<'_re & RE_NAME_START_CHAR;
    static constexpr auto RE_END_TAG_START = "</"_re;

    struct _Tag {
        QualifiedName name;
        Vec<Attr> attrs;
        NamespaceContext context; //< the context for the children of this element
        bool empty;
    };

    static Res<_Tag> _parseTag(Io::SScan& s, NamespaceContext const& parentContext) {
        // STag         ::= '<' Name (S Attribute)* S? '>'
        // EmptyElemTag ::= '<' Name (S Attribute)* S? '/>'

        struct RawAttr {
            UnresolvedQualifiedName name;
            String value;
        };

        if (not s.skip('<'))
            return Error::invalidData("expected '<'");

        auto parsedName = try$(_parseQualifiedName(s));
        _parseS(s);

        // NOTE: Namespace declarations apply to the element they appear on
        //       and to all of its attributes no matter the order, so names
        //       can only be resolved once the whole tag has been read.
        // https://www.w3.org/TR/xml-names/#scoping-defaulting
        Vec<RawAttr> rawAttrs;
        auto context = parentContext;
        bool empty = false;

        while (true) {
            if (s.skip("/>"_re)) {
                empty = true;
                break;
            }

            if (s.skip('>'))
                break;

            if (s.ended())
                return Error::invalidData("expected '>' or '/>'");

            // Attribute ::= Name Eq AttValue
            auto attrName = try$(_parseQualifiedName(s));
            try$(_parseEq(s));
            auto value = try$(_parseAttValue(s));
            _parseS(s);

            if (not attrName.prefix and attrName.localName == "xmlns"_sym)
                context.default_ = Some(Symbol::from(value));
            else if (attrName.prefix == "xmlns"_sym)
                context.declarePrefix(attrName.localName, Some(Symbol::from(value)));

            rawAttrs.pushBack(RawAttr{attrName, std::move(value)});
        }

        _Tag tag{
            try$(context.resolveElementName(parsedName)),
            {},
            context,
            empty,
        };

        for (auto& attr : rawAttrs) {
            tag.attrs.pushBack(Attr{
                try$(context.resolveAttributeName(attr.name)),
                std::move(attr.value),
            });
        }

        return Ok(std::move(tag));
    }

    static Res<String> _parseAttValue(Io::SScan& s) {
        // AttValue ::= '"' ([^<&"] | Reference)* '"'
        //              |  "'" ([^<&'] | Reference)* "'"

        StringBuilder sb;

        auto quote = s.next();
        if (quote != '"' and quote != '\'')
            return Error::invalidData("expected '\"' or '''");

        while (s.peek() != quote and not s.ended()) {
            if (auto r = _parseReference(s))
                sb.append(r.expect());
            else
                sb.append(s.next());
        }

        if (s.peek() != quote)
            return Error::invalidData("expected closing quote");

        s.next();

        return Ok(sb.take());
    }

    static Res<> _parseEndTag(Io::SScan& s, NamespaceContext const& context, QualifiedName const& expected) {
        // '</' Name S? '>'

        if (not s.skip(RE_END_TAG_START))
            return Error::invalidData("expected '</'");

        if (try$(context.resolveElementName(try$(_parseQualifiedName(s)))) != expected)
            return Error::invalidData("expected end tag name to match start tag name");

        _parseS(s);

        if (not s.skip('>'))
            return Error::invalidData("expected '>'");

        return Ok();
    }

    // 4.1 MARK: Character and Entity References
    // https://www.w3.org/TR/xml/#NT-CharRef

    static Res<Rune> _parseCharRef(Io::SScan& s) {
        // CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'

        auto rollback = s.rollbackPoint();

        if (not s.skip("&#"_re))
            return Error::invalidData("expected '&#'");

        Rune r = REPLACEMENT;

        if (s.skip('x')) {
            auto val = Io::atoi(s, {.base = 16});
            if (not val)
                return Error::invalidData("expected hexadecimal number");
            r = val.expect();
        } else {
            auto val = Io::atoi(s, {.base = 10});
            if (not val)
                return Error::invalidData("expected decimal number");
            r = val.expect();
        }

        if (not s.skip(';'))
            return Error::invalidData("expected ';'");

        rollback.disarm();
        return Ok(r);
    }

    static Res<Rune> _parseEntityRef(Io::SScan& s) {
        // EntityRef ::= '&' Name ';'

        auto rollback = s.rollbackPoint();

        if (not s.skip('&'))
            return Error::invalidData("expected '&'");

        auto name = try$(_parseName(s));

        if (not s.skip(';'))
            return Error::invalidData("expected ';'");

        rollback.disarm();
        if (name == "lt")
            return Ok('<');
        else if (name == "gt")
            return Ok('>');
        else if (name == "amp")
            return Ok('&');
        else if (name == "apos")
            return Ok('\'');
        else if (name == "quot")
            return Ok('"');

        // NOTE: (workaround) we don't support XML fully so nbsp can't be loaded from a DOCTYPE as its supposed to
        else if (name == "nbsp")
            return Ok(160);

        rollback.arm();
        return Error::invalidData("unknown entity reference");
    }

    static Res<Rune> _parseReference(Io::SScan& s) {
        // Reference ::= EntityRef | CharRef

        if (auto r = _parseCharRef(s))
            return r;
        else if (auto r = _parseEntityRef(s))
            return r;
        else
            return Error::invalidData("expected reference");
    }

    // 4.2 MARK: Entity Declarations
    // https://www.w3.org/TR/xml/#sec-entity-decl

    static Res<> _parseExternalId(Io::SScan& s, Doctype& doctype) {
        // ExternalID ::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral

        auto rollback = s.rollbackPoint();

        if (s.skip("SYSTEM"_re)) {
            _parseS(s);
            // NOSPEC: We are parsing the system literal as att value
            doctype.systemId = try$(_parseAttValue(s));
            rollback.disarm();
            return Ok();
        } else if (s.skip("PUBLIC"_re)) {
            // NOSPEC: We are parsing the public and system literals as att values
            _parseS(s);
            doctype.publicId = try$(_parseAttValue(s));
            _parseS(s);
            doctype.systemId = try$(_parseAttValue(s));
            rollback.disarm();
            return Ok();
        } else {
            return Error::invalidData("expected 'SYSTEM' or 'PUBLIC'");
        }
    }
};

} // namespace Karm::Xml
