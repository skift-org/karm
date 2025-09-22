module;

#include <SDL2/SDL.h>
#include <karm-core/macros.h>

module Karm.Av;

import Karm.Core;

namespace Karm::Av::_Embed {

struct SdlDevice : Device {
    SDL_AudioDeviceID _dev = 0;

    SdlDevice(Options const& options) : Device(options) {}

    ~SdlDevice() override {
        SDL_CloseAudioDevice(_dev);
    }

    void pause(bool on) override {
        if (_dev)
            SDL_PauseAudioDevice(_dev, on);
    }
};

static void _callback(void* userdata, Uint8* data, int len) {
    SdlDevice* device = static_cast<SdlDevice*>(userdata);
    if (not device->_stream)
        return;
    auto stream = device->_stream.unwrap();

    Samples input;
    input.format = device->_options.input;
    Samples output;
    output.format = device->_options.output;
    output.data = {reinterpret_cast<f32*>(data), len / sizeof(f32)};

    stream->process(input, output);
}

Res<Rc<Device>> create(Options const& options) {
    SDL_Init(SDL_INIT_AUDIO);
    auto stream = makeRc<SdlDevice>(options);

    SDL_AudioSpec want = {};
    SDL_AudioSpec have = {};

    want.freq = options.output.rate;
    want.format = AUDIO_F32SYS;
    want.channels = static_cast<u8>(options.output.channels);
    want.samples = static_cast<u16>(options.output.frames);
    want.callback = _callback;
    want.userdata = &stream.unwrap();

    stream->_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (not stream->_dev)
        return Error::other(SDL_GetError());

    return Ok(stream);
}

} // namespace Karm::Av::_Embed
