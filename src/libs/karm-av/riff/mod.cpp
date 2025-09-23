export module Karm.Av:riff;

import Karm.Core;

namespace Karm::Av::Riff {

// https://www.aelius.com/njh/wavemetatools/doc/riffmci.pdf

export using Id = Array<char, 4>;

export Id RIFF = {'R', 'I', 'F', 'F'};

struct ChunkHeader {
    Id id;
    u32le size;
};

export struct Chunk {
    Id id;
    Bytes data;

    static Res<Chunk> read(Bytes bytes) {
        Io::BScan s{bytes};
        return read(s);
    }

    static Res<Chunk> read(Io::BScan& s) {
        if (s.rem() <= sizeof(ChunkHeader))
            return Error::invalidData("not a riff chunk");

        auto header = s.next<ChunkHeader>();
        if (header.size > s.rem())
            return Error::invalidData("not a riff chunk");

        return Ok<Chunk>(header.id, s.nextBytes(header.size));
    }

    Io::BScan begin() const {
        return data;
    }

    Res<> expectId(Id expected) const {
        if (id != expected)
            return Error::invalidData("unexpected riff chunk");
        return Ok();
    }
};

} // namespace Karm::Av::Riff