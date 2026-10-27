#include <karm/entry>

import Karm.Math;
import Karm.Gfx;
import Karm.App;
import Karm.Image;
import Karm.Logger;

using namespace Karm;
using namespace Karm::Ref::Literals;

enum struct Sprite : u8 {
    AWAKE = 0x00,

    JARE = 0x30,

    KAKI1 = 0x31,
    KAKI2 = 0x32,

    MATI1 = 0x40,
    MATI2 = 0x41,

    SLEEP1 = 0x24,
    SLEEP2 = 0x34,

    UP1 = 0x50,
    UP2 = 0x44,

    DOWN1 = 0x10,
    DOWN2 = 0x01,

    LEFT1 = 0x03,
    LEFT2 = 0x13,

    RIGHT1 = 0x42,
    RIGHT2 = 0x43,

    UPLEFT1 = 0x51,
    UPLEFT2 = 0x52,

    UPRIGHT1 = 0x53,
    UPRIGHT2 = 0x54,

    DWLEFT1 = 0x21,
    DWLEFT2 = 0x02,

    DWRIGHT1 = 0x12,
    DWRIGHT2 = 0x22,

    UTOGI1 = 0x05,
    UTOGI2 = 0x15,

    DTOGI1 = 0x20,
    DTOGI2 = 0x11,

    LTOGI1 = 0x23,
    LTOGI2 = 0x33,

    RTOGI1 = 0x04,
    RTOGI2 = 0x14,
};

struct Anim {
    Sprite sp1;
    Sprite sp2 = sp1;
    isize speed = 1;

    Sprite play(isize tick) const {
        return (tick / speed) % 2 ? sp1 : sp2;
    }
};

static constexpr Anim STOP = {Sprite::MATI1};
static constexpr Anim JARE = {Sprite::JARE, Sprite::MATI1, 3};
static constexpr Anim KAKI = {Sprite::KAKI1, Sprite::KAKI2};
static constexpr Anim AKUBI = {Sprite::MATI2, Sprite::MATI2};
static constexpr Anim SLEEP = {Sprite::SLEEP1, Sprite::SLEEP2, 3};
static constexpr Anim AWAKE = {Sprite::AWAKE, Sprite::AWAKE};
static constexpr Anim MOVE_UP = {Sprite::UP1, Sprite::UP2};
static constexpr Anim MOVE_DOWN = {Sprite::DOWN1, Sprite::DOWN2};
static constexpr Anim MOVE_LEFT = {Sprite::LEFT1, Sprite::LEFT2};
static constexpr Anim MOVE_RIGHT = {Sprite::RIGHT1, Sprite::RIGHT2};
static constexpr Anim MOVE_UP_LEFT = {Sprite::UPLEFT1, Sprite::UPLEFT2};
static constexpr Anim MOVE_UP_RIGHT = {Sprite::UPRIGHT1, Sprite::UPRIGHT2};
static constexpr Anim MOVE_DOWN_LEFT = {Sprite::DWLEFT1, Sprite::DWLEFT2};
static constexpr Anim MOVE_DOWN_RIGHT = {Sprite::DWRIGHT1, Sprite::DWRIGHT2};

// static constexpr Anim TOGI_UP = {Sprite::UTOGI1, Sprite::UTOGI2};
// static constexpr Anim TOGI_DOWN = {Sprite::DTOGI1, Sprite::DTOGI2};
// static constexpr Anim TOGI_LEFT = {Sprite::LTOGI1, Sprite::LTOGI2};
// static constexpr Anim TOGI_RIGHT = {Sprite::RTOGI1, Sprite::RTOGI2};

static constexpr Anim WALK(Math::Vec2f dir) {
    Anim anims[3][3] = {
        {MOVE_UP_LEFT, MOVE_UP, MOVE_UP_RIGHT},
        {MOVE_LEFT, STOP, MOVE_RIGHT},
        {MOVE_DOWN_LEFT, MOVE_DOWN, MOVE_DOWN_RIGHT},
    };
    auto v = (dir.unit() * 2 + 1).roundi();
    return anims[clampIndex(v.y, 3l)][clampIndex(v.x, 3l)];
}

struct Neko {
    static constexpr Duration TICK_DURATION = Duration::fromMSecs(125);
    static constexpr int SPRITE_SIZE = 32;
    static constexpr int NEKO_SIZE = 64;
    static constexpr int MOVE_SPEED = 16;

    enum struct State : u8 {
        IDLE,
        CHASE_MOUSE,
        ITCHING,
        CLEANING,
        SLEEP,
        SURPRISED,
        WANDERING,
        YAWNING,
    };

    State _state = State::IDLE;
    Math::Vec2f _position;
    Math::Vec2f _destination;
    Math::Vec2f _lastMousePos;
    isize _timer = 0;
    isize _tick = 0;
    Math::Rand _rand;
    Rc<Gfx::Image> _sprites = Image::loadOrFallback("bundle://neko/sprites.png"_url).expect();

    void paint(Gfx::Canvas& g) {
        auto s = sprite();
        g.blit(
            {
                ((static_cast<u8>(s) >> 4) & 0xf) * SPRITE_SIZE,
                ((static_cast<u8>(s) >> 0) & 0xf) * SPRITE_SIZE,
                SPRITE_SIZE,
                SPRITE_SIZE,
            },
            Math::Rectf{
                _position.x - NEKO_SIZE / 2,
                _position.y - NEKO_SIZE,
                NEKO_SIZE,
                NEKO_SIZE,
            }
                .cast<isize>(),
            _sprites
        );
    }

