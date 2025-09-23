export module Karm.Av:audio;

import Karm.Core;

namespace Karm::Av {

export struct Format {
    usize rate = 44100; // hz
    usize channels = 1;
    usize frames = 2048;

    bool operator==(Format const&) const = default;
};

export struct Frame {
    usize index;
    MutSlice<f32> samples;

    void mono(f32 v) {
        for (auto& d : samples)
            d = v;
    }

    f32 mono() {
        f32 v = 0;
        for (auto& d : samples)
            v += d;
        return v / samples.len();
    }

    void clip() {
        for (auto& d : samples)
            d = clamp(d, -1., 1.);
    }

    void volume(f32 v) {
        f32 volMin = 1 / 100.;
        f32 volMax = Math::log(100.);
        f32 factor = volMin * Math::exp(volMax * v);
        for (auto& d : samples)
            d *= factor;
    }
};

struct FramesIter;

export struct Frames {
    Format format;
    MutSlice<f32> samples;

    Frame operator[](usize index) {
        auto sampleData = mutSub(
            samples,
            index * format.channels,
            index * format.channels + format.channels
        );
        return Frame{index, sampleData};
    }

    FramesIter iter();

    usize len() {
        return samples.len() / format.channels;
    }

    Frames sub(usize start) {
        auto sampleData = mutNext(
            samples,
            start * format.channels
        );
        return Frames{format, sampleData};
    }

    Frames sub(usize start, usize end) {
        auto sampleData = mutSub(
            samples,
            start * format.channels,
            end * format.channels
        );
        return Frames{format, sampleData};
    }
};

struct FramesIter {
    Frames& _self;
    usize index = 0;

    Opt<Frame> next() {
        if (index >= _self.len())
            return NONE;
        return _self[index++];
    }
};

export using Karm::begin;
export using Karm::end;

FramesIter Frames::iter() {
    return FramesIter{*this};
}

export struct Stream {
    virtual ~Stream() = default;
    virtual void process(Frames input, Frames output) = 0;
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

    void play(Rc<Stream> stream) {
        _stream = stream;
    }

    virtual void pause(bool on) = 0;
};

export usize resamples(Frames from, Frames to) {
    auto ff = from.format;
    auto tf = to.format;

    if (from.len() == 0 || to.len() == 0)
        return 0;

    auto inCh = ff.channels;
    auto outCh = tf.channels;

    // Fast path: same rate
    if (ff.rate == tf.rate) {
        auto n = min(from.len(), to.len());
        if (inCh == outCh) {
            // 1:1 copy per channel
            for (auto i : range(n)) {
                auto fi = from[i].samples;
                auto ti = to[i].samples;
                for (usize c = 0; c < outCh; ++c)
                    ti[c] = fi[c];
            }
        } else {
            // Mix to mono then fan out
            for (auto i : range(n)) {
                f32 m = from[i].mono();
                to[i].mono(m);
            }
        }
        return n; // consumed n input frames
    }

    // Resampling path: linear interpolation on a mono mix, then fan out
    f64 step = static_cast<f64>(ff.rate) / static_cast<f64>(tf.rate);
    f64 pos = 0.0;

    usize consumed = 0;
    usize maxIn = from.len();

    for (usize o = 0; o < to.len(); ++o) {
        usize i0 = static_cast<usize>(Math::floor(pos));
        if (i0 >= maxIn)
            break;

        usize i1 = i0 + 1;
        if (i1 >= maxIn)
            i1 = maxIn - 1; // clamp on tail

        f32 w1 = static_cast<f32>(pos - static_cast<f64>(i0));
        f32 w0 = 1.0f - w1;

        // Mono mix at i0 and i1
        f32 x0 = from[i0].mono();
        f32 x1 = from[i1].mono();

        f32 v = w0 * x0 + w1 * x1;

        // Fan out to target channels
        to[o].mono(v);

        pos += step;

        // Track how many input frames we effectively consumed
        // We consider frames up to i1 as "touched".
        usize used = i1 + 1;
        if (used > consumed)
            consumed = used;

        // If we've reached the end of input and the next step would overflow,
        // stop here to avoid reading past the end.
        if (i1 == maxIn - 1 and pos >= static_cast<f64>(maxIn - 1))
            break;
    }

    return min(consumed, maxIn);
}

struct Audio {
    Format format;
    Vec<f32> samples;

    Audio(Format format, usize frames)
        : format(format) {
        samples.resize(frames * format.channels);
    }

    Frames frames() {
        return {format, samples};
    }

    Duration duration() const {
        f64 secs = samples.len() / static_cast<f64>(format.rate * format.channels);
        return Duration::fromUSecs(secs * Duration::fromSecs(1).toUSecs());
    }

    usize fill(usize frame, Frames output) {
        return resamples(frames().sub(frame), output);
    }
};

export struct Player : Stream {
    enum struct Status {
        STOPPED,
        PAUSED,
        PLAYING,
        ENDED,
    };

    using enum Status;

    mutable Opt<Rc<Audio>> _audio;
    bool _pause = true;
    bool _mute = false;
    f64 _volume = 1;
    Atomic<usize> _currentFrame;

    void seek(Duration offset) {
        if (_audio.has()) {
            auto fmt = _audio.unwrap()->format;
            f64 secs = offset.toUSecs() / static_cast<f64>(Duration::fromSecs(1).toUSecs());
            usize frame = static_cast<usize>(secs * static_cast<f64>(fmt.rate));
            _currentFrame.store(clamp(frame, 0uz, _audio.unwrap()->frames().len()));
        }
    }

    Duration tell() const {
        auto curr = _currentFrame.load();
        auto fmt = _audio.unwrap()->format;
        f64 secs = curr / static_cast<f64>(fmt.rate);
        return Duration::fromUSecs(secs * Duration::fromSecs(1).toUSecs());
    }

    Duration duration() const {
        if (_audio.has())
            return _audio.unwrap()->duration();
        return Duration::fromSecs(0);
    }

    void play(Rc<Audio> audio) { _audio = audio; }

    void stop() { _audio = NONE; }

    void pause(bool on) { _pause = on; }

    bool pause() const { return _pause; }

    void mute(bool on) { _mute = on; }

    bool mute() const { return _mute; }

    void volume(f64 vol) { _volume = vol; }

    f64 volume() const { return _volume; }

    Status status() const {
        if (not _audio)
            return Status::STOPPED;
        if (_currentFrame.load() >= _audio.unwrap()->frames().len())
            return Status::ENDED;
        if (_pause)
            return Status::PAUSED;
        return Status::PLAYING;
    }

    void process(Frames, Frames output) override {
        if (_pause or not _audio) {
            for (auto f : output.iter())
                f.mono(0);
        } else {
            auto curr = _currentFrame.load();
            _currentFrame.store(curr + _audio.unwrap()->fill(curr, output));
            if (_mute or _volume != 1) {
                for (auto f : output.iter())
                    f.volume(_mute ? 0. : _volume);
            }
        }
    }
};

} // namespace Karm::Av
