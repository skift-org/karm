module;

#include <karm/macros>

export module Karm.Av:wav;

import Karm.Logger;

import :audio;
import :riff;

namespace Karm::Av::Wav {

Riff::Id WAVE = {'W', 'A', 'V', 'E'};
Riff::Id FMT = {'f', 'm', 't', ' '};
Riff::Id DATA = {'d', 'a', 't', 'a'};

export bool sniff(Bytes bytes) {
    return bytes.len() >= 12 and
           bytes[0] == 'R' and
           bytes[1] == 'I' and
           bytes[2] == 'F' and
           bytes[3] == 'F' and

           bytes[8] == 'W' and
           bytes[9] == 'A' and
           bytes[10] == 'V' and
           bytes[11] == 'E';
}

enum struct Type : u16 {
    PCM = 1,
};

struct PcmFormat {
    u16le channels;
    u32le freq;
    u32le bps;
    u16le align;
    u16le bitsPerSample;

    void repr(Io::Emit& e) const {
        e("(pcm-format channels:{} freq:{} bps:{} align:{})", channels, freq, bps, align);
    }
};

struct Decoder {
    // MARK: Format ------------------------------------------------------------
    Union<None, PcmFormat> _format = {};

    Res<> _handleFmt(Riff::Chunk& c) {
        Io::BScan s = c.begin();
        Type type = s.nextLe<Type>();
        if (type != Type::PCM)
            return Error::invalidData("only pcm data supported");
        _format = s.next<PcmFormat>();
        return Ok();
    }

    // MARK: Data --------------------------------------------------------------

    Res<Rc<Audio>> _handleData(Riff::Chunk& c) {
        Io::BScan s = c.begin();
        if (not _format.is<PcmFormat>())
            return Error::invalidData("only pcm data supported");

        auto format = _format.unwrap<PcmFormat>();

        if (format.bitsPerSample != 16)
            return Error::invalidData("only pcm data supported");

        auto audio =
            makeRc<Audio>(
                Av::Format{
                    format.freq,
                    format.channels,
                },
                c.data.len() / 2 / format.channels
            );

        for (auto sample : audio->frames().iter()) {
            for (auto channel : urange::zeroTo(format.channels)) {
                sample.samples[channel] = s.nextI16le() / static_cast<f32>(Limits<i16>::MAX);
            }
        }

        return Ok(audio);
    }

    Res<Rc<Audio>> decode(Bytes bytes) {
        auto riffChunk = try$(Riff::Chunk::read(bytes));
        try$(riffChunk.expectId(Riff::RIFF));

        Io::BScan s = riffChunk.begin();
        if (s.next<Riff::Id>() != WAVE)
            return Error::invalidData("expected wave data");

        while (not s.ended()) {
            auto chunk = try$(Riff::Chunk::read(s));

            if (chunk.id == FMT)
                try$(_handleFmt(chunk));
            else if (chunk.id == DATA)
                return _handleData(chunk);
            else
                logWarn("unsupported chunk {}", Str{chunk.id});
        }

        return Error::invalidData("missing data chunk");
    }
};

export struct Encoder : Stream {
    Io::Writer& _w;
    Format format;

    Encoder(Io::Writer& w, Format format)
        : _w(w), format(format) {}

    static Res<Rc<Encoder>> create(Io::Writer& w, Format format) {
        Io::BEmit e{w};

        Riff::ChunkHeader riffHeader;
        riffHeader.id = Riff::RIFF;
        riffHeader.size = Limits<u32>::MAX;
        try$(e.writeFrom(riffHeader));
        try$(e.writeFrom(WAVE));

        Riff::ChunkHeader fmtHeader;
        fmtHeader.id = FMT;
        fmtHeader.size = 1 + sizeof(PcmFormat);
        try$(e.writeFrom(fmtHeader));
        try$(e.writeFrom(Type::PCM));
        try$(e.writeFrom(PcmFormat{
            .channels = format.channels,
            .freq = format.rate,
            .bps = 16 * format.channels * format.rate,
            .align = 2,
            .bitsPerSample = 16,
        }));

        Riff::ChunkHeader dataHeader;
        dataHeader.id = DATA;
        dataHeader.size = Limits<u32>::MAX;
        try$(e.writeFrom(dataHeader));

        return Ok(makeRc<Encoder>(w, format));
    }

    Res<usize> process(Frames input, Frames) override {
        Io::BEmit e{_w};
        for (auto frame : input.iter()) {
            for (auto sample : frame.samples) {
                try$(e.writeI16le(static_cast<i16>(sample * Limits<i16>::MAX)));
            }
        }
        return Ok(input.len());
    }
};

} // namespace Karm::Av::Wav
