#include <karm/test>

import Karm.Core;
import Karm.Math;
import Karm.Dsp;

namespace Karm::Dsp::Tests {

test$("fft-matches-naive-dft") {
    usize n = 64;
    auto plan = Fft::create(n);

    Vec<Math::Complexf> input;
    input.resize(n);
    for (usize i = 0; i < n; i++)
        input[i] = {f32(Math::sin(f64(i) * 0.37) + 0.3 * Math::cos(f64(i) * 2.1)), 0.0f};

    Vec<Math::Complexf> expected;
    expected.resize(n);
    naiveDft(input, mutSub(expected));

    Vec<Math::Complexf> got = input;
    plan.forward(mutSub(got));

    for (usize i = 0; i < n; i++)
        expect$((got[i] - expected[i]).magnitude() < 1e-3f);

    return Ok();
}

test$("fft-round-trip") {
    usize n = 256;
    auto plan = Fft::create(n);

    Vec<Math::Complexf> input;
    input.resize(n);
    for (usize i = 0; i < n; i++)
        input[i] = {f32(Math::sin(f64(i) * 0.05)) * 0.5f, 0.0f};

    Vec<Math::Complexf> got = input;
    plan.forward(mutSub(got));
    plan.inverse(mutSub(got));

    for (usize i = 0; i < n; i++)
        expect$((got[i] - input[i]).magnitude() < 1e-4f);

    return Ok();
}

test$("fft-tone-lands-in-one-bin") {
    usize n = 1024;
    usize tone = 40;
    auto plan = Fft::create(n);

    Vec<Math::Complexf> input;
    input.resize(n);
    for (usize i = 0; i < n; i++)
        input[i] = {f32(Math::cos(2.0 * Math::PI * f64(tone) * f64(i) / f64(n))), 0.0f};

    plan.forward(mutSub(input));

    usize peak = 0;
    for (usize i = 0; i < plan.binCount(); i++)
        if (input[i].magnitude() > input[peak].magnitude())
            peak = i;

    expectEq$(peak, tone);

    // Half of the energy goes to the mirror bin, thus the peak is 0.5.
    expect$(Math::abs(input[peak].magnitude() / f32(n) - 0.5f) < 1e-3f);

    return Ok();
}

} // namespace Karm::Dsp::Tests
