export module Karm.Av:audio;

import Karm.Core;

namespace Karm::Av {

export struct Format {
    usize rate = 44100; // hz
    usize channels = 1;
    usize frames = 2048;
};

export struct Sample {
    usize index;
    MutSlice<f32> data;

    void all(f32 v) {
        for (auto& d : data)
            d = v;
    }

    void clip() {
        for (auto& d : data)
            d = clamp(d, -1., 1.);
    }

    void volume(f32 v) {
        for (auto& d : data)
            d *= v;
    }
};

struct SamplesIter;

export struct Samples {
    Format format;
    MutSlice<f32> data;

    Sample operator[](usize index) {
        auto sampleData = mutSub(
            data,
            index * format.channels,
            index * format.channels + format.channels
        );
        return Sample{index, sampleData};
    }

    SamplesIter iter();

    usize len() {
        return data.len() / format.channels;
    }
};

struct SamplesIter {
    Samples& _self;
    usize index = 0;

    Opt<Sample> next() {
        if (index >= _self.len())
            return NONE;
        return _self[index++];
    }
};

export using Karm::begin;
export using Karm::end;

SamplesIter Samples::iter() {
    return SamplesIter{*this};
}

export struct Stream {
    virtual ~Stream() = default;
    virtual void process(Samples input, Samples output) = 0;
};

struct Options {
    Format input;
    Format output;
};

export struct Device;

namespace _Embed {

export Res<Rc<Device>> create(Options const& options);

} // namespace _Embed

struct Device {
    Options _options;
    Opt<Rc<Stream>> _stream;

    static Res<Rc<Device>> create(Options const& options = {}) {
        return _Embed::create(options);
    }

    Device(Options options)
        : _options(options) {}

    virtual ~Device() = default;

    void play(Rc<Stream> stream) { _stream = stream; }

    virtual void pause(bool on) = 0;
};

} // namespace Karm::Av