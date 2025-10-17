#include "chip8.h"
#include "platform_sdl.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

struct Args { std::string rom; int scale = 12; int hz = 700; bool vsync = false; };

static Args parse_args(int argc, char** argv) {
    Args a{}; if (argc >= 2) a.rom = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string t = argv[i];
        auto next = [&](int& val){ if (i+1<argc) val = std::max(1, std::atoi(argv[++i])); };
        if (t == "--scale") next(a.scale);
        else if (t == "--hz") next(a.hz);
        else if (t == "--vsync") a.vsync = true;
    }
    return a;
}

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);
    if (args.rom.empty()) {
        std::cerr << "Usage: " << argv[0] << " <rom> [--scale N] [--hz N] [--vsync]\n";
        return 1;
    }

    Chip8 chip;
    if (!chip.load_rom(args.rom)) { std::cerr << "Failed to load ROM\n"; return 2; }

    PlatformSDL plat;
    if (!plat.init("Chip-8 — sp4r1ng", args.scale, args.vsync)) return 3;

    const double cycles_per_sec = (double)args.hz;
    const double ns_per_cycle   = 1e9 / cycles_per_sec;
    const double ns_per_timer   = 1e9 / 60.0;

    auto now = []{ return std::chrono::high_resolution_clock::now(); };
    auto t_prev = now();
    double acc_ns = 0.0, timer_acc = 0.0;
    bool running = true;

    while (running) {
        plat.handle_events(chip, running);

        auto t_now = now();
        double dt_ns = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(t_now - t_prev).count();
        t_prev = t_now;
        acc_ns += dt_ns; timer_acc += dt_ns;

        while (acc_ns >= ns_per_cycle) { chip.step_cycle(); acc_ns -= ns_per_cycle; }
        while (timer_acc >= ns_per_timer) { chip.tick_timers(); timer_acc -= ns_per_timer; }

        plat.set_beep(chip.sound > 0);
        if (chip.draw_flag) { chip.draw_flag = false; plat.draw_frame(chip.gfx); }
    }

    return 0;
}
