#pragma once

import Karm.Core;

#include <karm-gfx/font.h>
#include <karm-mime/url.h>

namespace Karm::Font {

struct Query {
    Gfx::FontWeight weight = Gfx::FontWeight::REGULAR;
    Gfx::FontStretch stretch = Gfx::FontStretch::NORMAL;
    Gfx::FontStyle style = Gfx::FontStyle::NORMAL;

    void repr(Io::Emit& e) const {
        e.ln("weight: {}", weight);
        e.ln("stretch: {}", stretch);
        e.ln("style: {}", style);
    }
};

struct Record {
    Mime::Url url;
    Gfx::FontAttrs attrs;
    Rc<Gfx::Fontface> face;
    Gfx::FontAdjust adjust = {};
};

Symbol commonFamily(Symbol lhs, Symbol rhs);

struct Database {
    Vec<Record> _records;

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

    void add(Record record) {
        _records.pushBack(record);
    }

    Res<> load(Mime::Url const& url, Opt<Gfx::FontAttrs> attrs = NONE);

    Res<> loadAll();

    Vec<Symbol> families() const;

    Symbol _resolveFamily(Symbol family) const;

    Opt<Rc<Gfx::Fontface>> queryExact(Symbol family, Query query = {}) const;

    Opt<Rc<Gfx::Fontface>> queryClosest(Symbol family, Query query = {}) const;

    Vec<Rc<Gfx::Fontface>> queryFamily(Symbol family) const;
};

Database& globalDatabase();

} // namespace Karm::Font
