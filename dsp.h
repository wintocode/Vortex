#pragma once

#include <cmath>
#include <cstdint>

namespace vortex {

// --- Constants ---
// (Replacing C++20 std::numbers with C++11 constexpr)

static const float PI       = 3.14159265358979323846f;
static const float TWO_PI   = 6.28318530717958647692f;
static const float SQRT2    = 1.41421356237309504880f;
static const float INV_PI   = 0.31830988618379067154f;
static const float INV_SQRT2 = 0.70710678118654752440f;

// --- Utility functions ---

// Flush denormals to zero (prevents FPU slowdown on ARM)
inline float flush_denormal(float x)
{
    union { float f; uint32_t i; } u;
    u.f = x;
    if ((u.i & 0x7F800000) == 0 && (u.i & 0x007FFFFF) != 0)
        u.f = 0.0f;
    return u.f;
}

// Soft-clip saturation: x*(27+x^2)/(27+9x^2)
// Smooth saturator approaching +/-1/3 at extremes
inline float soft_clip(float x)
{
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// MIDI note to frequency (note 69 = A4 = 440 Hz)
inline float midi_note_to_freq(float note)
{
    return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
}

// V/OCT to frequency (0V = C4 = 261.63 Hz)
inline float voct_to_freq(float voltage)
{
    return 261.6255653f * powf(2.0f, voltage);
}

// V/OCT to frequency multiplier (0V = 1x, 1V = 2x)
inline float voct_to_mult(float voltage)
{
    return powf(2.0f, voltage);
}

// Cutoff parameter (0-1000) to Hz (20-20000, exponential)
// freq = 20 * 1000^(param/1000)
inline float cutoff_param_to_hz(int param)
{
    return 20.0f * powf(1000.0f, (float)param / 1000.0f);
}

// Resonance parameter (0-1000) to damping factor
// 0 = Butterworth (damping=0.707), 1000 = near self-oscillation (damping=0.01)
inline float resonance_to_damping(int param)
{
    float t = (float)param / 1000.0f;
    return 0.707f * (1.0f - t) + 0.01f * t;
}

} // namespace vortex
