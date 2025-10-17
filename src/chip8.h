#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <string>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

struct Chip8 {
    static constexpr int W = 64;
    static constexpr int H = 32;

    std::array<u8, 4096> mem{};
    std::array<u8, 16>   V{};
    u16 I = 0;
    u16 pc = 0x200;

    std::array<u16, 16> stack{};
    u8 sp = 0;

    std::array<u8, W * H> gfx{}; // 0/1
    std::array<u8, 16>    keys{}; // 0/1

    u8 delay = 0;
    u8 sound = 0;

    int  wait_key_reg = -1;
    bool draw_flag    = false;

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist{0, 255};

    void reset();
    bool load_rom(const std::string& path);
    void set_key(int k, bool down);

    void step_cycle();
    void tick_timers();
};
