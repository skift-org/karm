export module Karm.Print:page;

import Karm.Core;
import Karm.Scene;
import :paper;
import :printer;

namespace Karm::Print {

export struct Page {
    Math::Vec2f _size;
    Rc<Scene::Node> _content;

    Page(Math::Vec2f paper, Opt<Rc<Scene::Node>> content = NONE)
        : _size(paper), _content(content ? content.take() : makeRc<Scene::Stack>()) {}

    Rc<Scene::Node> content() const {
        return makeRc<Scene::Viewbox>(_content, _size);
    }

    void print(Printer& doc, Scene::PaintOptions o = {.showBackgroundGraphics = false}) {
        auto& canvas = doc.beginPage(_size);
        content()->paint(canvas, _size, o);
    }

    void repr(Io::Emit& e) const {
        e("(page size:{} root:{})", _size, content());
    }
};

} // namespace Karm::Print
