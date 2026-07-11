#include <karm/entry>

import Karm.Core;
import Karm.Av;
import Karm.Sys;
import Karm.Ref;
import Karm.Math;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

struct Sin : Av::Stream {
    f64 freq = 440.0f; // A4
    f64 phase = 0.0f;

    Res<usize> process(Av::Frames, Av::Frames output) override {
        f64 step = 2.0f * Math::PI * freq / output.format.rate;
        for (Av::Frame frame : output.iter()) {
            f64 s = Math::sin(phase);
            phase += step;
            if (phase >= 2.0f * Math::PI)
                phase -= 2.0f * Math::PI;
            frame.mono(s);
            frame.clip();
        }
        return Ok(output.len());
    }
};

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    Av::Format format = {
        .rate = 44100,
        .channels = 1,
    };
    Sin sin;
    auto file = co_try$(Sys::File::create("file:out.wav"_url));
    auto encoder = co_try$(
        Av::Wav::Encoder::create(
            file,
            format
        )
    );
    co_try$(Av::pump(sin, *encoder, format, format.toFrames(10_s)));
    co_return Ok();
}
