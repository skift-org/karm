#include <karm-math/rand.h>
#include <karm-sys/entry.h>
#include <karm-sys/proc.h>
#include <karm-sys/time.h>

import Karm.Core;
import Karm.Av;

using namespace Karm;

struct Sin : Av::Stream {
    f64 freq = 440.0f; // A4
    f64 phase = 0.0f;

    void process(Av::Samples, Av::Samples output) override {
        f64 step = 2.0f * Math::PI * freq / output.format.rate;
        for (Av::Sample sample : output.iter()) {
            f64 s = Math::sin(phase);
            phase += step;
            if (phase >= 2.0f * Math::PI)
                phase -= 2.0f * Math::PI;
            sample.all(s);
            sample.clip();
        }
    }
};

Async::Task<> entryPointAsync(Sys::Context&) {
    auto stream = co_try$(Av::Device::create());
    stream->play(makeRc<Sin>());
    stream->pause(false);
    co_try$(Sys::sleep(Duration::fromSecs(10)));

    co_return Ok();
}
