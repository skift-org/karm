#include <karm-sys/entry.h>

import Karm.Core;
import Karm.Av;
import Karm.Sys;
import Karm.Ref;
import Karm.Math;

using namespace Karm;

struct Sin : Av::Stream {
    f64 freq = 440.0f; // A4
    f64 phase = 0.0f;

    void process(Av::Frames, Av::Frames output) override {
        f64 step = 2.0f * Math::PI * freq / output.format.rate;
        for (Av::Frame frame : output.iter()) {
            f64 s = Math::sin(phase);
            phase += step;
            if (phase >= 2.0f * Math::PI)
                phase -= 2.0f * Math::PI;
            frame.mono(s);
            frame.clip();
        }
    }
};

Async::Task<> entryPointAsync(Sys::Context&, Async::CancellationToken) {
    auto url = "bundle://karm-av/audio/free-software.wav"_url;
    auto device = co_try$(Av::Device::create());
    auto player = makeRc<Av::Player>();
    auto audio = co_try$(Av::load(url));
    player->play(audio);
    device->play(player);
    device->pause(false);
    player->pause(false);

    while (player->status() == Av::Player::PLAYING) {
        Sys::println("Playing {}... ({}/{})", url, player->tell(), audio->duration());
        co_try$(Sys::sleep(Duration::fromSecs(1)));
    }

    co_return Ok();
}
