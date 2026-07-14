export module Karm.Md:base;

import Karm.Core;
import Karm.Ref;

namespace Karm::Md {

export struct Node;

export struct Document {
    Serde::Value frontmatter;
    Vec<Node> children;

    void repr(Io::Emit& e) const {
        e("(document {})", children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& child : self.children)
            child.walk(visitor);
    }
};

using Paragraph = Vec<Node>;

export struct Heading {
    usize level;
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(h{} {})", level, children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& child : self.children)
            child.walk(visitor);
    }
};

export struct Code {
    String text;

    void repr(Io::Emit& e) const {
        e("(code {:#})", text);
    }

    template <typename F>
    void walk(this auto&&, F&&) {}
};

export struct InlineCode {
    String text;

    void repr(Io::Emit& e) const {
        e("(icode {:#})", text);
    }

    template <typename F>
    void walk(this auto&&, F&&) {}
};

export struct Html {
    String text;

    void repr(Io::Emit& e) const {
        e("(html {:#})", text);
    }

    template <typename F>
    void walk(this auto&&, F&&) {}
};

export struct Italic {
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(em {})", children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& child : self.children)
            child.walk(visitor);
    }
};

export struct Bold {
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(strong {})", children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& child : self.children)
            child.walk(visitor);
    }
};

export struct Link {
    Ref::Url href;
    Paragraph children;

    void repr(Io::Emit& e) const {
        e("(a {:#} {})", href, children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& child : self.children)
            child.walk(visitor);
    }
};

export struct Image {
    Ref::Url src;
    String alt;

    void repr(Io::Emit& e) const {
        e("(img {:#} {:#})", src, alt);
    }

    template <typename F>
    void walk(this auto&&, F&&) {}
};

export struct ListItem {
    Vec<Node> children;

    void repr(Io::Emit& e) const {
        e("(li {})", children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& child : self.children)
            child.walk(visitor);
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

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& item : self.items)
            item.walk(visitor);
    }
};

export struct Quote {
    Vec<Paragraph> children;

    void repr(Io::Emit& e) const {
        e("(blockquote {})", children);
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        for (auto&& paragraph : self.children) {
            for (auto&& child : paragraph)
                child.walk(visitor);
        }
    }
};

export struct Hr {
    void repr(Io::Emit& e) const {
        e("(hr)");
    }

    template <typename F>
    void walk(this auto&&, F&&) {}
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
                e("{:#}", n);
            },
            [&](auto const& n) {
                e("{}", n);
            }
        );
    }

    template <typename Self, typename F>
    void walk(this Self&& self, F&& visitor) {
        self.visit([&](auto&& n) {
            visitor(n);

            using T = Meta::Decay<decltype(n)>;

            if constexpr (Meta::Same<T, Paragraph>) {
                for (auto&& child : n)
                    child.walk(visitor);
            } else if constexpr (Meta::Same<T, String>) {
            } else {
                n.walk(visitor);
            }
        });
    }
};

export template <typename... Ts>
struct Visitor : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Visitor(Ts...) -> Visitor<Ts...>;

} // namespace Karm::Md
