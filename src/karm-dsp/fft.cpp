export module Karm.Dsp:fft;

import Karm.Core;
import Karm.Math;

namespace Karm::Dsp {

export void naiveDft(Slice<Math::Complexf> in, MutSlice<Math::Complexf> out) {
    usize n = in.len();
    for (usize k = 0; k < n; k++) {
        f64 re = 0.0;
        f64 im = 0.0;
        for (usize t = 0; t < n; t++) {
            f64 angle = -2.0 * Math::PI * f64(k) * f64(t) / f64(n);
            re += in[t].re * Math::cos(angle) - in[t].im * Math::sin(angle);
            im += in[t].re * Math::sin(angle) + in[t].im * Math::cos(angle);
        }
        out[k] = {
            f32(re),
            f32(im),
        };
    }
}

// https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
export struct Fft {
    usize _size;
    Vec<usize> _reversal;
    Vec<Math::Complexf> _twiddles;

    static Fft create(usize size) {
        if (size < 2 or (size & (size - 1)) != 0)
            panic("fft size must be a power of two");

        usize bits = 0;
        while ((usize{1} << bits) < size)
            bits++;

        Vec<usize> reversal;
        reversal.resize(size);
        for (usize i = 0; i < size; i++)
            reversal[i] = reverseBits(i, bits);

        Vec<Math::Complexf> twiddles;
        twiddles.resize(size / 2);
        for (usize i = 0; i < size / 2; i++) {
            f64 angle = -2.0 * Math::PI * f64(i) / f64(size);
            twiddles[i] = {f32(Math::cos(angle)), f32(Math::sin(angle))};
        }

        return Fft{
            size,
            std::move(reversal),
            std::move(twiddles),
        };
    }

    usize size() const {
        return _size;
    }

    usize binCount() const {
        return _size / 2;
    }

    void forward(MutSlice<Math::Complexf> buf) const {
        if (buf.len() != _size)
            panic("buffer does not match the plan size");

        for (usize i = 0; i < _size; i++) {
            usize j = _reversal[i];
            if (i < j)
                std::swap(buf[i], buf[j]);
        }

        for (usize len = 2; len <= _size; len <<= 1) {
            usize half = len / 2;
            usize stride = _size / len;

            for (usize base = 0; base < _size; base += len) {
                for (usize j = 0; j < half; j++) {
                    Math::Complexf twiddle = _twiddles[j * stride];
                    Math::Complexf even = buf[base + j];
                    Math::Complexf odd = buf[base + j + half] * twiddle;

                    buf[base + j] = even + odd;
                    buf[base + j + half] = even - odd;
                }
            }
        }
    }

    void inverse(MutSlice<Math::Complexf> buf) const {
        for (auto& value : buf)
            value = value.conjugate();

        forward(buf);

        f32 scale = f32{1} / f32(_size);
        for (auto& value : buf)
            value = value.conjugate() * scale;
    }
};

} // namespace Karm::Dsp
