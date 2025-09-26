module;

#include <SDL3/SDL.h>
#include <karm-core/macros.h>
#include <karm-gfx/buffer.h>

module Karm.Av;
import Karm.Core;

namespace Karm::Av::_Embed {

// MARK: Audio Playback --------------------------------------------------------

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
    if (additionalAmount == 0 or totalAmount == 0)
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

// MARK: Video Capture ---------------------------------------------------------

struct SdlCameraStream : Av::VideoStream {
    SDL_Camera* _sdlCamera;
    Opt<VideoFrame> _curr;

    SdlCameraStream(SDL_Camera* sdlCamera)
        : _sdlCamera(sdlCamera) {}

    ~SdlCameraStream() {
        SDL_CloseCamera(_sdlCamera);
    }

    Opt<VideoFrame> next() override {
        u64 timestampNs = 0;
        SDL_Surface* sdlSurface = SDL_AcquireCameraFrame(_sdlCamera, &timestampNs);
        if (not sdlSurface)
            return _curr;
        SDL_Surface* sdlConverted = SDL_ConvertSurface(sdlSurface, SDL_PIXELFORMAT_BGRA32);

        Gfx::Pixels sdlPixels = {
            sdlConverted->pixels,
            {sdlConverted->w, sdlConverted->h},
            (usize)sdlConverted->pitch,
            Gfx::BGRA8888,
        };

        auto surface = Gfx::Surface::alloc({sdlSurface->w, sdlSurface->h});
        Gfx::blitUnsafe(surface->mutPixels(), sdlPixels);

        SDL_DestroySurface(sdlConverted);
        SDL_ReleaseCameraFrame(_sdlCamera, sdlSurface);

        _curr = VideoFrame{
            surface,
            Duration::fromSecs(0),
        };
        return _curr;
    }
};

struct SdlCamera : Av::Camera {
    SDL_CameraID _id;

    SdlCamera(SDL_CameraID id) : _id(id) {}

    Res<Rc<VideoStream>> startCapture() override {
        return Ok(makeRc<SdlCameraStream>(SDL_OpenCamera(_id, nullptr)));
    }

    CameraInfo info() const override {
        return {
            .name = Str{SDL_GetCameraName(_id)},
            .driver = Str{SDL_GetCameraDriver(_id)}
        };
    }

    Vec<CameraFormat> formats() const override {
        int len;
        auto* formats = SDL_GetCameraSupportedFormats(_id, &len);
        Defer _ = [&] {
            SDL_free(formats);
        };
        Vec<Av::CameraFormat> fmts;
        for (int i = 0; i < len; i++) {
            auto& f = *formats[i];
            fmts.pushBack({
                .resolution = {f.width, f.width},
                .framerate = f.framerate_numerator / static_cast<f64>(f.framerate_denominator),
            });
        }
        return fmts;
    }
};

Res<Rc<Camera>> openDefaultCamera() {
    SDL_Init(SDL_INIT_CAMERA);
    int devcount = 0;
    SDL_CameraID* devices = SDL_GetCameras(&devcount);
    if (not devices)
        return Error::other("no camera devices");
    Defer _ = [&] {
        SDL_free(devices);
    };
    return Ok(makeRc<SdlCamera>(devices[0]));
}

} // namespace Karm::Av::_Embed
