export module Karm.Image:codec;

import Karm.Core;
import Karm.Ref;
import Karm.Gfx.Pixels;

namespace Karm::Image {

export struct Saver {
    /// Image format to save as
    Ref::Uti format = Ref::Uti::PUBLIC_BMP;

    /// Quality for lossy formats (JPEG)
    f64 quality = DEFAULT_QUALITY;
    static constexpr f64 MIN_QUALITY = 0.0;
    static constexpr f64 DEFAULT_QUALITY = 0.75;
    static constexpr f64 MAX_QUALITY = 1.0;

    f64 density = 1; /// For saving scenes as raster images
};

export struct Codec;

static Codec* _first = nullptr;

export struct Codec : Meta::Pinned {
    enum struct Sniffing {
        MAGIC,
        // NOTE: Heuristic sniffing can mistake other formats, so these
        //       codecs are tried after all the ones with a magic number.
        HEURISTIC,
    };

    using enum Sniffing;

    enum struct Source {
        // NOTE: Codecs backed by a system library are more complete and
        //       battle tested, so they take precedence over our own.
        SYSTEM,
        BUILTIN,
    };

    using enum Source;

    using Sniff = bool (*)(Bytes);
    using Decode = Res<Rc<Gfx::Image>> (*)(Bytes);
    using Encode = Res<> (*)(Gfx::Pixels, Io::BEmit&, Saver const&);

    Str name;
    // NOTE: Stored as Common since constructing an Uti queries the UTI
    //       repository, which might not be initialized yet.
    Ref::Uti::Common uti;
    Sniffing sniffing;
    Source source;
    Sniff sniff;
    Decode decode;
    Encode encode;
    Codec* next = nullptr;

    static Codec* first() {
        return _first;
    }

    Codec(Str name, Ref::Uti::Common uti, Sniffing sniffing, Source source, Sniff sniff, Decode decode, Encode encode = nullptr)
        : name(name), uti(uti), sniffing(sniffing), source(source), sniff(sniff), decode(decode), encode(encode) {
        Codec** slot = &_first;
        auto before = [&](Codec const& other) {
            if (other.sniffing != sniffing)
                return other.sniffing < sniffing;
            return other.source <= source;
        };
        while (*slot and before(**slot))
            slot = &(*slot)->next;
        next = *slot;
        *slot = this;
    }
};

} // namespace Karm::Image
