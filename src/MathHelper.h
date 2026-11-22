#pragma once
#include <cmath>
#include <cstdint>

// Port of the parts of the original MathHelper that Bird needs:
// the integer-degree sine table and the seeded random generator.
namespace MathHelper {

namespace detail {
struct Tables {
    float sinT[360];
    Tables() {
        for (int i = 0; i < 360; i++) {
            float f = (i * 3.1415925f) / 180.0f;   // the original's slightly-off pi
            sinT[i] = static_cast<float>(std::sin(static_cast<double>(f)));
        }
    }
};
inline const Tables& tables() {
    static const Tables t;
    return t;
}
inline int32_t seed1 = 0;
inline int32_t seed2 = 0;
}  // namespace detail

inline float getSin(float angle) {
    return detail::tables().sinT[static_cast<int>(angle)];
}

inline void setSeed(int32_t seed) {
    detail::seed1 = seed % 32000;
    detail::seed2 = seed % 65535;
}

// Same arithmetic as the Java version, including int32 wraparound.
inline int nextRandom() {
    using detail::seed1;
    using detail::seed2;
    seed2 = static_cast<int32_t>(36969u * static_cast<uint32_t>(seed2 & 65535) +
                                 static_cast<uint32_t>(seed2 >> 16));
    seed1 = static_cast<int32_t>(static_cast<uint32_t>(seed1 & 65535) * 18000u +
                                 static_cast<uint32_t>(seed1 >> 16));
    int32_t r = static_cast<int32_t>((static_cast<uint32_t>(seed2) << 16) +
                                     static_cast<uint32_t>(seed1));
    return r < 0 ? static_cast<int32_t>(0u - static_cast<uint32_t>(r)) : r;
}

// Original randomRange: result is in [min, max).
inline int randomRange(int min, int max) {
    return (nextRandom() % (max - min)) + min;
}

}  // namespace MathHelper
