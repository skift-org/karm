module;

#include <karm/macros>

export module Karm.Svg:render;

import Karm.Core;
import Karm.Gfx;
import Karm.Xml;
import Karm.Logger;
import Karm.Math;

using namespace Karm::Literals;

namespace Karm::Svg {

export Res<Math::Vec2i> render(Gfx::Canvas& g, Io::SScan& s) {
    f64 width = 300;
    f64 height = 200;
    g.push();

    for (auto& event : Xml::Parser::parse(s)) {
        try$(event.visit(
            [&](Xml::StartElement& e) -> Res<> {
                if (e.name.name == "svg"s) {
                    for (auto& attr : e.attrs) {
                        if (attr.name.name == "width"s)
                            width = try$(Io::atof(attr.value.str()));
                        else if (attr.name.name == "height"s)
                            height = try$(Io::atof(attr.value.str()));
                    }
                } else if (e.name.name == "path"s) {
                    Str path;
                    for (auto& attr : e.attrs) {
                        if (attr.name.name == "d"s)
                            path = attr.value.str();
                    }
                    g.path(Math::Path::fromSvg(path));
                    g.strokeStyle({.fill = Gfx::BLACK, .width = 2, .align = Gfx::CENTER_ALIGN, .cap = Gfx::ROUND_CAP});
                    g.stroke();
                }
                return Ok();
            },
            [&](Xml::EndElement&) -> Res<> {
                return Ok();
            },
            [&](Xml::Text&) -> Res<> {
                return Ok();
            },
            [&](Xml::Comment&) -> Res<> {
                return Ok();
            },
            [&](Xml::Doctype&) -> Res<> {
                return Ok();
            },
            [&](Xml::XmlDecl&) -> Res<> {
                return Ok();
            },
            [](Error& e) -> Res<> {
                return e;
            }
        ));
    }

    g.pop();

    return Ok(Math::Vec2f{width, height}.cast<isize>());
}

export Res<Gfx::Snapshot> render(Str str) {
    Gfx::Snapshot::Recorder recorder{};
    Io::SScan s{str};
    recorder.clear(Gfx::WHITE);
    recorder.size(try$(render(recorder, s)));
    auto snapshot = recorder.finalize();
    return Ok(snapshot);
}

} // namespace Karm::Svg