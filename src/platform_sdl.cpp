#include "platform_sdl.h"
#include <iostream>

PlatformSDL::~PlatformSDL() {
    if (dev) SDL_CloseAudioDevice(dev);
    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

bool PlatformSDL::init(const char* title, int scale, bool vsync) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n"; return false;
    }

    int winW = Chip8::W * scale, winH = Chip8::H * scale;
    Uint32 wflags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH, wflags);
    if (!win) { std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n"; return false; }

    Uint32 rflags = SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
    ren = SDL_CreateRenderer(win, -1, rflags);
    if (!ren) { std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n"; return false; }

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Chip8::W, Chip8::H);
    if (!tex) { std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n"; return false; }

    SDL_AudioSpec want{}; want.freq=audio.sample_rate; want.format=AUDIO_F32SYS; want.channels=1; want.samples=1024; want.callback=audio_cb; want.userdata=&audio;
    SDL_AudioSpec have{}; dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!dev) { std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << "\n"; }
    else { SDL_PauseAudioDevice(dev, 0); }

    return true;
}

void PlatformSDL::audio_cb(void* userdata, Uint8* stream, int len) {
    auto* st = reinterpret_cast<AudioState*>(userdata);
    auto* out = reinterpret_cast<float*>(stream);
    int frames = len / (int)sizeof(float);
    double step = st->freq / st->sample_rate;
    for (int i = 0; i < frames; ++i) {
        float s = 0.0f;
        if (st->beep.load(std::memory_order_relaxed)) {
            s = (st->phase < 0.5 ? 0.25f : -0.25f);
            st->phase += step; if (st->phase >= 1.0) st->phase -= 1.0;
        }
        out[i] = s;
    }
}

int PlatformSDL::map_key(SDL_Keycode kc) {
    switch (kc) {
        case SDLK_x: return 0x0; case SDLK_1: return 0x1; case SDLK_2: return 0x2; case SDLK_3: return 0x3;
        case SDLK_q: return 0x4; case SDLK_w: return 0x5; case SDLK_e: return 0x6; case SDLK_a: return 0x7;
        case SDLK_s: return 0x8; case SDLK_d: return 0x9; case SDLK_z: return 0xA; case SDLK_c: return 0xB;
        case SDLK_4: return 0xC; case SDLK_r: return 0xD; case SDLK_f: return 0xE; case SDLK_v: return 0xF;
        default: return -1;
    }
}

bool PlatformSDL::handle_events(Chip8& chip, bool& running) {
    SDL_Event e; 
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { running = false; }
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            int k = map_key(e.key.keysym.sym);
            if (k >= 0) chip.set_key(k, e.type == SDL_KEYDOWN);
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }
    }
    return running;
}

void PlatformSDL::draw_frame(const std::array<u8, Chip8::W * Chip8::H>& gfx) {
    void* pixels = nullptr; int pitch = 0;
    SDL_LockTexture(tex, nullptr, &pixels, &pitch);
    auto* p = static_cast<u32*>(pixels);
    for (int y = 0; y < Chip8::H; ++y) {
        for (int x = 0; x < Chip8::W; ++x) {
            u8 v = gfx[(size_t)(y*Chip8::W + x)];
            p[(size_t)(y*Chip8::W + x)] = v ? 0xFFFFFFFFu : 0xFF000000u;
        }
    }
    SDL_UnlockTexture(tex);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
}

void PlatformSDL::set_beep(bool on) {
    audio.beep.store(on, std::memory_order_relaxed);
}
