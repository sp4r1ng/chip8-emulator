#include "chip8.h"
#include <cstring>
#include <fstream>
#include <vector>

namespace {
static const u8 FONT[80] = {
    0xF0,0x90,0x90,0x90,0xF0, 0x20,0x60,0x20,0x20,0x70,
    0xF0,0x10,0xF0,0x80,0xF0, 0xF0,0x10,0xF0,0x10,0xF0,
    0x90,0x90,0xF0,0x10,0x10, 0xF0,0x80,0xF0,0x10,0xF0,
    0xF0,0x80,0xF0,0x90,0xF0, 0xF0,0x10,0x20,0x40,0x40,
    0xF0,0x90,0xF0,0x90,0xF0, 0xF0,0x90,0xF0,0x10,0xF0,
    0xF0,0x90,0xF0,0x90,0x90, 0xE0,0x90,0xE0,0x90,0xE0,
    0xF0,0x80,0x80,0x80,0xF0, 0xE0,0x90,0x90,0x90,0xE0,
    0xF0,0x80,0xF0,0x80,0xF0, 0xF0,0x80,0xF0,0x80,0x80
};
}

void Chip8::reset() {
    mem.fill(0); V.fill(0); stack.fill(0); gfx.fill(0); keys.fill(0);
    I = 0; pc = 0x200; sp = 0; delay = 0; sound = 0; wait_key_reg = -1; draw_flag = true;
    std::memcpy(mem.data() + 0x50, FONT, sizeof(FONT));
}

bool Chip8::load_rom(const std::string& path) {
    reset();
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<char> buf((std::istreambuf_iterator<char>(f)), {});
    if (buf.size() > mem.size() - 0x200) return false;
    std::memcpy(mem.data() + 0x200, buf.data(), buf.size());
    return true;
}

void Chip8::set_key(int k, bool down) {
    if (k >= 0 && k < 16) keys[(size_t)k] = down ? 1 : 0;
}

void Chip8::step_cycle() {
    if (wait_key_reg >= 0) {
        for (int k = 0; k < 16; ++k) if (keys[(size_t)k]) { V[(size_t)wait_key_reg] = (u8)k; wait_key_reg = -1; break; }
        if (wait_key_reg >= 0) return;
    }

    u16 op = (u16)((mem[(size_t)pc] << 8) | mem[(size_t)pc + 1]);
    pc = (u16)(pc + 2);

    switch (op & 0xF000) {
        case 0x0000: {
            switch (op) {
                case 0x00E0: gfx.fill(0); draw_flag = true; break;
                case 0x00EE: pc = stack[(size_t)--sp]; break;
                default: break; // 0NNN ignored
            }
        } break;
        case 0x1000: pc = (u16)(op & 0x0FFF); break;
        case 0x2000: stack[(size_t)sp++] = pc; pc = (u16)(op & 0x0FFF); break;
        case 0x3000: if (V[(op>>8)&0xF] == (u8)(op & 0xFF)) pc = (u16)(pc + 2); break;
        case 0x4000: if (V[(op>>8)&0xF] != (u8)(op & 0xFF)) pc = (u16)(pc + 2); break;
        case 0x5000: if (((op & 0xF) == 0) && (V[(op>>8)&0xF] == V[(op>>4)&0xF])) pc = (u16)(pc + 2); break;
        case 0x6000: V[(op>>8)&0xF] = (u8)(op & 0xFF); break;
        case 0x7000: V[(op>>8)&0xF] = (u8)(V[(op>>8)&0xF] + (u8)(op & 0xFF)); break;
        case 0x8000: {
            u8 x = (u8)((op>>8)&0xF), y = (u8)((op>>4)&0xF);
            switch (op & 0xF) {
                case 0x0: V[x] = V[y]; break;
                case 0x1: V[x] |= V[y]; break;
                case 0x2: V[x] &= V[y]; break;
                case 0x3: V[x] ^= V[y]; break;
                case 0x4: { u16 s = (u16)(V[x] + V[y]); V[0xF] = (u8)(s > 0xFF); V[x] = (u8)s; } break;
                case 0x5: { V[0xF] = (u8)(V[x] > V[y]); V[x] = (u8)(V[x] - V[y]); } break;
                case 0x6: { V[0xF] = (u8)(V[x] & 1u); V[x] = (u8)(V[x] >> 1); } break;
                case 0x7: { V[0xF] = (u8)(V[y] > V[x]); V[x] = (u8)(V[y] - V[x]); } break;
                case 0xE: { V[0xF] = (u8)((V[x] & 0x80u) != 0); V[x] = (u8)(V[x] << 1); } break;
            }
        } break;
        case 0x9000: if (((op & 0xF) == 0) && (V[(op>>8)&0xF] != V[(op>>4)&0xF])) pc = (u16)(pc + 2); break;
        case 0xA000: I = (u16)(op & 0x0FFF); break;
        case 0xB000: pc = (u16)((op & 0x0FFF) + V[0]); break;
        case 0xC000: V[(op>>8)&0xF] = (u8)(dist(rng) & (u8)(op & 0xFF)); break;
        case 0xD000: {
            u8 x = (u8)(V[(op>>8)&0xF] % W);
            u8 y = (u8)(V[(op>>4)&0xF] % H);
            u8 n = (u8)(op & 0xF);
            V[0xF] = 0;
            for (u8 row = 0; row < n; ++row) {
                u8 sprite = mem[(size_t)(I + row)];
                u8 py = (u8)((y + row) % H);
                for (u8 col = 0; col < 8; ++col) {
                    if (sprite & (u8)(0x80 >> col)) {
                        u16 idx = (u16)(py * W + ((x + col) % W));
                        V[0xF] = (u8)(V[0xF] | gfx[idx]);
                        gfx[idx] ^= 1;
                    }
                }
            }
            draw_flag = true;
        } break;
        case 0xE000: {
            u8 x = (u8)((op>>8)&0xF);
            if ((op & 0xFF) == 0x9E) { if (keys[V[x]]) pc = (u16)(pc + 2); }
            else if ((op & 0xFF) == 0xA1) { if (!keys[V[x]]) pc = (u16)(pc + 2); }
        } break;
        case 0xF000: {
            u8 x = (u8)((op>>8)&0xF);
            switch (op & 0xFF) {
                case 0x07: V[x] = delay; break;
                case 0x0A: wait_key_reg = x; break;
                case 0x15: delay = V[x]; break;
                case 0x18: sound = V[x]; break;
                case 0x1E: I = (u16)(I + V[x]); break;
                case 0x29: I = (u16)(0x50 + (V[x] & 0xF) * 5); break;
                case 0x33: { u8 v = V[x]; mem[(size_t)I] = (u8)(v/100); mem[(size_t)I+1] = (u8)((v/10)%10); mem[(size_t)I+2] = (u8)(v%10); } break;
                case 0x55: for (u8 i=0;i<=x;++i) mem[(size_t)I++] = V[i]; break; // I increments (original)
                case 0x65: for (u8 i=0;i<=x;++i) V[i] = mem[(size_t)I++]; break; // I increments
            }
        } break;
    }
}

void Chip8::tick_timers() {
    if (delay > 0) --delay;
    if (sound > 0) --sound;
}
