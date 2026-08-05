#include <karm/entry>

import Karm.Core;
import Karm.Av;
import Karm.Sys;
import Karm.Ref;
import Karm.Math;
import Karm.Cli;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto audioUrl = Cli::operand<Ref::Url>("url"s, "Audio file to play"s);

    Cli::Command cmd{
        "audio-play"s,
        "Play audio file."s,
        {
            {
                "Arguments"s,
                {
                    audioUrl,
                },
            },
        },
    };

    co_try$(cmd.exec(env));

    if (not cmd)
        co_return Ok();
    auto device = co_try$(Av::Device::create());
    auto player = makeRc<Av::Player>();
    auto audio = co_try$(Av::load(audioUrl));
    player->play(audio);
    device->play(player);
    device->pause(false);
    player->pause(false);

    while (player->status() == Av::Player::PLAYING) {
        Sys::println("Playing {}... ({}/{})", audioUrl.value(), player->tell(), audio->duration());
        co_try$(Sys::sleep(Duration::fromSecs(1)));
    }

    co_return Ok();
}
