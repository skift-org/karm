module;

#include <karm/macros>

export module Karm.Dsp:reverb;

import Karm.Core;
import Karm.Math;

namespace Karm::Dsp {

// Reverb after Jon Dattorro
// https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf

// MARK: Delay Line ------------------------------------------------------------

export enum struct Tap : u8 {
    MAIN,
    OUT1,
    OUT2,
    OUT3,

    _LEN,
};

/// Circular delay line with more than one read tap.
export struct DelayLine {
    Vec<f64> _buf;
    u16 _mask = 0;
    Array<u16, usize(Tap::_LEN)> _readOff = {};

    DelayLine() = default;

    DelayLine(u16 delay) {
        init(delay);
    }

    /// Allocate the storage. The length is always a power of two.
    void init(u16 delay) {
        u16 bits = 0;
        for (u16 x = delay; x; x >>= 1)
            bits++;

        usize len = usize(1) << bits;
        _buf.resize(len, 0.);

        // The mask wraps the index of the circular buffer
        _mask = u16(len - 1);
        tap(Tap::MAIN, delay);
    }

    /// Set the delay of one tap.
    void tap(Tap t, u16 delay) {
        _readOff[usize(t)] = u16(_mask + 1 - delay);
    }

    /// Move the main tap. This modulates the delay line.
    always_inline void nudge(i16 delta) {
        _readOff[usize(Tap::MAIN)] += delta;
    }

    always_inline f64 read(Tap t, u16 time) const {
        return _buf[(time + _readOff[usize(t)]) & _mask];
    }

    always_inline void write(u16 time, f64 in) {
        _buf[time & _mask] = in;
    }

    /// Write the input, then read the delayed output.
    always_inline f64 process(u16 time, f64 in) {
        write(time, in);
        return read(Tap::MAIN, time);
    }

    /// All pass filter that uses this delay line.
    /// It keeps the amplitude per frequency but changes the phase.
    f64 allPass(u16 time, f64 gain, f64 in) {
        f64 delayed = read(Tap::MAIN, time);
        in += delayed * -gain;
        write(time, in);
        return delayed + in * gain;
    }
};

// MARK: Low Pass --------------------------------------------------------------

/// One pole low pass filter.
export struct LowPass {
    f64 _state = 0.;

    always_inline f64 process(f64 freq, f64 in) {
        _state += (in - _state) * freq;
        return _state;
    }
};

// MARK: Reverb ----------------------------------------------------------------

export struct Reverb {
    static constexpr u16 MAX_PREDELAY = 4800; // 100ms at 48kHz

    DelayLine _preDelay;
    Array<DelayLine, 4> _inDiff;
    Array<DelayLine, 2> _decayDiff1;
    Array<DelayLine, 2> _preDamp;
    Array<DelayLine, 2> _decayDiff2;
    Array<DelayLine, 2> _postDamp;

    LowPass _preFilter;
    Array<LowPass, 2> _damp;

    f64 _preFilterAmount = 0.;
    f64 _inDiff1Amount = 0.;
    f64 _inDiff2Amount = 0.;
    f64 _decayDiff1Amount = 0.;
    f64 _decayDiff2Amount = 0.;
    f64 _decayAmount = 0.;
    f64 _dampAmount = 0.;

    u16 _time = 0;