    void moveTo(Math::Vec2f destintation) {
        if (_position.dist(destintation) < MOVE_SPEED) {
            _position = destintation;
        } else {
            _position += (destintation - _position).unit() * MOVE_SPEED;
        }
    }

    void transition(State to) {
        switch (to) {
        case State::IDLE:
            _timer = 4 + _rand.nextU32() % 16;
            break;
        case State::CHASE_MOUSE:
            break;
        case State::ITCHING:
            _timer = 8 + _rand.nextU32() % 8;
            break;
        case State::CLEANING:
            _timer = 16;
            break;
        case State::SLEEP:
            break;
        case State::SURPRISED:
            _timer = 2;
            break;
        case State::WANDERING:
            _destination = _lastMousePos +
                           Math::Vec2f{
                               _rand.nextDouble(-512, 512),
                               _rand.nextDouble(-512, 512),
                           };
            break;
        case State::YAWNING:
            _timer = 8 + _rand.nextU32() % 4;
            break;
        }
        _state = to;
    }

    bool mouseMoved(Math::Vec2f mouse) {
        return mouse.dist(_lastMousePos) > 64;
    }

    bool mouseMovedAlot(Math::Vec2f mouse) {
        return mouse.dist(_lastMousePos) > 64;
    }

    void update(Math::Vec2f mouse) {
        switch (_state) {
        case State::IDLE:
            if (not _timer) {
                Array next = {
                    State::WANDERING,
                    State::WANDERING,
                    State::CLEANING,
                    State::CLEANING,
                    State::ITCHING,
                    State::ITCHING,
                    State::YAWNING,
                };
                transition(_rand.pick(next));
            } else if (mouseMoved(mouse)) {
                transition(State::SURPRISED);
            }
            break;
        case State::CHASE_MOUSE:
            if (_position.dist(mouse) < MOVE_SPEED)
                transition(State::IDLE);
            else
                moveTo(mouse);
            break;
        case State::ITCHING:
            if (mouseMovedAlot(mouse))
                transition(State::SURPRISED);
            else if (not _timer) {
                transition(State::IDLE);
            }
            break;
        case State::CLEANING:
            if (mouseMoved(mouse))
                transition(State::SURPRISED);
            else if (not _timer)
                transition(State::IDLE);
            break;
        case State::SLEEP:
            if (mouseMovedAlot(mouse))
                transition(State::SURPRISED);
            break;
        case State::SURPRISED:
            if (not _timer)
                transition(State::CHASE_MOUSE);
            break;
        case State::WANDERING:
            if (mouseMovedAlot(mouse))
                transition(State::SURPRISED);
            else if (_position.dist(_destination) < MOVE_SPEED) {
                Array next = {
                    State::WANDERING,
                    State::IDLE,
                    State::IDLE,
                    State::IDLE,
                };
                transition(_rand.pick(next));
            } else
                moveTo(_destination);
            break;
        case State::YAWNING:
            if (mouseMovedAlot(mouse))
                transition(State::SURPRISED);
            else if (not _timer) {
                Array next = {
                    State::SLEEP,
                    State::IDLE,
                    State::IDLE,
                    State::IDLE,
                };
                transition(_rand.pick(next));
            }
            break;
        }

        if (_timer)
            _timer--;
        _tick++;
        _lastMousePos = mouse;
    }

    Sprite sprite() {
        switch (_state) {
        case State::IDLE:
            return STOP.play(_tick);
        case State::CHASE_MOUSE:
            return WALK(_lastMousePos - _position).play(_tick);
        case State::ITCHING:
            return KAKI.play(_tick);
        case State::CLEANING:
            return JARE.play(_tick);
        case State::SLEEP:
            return SLEEP.play(_tick);
        case State::SURPRISED:
            return AWAKE.play(_tick);
        case State::WANDERING:
            return WALK(_destination - _position).play(_tick);
        case State::YAWNING:
            return AKUBI.play(_tick);
        }
    }
};

struct Handler : App::Handler {
    Rc<App::Window> _window;
    Rc<App::SwapChain> _swapChain;
    Neko _neko;
    Math::Vec2f _mouse;
    Duration _counter;

    Handler(Rc<App::Window> window)
        : _window(window),
          _swapChain(window->createSwapChain().expect()) {
    }

    void handle(App::WindowId, App::Event& event) override {
        if (event.is<App::ResizeEvent>())
            _swapChain = _window->createSwapChain().expect();
        else if (auto e = event.is<App::MouseEvent>())
            _mouse = e->pos.cast<f64>();
    }

    void update() override {
        _counter += Duration::fromMSecs(1000 / 60);
        if (_counter >= Neko::TICK_DURATION) {
            _counter -= Neko::TICK_DURATION;
            _neko.update(_mouse);

            auto [drmBuffer, _] = _swapChain->acquire();
            Gfx::CpuCanvas g;
            g.begin(Gfx::MutPixels::from(drmBuffer));
            g.scale(_window->scaleFactor());
            g.clear(Gfx::ALPHA);
            _neko.paint(g);
            _swapChain->present(drmBuffer);
        }
    }
};

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(env, {}, ct));
    auto win = co_trya$(app->createWindowAsync({}, ct));
    co_return co_await app->runAsync(makeRc<Handler>(win), ct);
}
