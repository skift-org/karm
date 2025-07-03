#pragma once

#include <karm-base/set.h>
#include <karm-mime/url.h>
#include <karm-sys/mmap.h>

#include "base.h"
#include "font.h"

namespace Karm::Text {

struct FontQuery {
    Symbol family = "system"_sym;
    FontWeight weight = FontWeight::REGULAR;
    FontStretch stretch = FontStretch::NORMAL;
    FontStyle style = FontStyle::NORMAL;

    void repr(Io::Emit& e) const {
        e.ln("family: {#}", family);
        e.ln("weight: {}", weight);
        e.ln("stretch: {}", stretch);
        e.ln("style: {}", style);
    }
};

struct FontInfo {
    Mime::Url url;
    FontAttrs attrs;
    Rc<Fontface> face;
};

Symbol commonFamily(Symbol lhs, Symbol rhs);

struct FontBook {
    Vec<FontInfo> _faces;

    // FIXME: these value depend on the correct loading of the bundle
    Map<Symbol, Symbol> _genericFamily = {
        {"serif"_sym, "Noto Serif"_sym},
        {"sans-serif"_sym, "Noto Sans"_sym},
        {"monospace"_sym, "Fira Code"_sym},
        {"cursive"_sym, "Dancing Script"_sym},
        {"fantasy"_sym, "Excalibur"_sym},
        {"system"_sym, "Noto Serif"_sym},
        {"emoji"_sym, "Noto Emoji"_sym},
        {"math"_sym, "Noto Sans Math"_sym},
        {"fangsong"_sym, "Noto"_sym},
    };

    void add(FontInfo info) {
        _faces.pushBack(info);
    }

    Res<> load(Mime::Url const& url, Opt<FontAttrs> attrs = NONE);

    Res<> loadAll();

    Vec<Symbol> families() const;

    Symbol _resolveFamily(Symbol family) const;

    Opt<Rc<Fontface>> queryExact(FontQuery query) const;

    Opt<Rc<Fontface>> queryClosest(FontQuery query) const;

    Vec<Rc<Fontface>> queryFamily(Symbol family) const;
};

} // namespace Karm::Text
