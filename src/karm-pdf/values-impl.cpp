module;

#include <karm-core/macros.h>

module Karm.Pdf;

import Karm.Core;

namespace Karm::Pdf {

Res<> Name::write(Io::Writer& w) const {
    return Io::format(w, "/{}", str());
}

Res<> Array::write(Io::Writer& w) const {
    try$(Io::format(w, "["));
    for (usize i = 0; i < len(); ++i) {
        if (i > 0) {
            try$(Io::format(w, " "));
        }
        try$(buf()[i].write(w));
    }
    return Io::format(w, "]");
}

Res<> Dict::write(Io::Writer& w) const {
    try$(Io::format(w, "<<\n"));
    for (auto const& [k, v] : iterUnordered()) {
        try$(Io::format(w, "/{} ", k));
        try$(v.write(w));
        try$(Io::format(w, "\n"));
    }
    return Io::format(w, ">>");
}

Res<> Stream::write(Io::Writer& w) const {
    try$(dict.write(w));
    try$(Io::format(w, "stream\n"));
    try$(w.write(data));
    return Io::format(w, "\nendstream\n");
}

Res<> Value::write(Io::Writer& w) const {
    return visit(Visitor{
        [&](None) -> Res<> {
            return Io::format(w, "null");
        },
        [&](Ref const& ref) -> Res<> {
            return Io::format(w, "{} {} R", ref.num, ref.gen);
        },
        [&](bool b) -> Res<> {
            return Io::format(w, b ? "true" : "false");
        },
        [&](isize i) -> Res<> {
            return Io::format(w, "{}", i);
        },
        [&](usize i) -> Res<> {
            return Io::format(w, "{}", i);
        },
        [&](f64 f) -> Res<> {
            return Io::format(w, "{}", f);
        },
        [&](String const& s) -> Res<> {
            return Io::format(w, "({})", s);
        },
        [&](auto const& v) -> Res<> {
            return v.write(w);
        },
    });
}

Res<> File::write(Io::Writer& writer) const {
    Io::Count count{writer};

    try$(Io::format(count, "%{}\n", header));
    try$(Io::format(count, "%Powered By Karm PDF 🐢🏳️‍⚧️🦔\n"));

    XRef xref;

    for (auto const& [k, v] : body.iterUnordered()) {
        xref.add(try$(Io::tell(count)), k.gen);
        try$(Io::format(count, "{} {} obj\n", k.num, k.gen));
        try$(v.write(count));
        try$(Io::format(count, "\nendobj\n"));
    }

    auto startxref = try$(Io::tell(count));
    try$(Io::format(count, "xref\n"));
    try$(xref.write(count));

    try$(Io::format(count, "trailer\n"));
    try$(trailer.write(count));

    try$(Io::format(count, "\nstartxref\n"));
    try$(Io::format(count, "{}\n", startxref));
    return Io::format(count, "%%EOF");
}

Res<> XRef::write(Io::Writer& w) const {
    try$(Io::format(w, "1 {}\n", entries.len()));
    for (usize i = 0; i < entries.len(); ++i) {
        auto const& entry = entries[i];
        if (entry.used) {
            try$(Io::format(w, "{:010} {:05} n\n", entry.offset, entry.gen));
        }
    }
    return Ok();
}

} // namespace Karm::Pdf
