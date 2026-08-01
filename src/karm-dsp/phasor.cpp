export module Karm.Dsp:phasor;

import Karm.Core;
import Karm.Math;

import :base;

namespace Karm::Dsp {

export struct Phasor {
    f64 _phase = 0;

    f64 phase() const {
        return _phase;
    }

    void update(Device& dev, f64 freq) {
        _phase += freq / dev.samplingRate;
        if (_phase >= 1.0)
            _phase -= 1.0;
        if (_phase < 0)
            _phase += 1.0;
    }

    f64 sineOsc(Device& dev, f64 freq) {
        f64 s = Math::sin(_phase * 2.0 * Math::PI);
        update(dev, freq);
        return s;
    }
};

} // namespace Karm::Dsp