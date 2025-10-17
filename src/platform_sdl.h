#pragma once
#include <SDL2/SDL.h>
#include <atomic>
#include <string>
#include <array>
#include "chip8.h"

struct PlatformSDL {
    PlatformSDL() = default;
    ~PlatformSDL();

    bool init(const char* title, int scale, bool vsync);
    bool handle_events(Chip8& chip, bool& running);
    void draw_frame(const std::array<u8, Chip8::W * Chip8::H>& gfx);
    void set_beep(bool on);

private:
    SDL_Window*   win = nullptr;
    SDL_Renderer* ren = nullptr;
    SDL_Texture*  tex = nullptr;
    SDL_AudioDeviceID dev = 0;

    struct AudioState { std::atomic<bool> beep{false}; double phase=0.0; double freq=440.0; int sample_rate=48000; } audio;

    static void audio_cb(void* userdata, Uint8* stream, int len);
    static int  map_key(SDL_Keycode kc);
};
