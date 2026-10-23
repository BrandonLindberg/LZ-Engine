#pragma once

#include <cstdint>

// splitmix64: small, fast, and identical on every platform (unlike std distributions)
class Rng {
public:
    explicit Rng(uint64_t seed) : state(seed) {}

    uint64_t Next() {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Inclusive range
    int Range(int lo, int hi) {
        if (hi <= lo) return lo;
        return lo + static_cast<int>(Next() % static_cast<uint64_t>(hi - lo + 1));
    }

private:
    uint64_t state;
};
