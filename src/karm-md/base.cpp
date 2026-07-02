export module Karm.Md:base;

import Karm.Core;

namespace Karm::Md {

export struct Node;

export struct Document {
    Vec<Node> children;

    void repr(Io::Emit& e) const {
        e("(document {})", children);
    }
};

using Paragraph = Vec<Node>;

export struct Heading {
    usize level;
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(h{} {})", level, children);
    }
};

export struct Code {
    String text;

    void repr(Io::Emit& e) const {
        e("(code {#})", text);
    }
};

export struct InlineCode {
    String text;

    void repr(Io::Emit& e) const {
        e("(icode {#})", text);
    }
};

export struct Html {
    String text;

    void repr(Io::Emit& e) const {
        e("(html {#})", text);
    }
};

export struct Italic {
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(em {})", children);
    }
};

export struct Bold {
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(strong {})", children);
    }
};

export struct Link {
    String href;
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(a {#} {})", href, children);
    }
};

export struct Image {
    String src;
    String alt;

    void repr(Io::Emit& e) const {
        e("(img {#} {#})", src, alt);
    }
};

export struct ListItem {
    Vec<Node> children;

    void repr(Io::Emit& e) const {
        e("(li {})", children);
    }
};

export struct List {
    bool ordered;
    Vec<ListItem> items;

    void repr(Io::Emit& e) const {
        if (ordered)
            e("(ol {})", items);
        else
            e("(ul {})", items);
    }
};

export struct Quote {
    Vec<Paragraph> children;

    void repr(Io::Emit& e) const {
        e("(blockquote {})", children);
    }
};

export struct Hr {
    void repr(Io::Emit& e) const {
        e("(hr)");
    }
};

using _Node = Union<
    Paragraph,
    Heading,
    Code,
    InlineCode,
    Html,
    Italic,
    Bold,
    Link,
    Image,
    List,
    Quote,
    Hr,
    String>;

export struct Node : _Node {
    using _Node::_Node;

    void repr(Io::Emit& e) const {
        visit(
            [&](String const& n) {
                e("{#}", n);
            },
            [&](auto const& n) {
                e("{}", n);
            }
        );
    }
};

} // namespace Karm::Md