    Reverb() {
        _preDelay.init(MAX_PREDELAY);

        _inDiff[0].init(142);
        _inDiff[1].init(107);
        _inDiff[2].init(379);
        _inDiff[3].init(277);

        _decayDiff1[0].init(672); // + excursion

        _preDamp[0].init(4453);
        _preDamp[0].tap(Tap::OUT1, 353);
        _preDamp[0].tap(Tap::OUT2, 3627);
        _preDamp[0].tap(Tap::OUT3, 1990);

        _decayDiff2[0].init(1800);
        _decayDiff2[0].tap(Tap::OUT1, 187);
        _decayDiff2[0].tap(Tap::OUT2, 1228);

        _postDamp[0].init(3720);
        _postDamp[0].tap(Tap::OUT1, 1066);
        _postDamp[0].tap(Tap::OUT2, 2673);

        _decayDiff1[1].init(908); // + excursion

        _preDamp[1].init(4217);
        _preDamp[1].tap(Tap::OUT1, 266);
        _preDamp[1].tap(Tap::OUT2, 2974);
        _preDamp[1].tap(Tap::OUT3, 2111);

        _decayDiff2[1].init(2656);
        _decayDiff2[1].tap(Tap::OUT1, 335);
        _decayDiff2[1].tap(Tap::OUT2, 1913);

        _postDamp[1].init(3163);
        _postDamp[1].tap(Tap::OUT1, 121);
        _postDamp[1].tap(Tap::OUT2, 1996);

        preDelay(0.1);
        preFilter(0.85);
        inputDiffusion1(0.75);
        inputDiffusion2(0.625);
        decay(0.75);
        decayDiffusion(0.70);
        damping(0.95);
    }

    // MARK: Controls ----------------------------------------------------------

    void preDelay(f64 v) {
        _preDelay.tap(Tap::MAIN, u16(v * MAX_PREDELAY));
    }

    void preFilter(f64 v) {
        _preFilterAmount = v;
    }

    void inputDiffusion1(f64 v) {
        _inDiff1Amount = v;
    }

    void inputDiffusion2(f64 v) {
        _inDiff2Amount = v;
    }

    void decayDiffusion(f64 v) {
        _decayDiff1Amount = v;
    }

    void damping(f64 v) {
        _dampAmount = v;
    }

    void decay(f64 v) {
        _decayAmount = v;
        _decayDiff2Amount = clamp(v + 0.15, 0.25, 0.50);
    }

    // MARK: Processing --------------------------------------------------------

    void process(f64 in) {
        if ((_time & 0x07ff) == 0) {
            i16 delta = _time < (1 << 15) ? -1 : 1;
            _decayDiff1[0].nudge(delta);
            _decayDiff1[1].nudge(delta);
        }

        f64 x = _preDelay.process(_time, in);
        x = _preFilter.process(_preFilterAmount, x);

        x = _inDiff[0].allPass(_time, _inDiff1Amount, x);
        x = _inDiff[1].allPass(_time, _inDiff1Amount, x);
        x = _inDiff[2].allPass(_time, _inDiff2Amount, x);
        x = _inDiff[3].allPass(_time, _inDiff2Amount, x);

        for (usize i = 0; i < 2; i++) {
            // Add the cross feedback from the other half
            f64 y = x + _postDamp[1 - i].read(Tap::MAIN, _time) * _decayAmount;

            y = _decayDiff1[i].allPass(_time, -_decayDiff1Amount, y);
            y = _preDamp[i].process(_time, y);
            y = _damp[i].process(_dampAmount, y);
            y *= _decayAmount;
            y = _decayDiff2[i].allPass(_time, _decayDiff2Amount, y);
            _postDamp[i].write(_time, y);
        }

        _time++;
    }

    // MARK: Output ------------------------------------------------------------
    // The wet signal is a sum of taps of the network.
    // Read these after process().

    f64 left() const {
        f64 a = _preDamp[1].read(Tap::OUT1, _time);
        a += _preDamp[1].read(Tap::OUT2, _time);
        a -= _decayDiff2[1].read(Tap::OUT2, _time);
        a += _postDamp[1].read(Tap::OUT2, _time);
        a -= _preDamp[0].read(Tap::OUT3, _time);
        a -= _decayDiff2[0].read(Tap::OUT1, _time);
        a += _postDamp[0].read(Tap::OUT1, _time);
        return a;
    }

    f64 right() const {
        f64 a = _preDamp[0].read(Tap::OUT1, _time);
        a += _preDamp[0].read(Tap::OUT2, _time);
        a -= _decayDiff2[0].read(Tap::OUT2, _time);
        a += _postDamp[0].read(Tap::OUT2, _time);
        a -= _preDamp[1].read(Tap::OUT3, _time);
        a -= _decayDiff2[1].read(Tap::OUT1, _time);
        a += _postDamp[1].read(Tap::OUT1, _time);
        return a;
    }
};

} // namespace Karm::Dsp