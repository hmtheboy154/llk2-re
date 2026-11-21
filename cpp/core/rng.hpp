#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace llk2::core {

// Exact MSVBVM50.DLL linear congruential generator (rtcRandomNext / rtcRandomize).
// Default initial state in VB5 runtime is 0x00050000.
class Vb5Rng {
public:
    explicit Vb5Rng(uint32_t initial_seed = 0x00050000u) noexcept
        : seed_(initial_seed & 0x00FFFFFFu) {}

    // Equivalent to VB5 `Rnd()` (with positive or omitted argument).
    float next_float() noexcept {
        seed_ = (seed_ * 0x43FD43FDu + 0x00C39EC3u) & 0x00FFFFFFu;
        return static_cast<float>(seed_) / 16777216.0f; // 2^24
    }

    // Equivalent to VB5 `Int(Rnd() * upper_bound)`.
    int next_int(int upper_bound) noexcept {
        if (upper_bound <= 0) {
            return 0;
        }
        float v = next_float() * static_cast<float>(upper_bound);
        int r = static_cast<int>(std::floor(v));
        if (r >= upper_bound) {
            r = upper_bound - 1;
        }
        return r;
    }

    // Equivalent to VB5 `Randomize <number>` (rtcRandomize).
    void randomize(double number) noexcept {
        uint64_t bits = 0;
        std::memcpy(&bits, &number, sizeof(bits));
        uint32_t hi = static_cast<uint32_t>(bits >> 32);
        uint16_t x = static_cast<uint16_t>((hi & 0xFFFFu) ^ (hi >> 16));
        seed_ = (seed_ & 0x000000FFu) | (static_cast<uint32_t>(x) << 8);
    }

    uint32_t state() const noexcept { return seed_; }
    void set_state(uint32_t s) noexcept { seed_ = s & 0x00FFFFFFu; }

private:
    uint32_t seed_;
};

} // namespace llk2::core
