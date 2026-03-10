export module Karm.Ref:uti;

import Karm.Core;

import :mime;
import :uid;

namespace Karm::Ref {

export struct Uti {
    enum struct Common {
        PUBLIC_INTENT,
        PUBLIC_OPEN,
        PUBLIC_PREVIEW,
        PUBLIC_MODIFY,
        PUBLIC_SHARE,
        PUBLIC_PRINT,
        PUBLIC_ITEM,
        PUBLIC_DIRECTORY,
        PUBLIC_DATA,
        PUBLIC_IMAGE,
        PUBLIC_BMP,
        PUBLIC_PNG,
        PUBLIC_TGA,
        PUBLIC_JPEG,
        PUBLIC_ICO,
        PUBLIC_GIF,
        PUBLIC_QOI,
        PUBLIC_WEBP,
        PUBLIC_FONT,
        PUBLIC_TTF,
        PUBLIC_OTF,
        PUBLIC_EOT,
        PUBLIC_WOFF,
        PUBLIC_WOFF2,
        PUBLIC_PDF,
        PUBLIC_TEXT,
        PUBLIC_HTML,
        PUBLIC_CSS,
        PUBLIC_JAVASCRIPT,
        PUBLIC_JSON,
        PUBLIC_TOML,
        PUBLIC_MARKDOWN,
        PUBLIC_XML,
        PUBLIC_SVG,
        PUBLIC_XHTML,
        PUBLIC_AV,
        PUBLIC_MOV,
        PUBLIC_MP4,
        PUBLIC_WEBM,
        PUBLIC_MP3,
        PUBLIC_WAV,
        PUBLIC_OGG,
        PUBLIC_FLAC,
        PUBLIC_AAC,
        PUBLIC_ARCHIVE,
        PUBLIC_ZIP,
        PUBLIC_TAR,
        PUBLIC_GZ,
        PUBLIC_BZIP2,
        PUBLIC_7Z,
        PUBLIC_RAR,
        PUBLIC_XZ,
    };

    using enum Common;

    struct Registration {
        Symbol name;
        String description = ""s;
        Vec<String> suffixes = {};
        Vec<Mime> mimes = {};
        Vec<Symbol> conformsTo = {};
    };

    struct Repository {
        Map<Symbol, Rc<Registration>> _registrations;
        Map<Common, Rc<Registration>> _common;

        Repository() {}

        void registerCommonUti();

        static Rc<Registration> createDynamic() {
            auto name = Symbol::from(Io::format("dynamic.{}", Uuid::v4().unwrap()));
            return makeRc<Registration>(name);
        }

        void addRegistration(Registration registration) {
            _registrations.put(registration.name, makeRc<Registration>(registration));
        }

        void addRegistration(Common common, Registration registration) {
            auto r = makeRc<Registration>(registration);
            _common.put(common, r);
            _registrations.put(r->name, r);
        }

        Rc<Registration> lookup(Common name) {
            return _common.lookup(name).unwrap();
        }

        Rc<Registration> lookupByName(Symbol name) {
            return _registrations.lookup(name).unwrapOrElse([&] {
                return makeRc<Registration>(name);
            });
        }

        Rc<Registration> lookupBySuffix(Str suffix) {
            for (auto& v : _registrations.iterValue()) {
                if (contains(v->suffixes, suffix))
                    return v;
            }

            auto dynamic = createDynamic();
            dynamic->suffixes = {suffix};
            dynamic->mimes = {"application/octet-stream"_mime};
            dynamic->conformsTo = {"public.data"_sym};
            return dynamic;
        }

        Rc<Registration> lookupByMime(Mime mime) {
            for (auto& v : _registrations.iterValue()) {
                if (contains(v->mimes, mime))
                    return v;
            }

            auto dynamic = createDynamic();
            dynamic->mimes = {mime};
            dynamic->conformsTo = {"public.data"_sym};
            return dynamic;
        }
    };

    static Repository& repository() {
        static Repository repository = [] {
            Repository repo;
            repo.registerCommonUti();
            return repo;
        }();
        return repository;
    }

    Rc<Registration> _registration;

    static Uti fromSuffix(Str suffix) {
        return repository().lookupBySuffix(suffix);
    }

    static Uti fromMime(Mime mime) {
        return repository().lookupByMime(mime);
    }

    static Uti fromUtiOrMime(Str str) {
        if (contains(str, "/"s))
            return fromMime(Mime{str});

        return Uti{Symbol::from(str)};
    }

    Uti(Symbol name) : _registration(repository().lookupByName(name)) {}

    Uti(Common common) : _registration(repository().lookup(common)) {}

    Uti(Rc<Registration> registration) : _registration(registration) {}

    Symbol name() const { return _registration->name; }

    Str description() const {
        return _registration->description;
    }

    auto const& suffixes() const {
        return _registration->suffixes;
    }

    auto const& mimeTypes() const {
        return _registration->mimes;
    }

    auto const& declaredConformances() const {
        return _registration->conformsTo;
    }

    Opt<Str> primarySuffix() const {
        if (not _registration->suffixes)
            return NONE;
        return _registration->suffixes[0];
    }

    Mime primaryMimeType() const {
        if (not _registration->mimes)
            return "application/octet-stream"_mime;
        return _registration->mimes[0];
    }

    bool conformsTo(Uti uti) const {
        if (name() == uti.name())
            return true;
        for (auto& c : _registration->conformsTo)
            if (Uti{c}.conformsTo(uti.name()))
                return true;
        return false;
    }

    u64 hash() const {
        return Karm::hash(name());
    }

    bool operator==(Uti const& other) const {
        return name() == other.name();
    }

    auto operator<=>(Uti const& other) const {
        return name() <=> other.name();
    }

    void repr(Io::Emit& e) const {
        e("{}", name());
    }
};

} // namespace Karm::Ref

export auto operator""_uti(char const* str, Karm::usize len) {
    return Karm::Ref::Uti(Karm::Symbol::from(Karm::Str{str, len}));
}
