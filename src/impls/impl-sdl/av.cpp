module;

#include <SDL3/SDL.h>
#include <karm-core/macros.h>

module Karm.Av;

import Karm.Core;

namespace Karm::Av::_Embed {

struct SdlDevice : Device {
    SDL_AudioStream* _sdlStream = nullptr;
    Vec<f32> _outputBuf;

    SdlDevice(Options const& options) : Device(options) {
        _outputBuf.resize(options.output.channels * options.output.frames);
    }

    ~SdlDevice() override {
        if (_sdlStream)
            SDL_DestroyAudioStream(_sdlStream);
    }

    void pause(bool on) override {
        if (on)
            SDL_PauseAudioStreamDevice(_sdlStream);
        else
            SDL_ResumeAudioStreamDevice(_sdlStream);
    }
};

static void _callback(void* userdata, SDL_AudioStream* sdlStream, int additionalAmount, int totalAmount) {
    if (additionalAmount == 0 || totalAmount == 0)
        return;
    
    SdlDevice* device = static_cast<SdlDevice*>(userdata);
    if (not device->_stream)
        return;
    auto stream = device->_stream.unwrap();

    Frames input;
    input.format = device->_options.input;
    Frames output;
    output.format = device->_options.output;
    output.samples = device->_outputBuf;

    stream->process(input, output);
    SDL_PutAudioStreamData(sdlStream, device->_outputBuf.buf(), sizeOf(device->_outputBuf));
}

Res<Rc<Device>> create(Options const& options) {
    SDL_Init(SDL_INIT_AUDIO);
    auto stream = makeRc<SdlDevice>(options);

    SDL_AudioSpec want = {};
    want.freq = options.output.rate;
    want.format = SDL_AUDIO_F32LE;
    want.channels = static_cast<u8>(options.output.channels);

    stream->_sdlStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, _callback, &stream.unwrap());
    if (not stream->_sdlStream)
        return Error::other(SDL_GetError());

    return Ok(stream);
}

} // namespace Karm::Av::_Embed
