# Vortex Multi-mode Filter Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a mono multi-mode filter plugin for Disting NT using ported ivantsov-filters DSP (by Yuriy Ivantsov).

**Architecture:** C++11 plugin with `dsp.h` (portable filter math ported from ivantsov-filters C++20) and `vortex.cpp` (NT integration). 7 filter modes with 2/4-pole cascading, pre-filter drive, MIDI keyboard tracking, 8 CV inputs.

**Tech Stack:** ARM Cortex-M7, C++11, Expert Sleepers Disting NT API

**Reference projects:** `/Volumes/CODE/meld` and `/Volumes/CODE/four` use identical patterns.

**Filter credit:** DSP based on [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy Ivantsov.

---

### Task 1: Project Scaffold

**Files:**
- Create: `Makefile`
- Create: `VERSION`
- Create: `LICENSE`
- Create: `.gitignore`
- Create: `plugin.json`
- Create: `tests/Makefile`

**Step 1: Create all scaffold files**

`Makefile`:
```makefile
NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := vortex.cpp
OUTPUT := plugins/vortex.o
MANIFEST := plugins/plugin.json
VERSION := $(shell cat VERSION)

CC := arm-none-eabi-c++
CFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
          -mthumb -fno-rtti -fno-exceptions -Os -fPIC -Wall \
          -I$(INCLUDE_PATH) \
          -DVORTEX_VERSION='"$(VERSION)"'

all: $(OUTPUT) $(MANIFEST)

$(OUTPUT): $(SRC) dsp.h VERSION
	mkdir -p plugins
	$(CC) $(CFLAGS) -c -o $@ $<

$(MANIFEST): VERSION
	mkdir -p plugins
	@echo '{"name": "Vortex", "guid": "Vrtx", "version": "$(VERSION)", "author": "wintoid", "description": "Vortex - multi-mode filter for Disting NT", "tags": ["effect", "filter"]}' > $@

clean:
	rm -f $(OUTPUT) $(MANIFEST)

.PHONY: all clean
```

`VERSION`:
```
0.1.0
```

`LICENSE`: MIT license, `Copyright (c) 2026 wintocode`

`.gitignore`:
```
plugins/*.o
plugins/plugin.json
tests/test_dsp
tests/*.dSYM/
.DS_Store
```

`plugin.json`:
```json
{"name": "Vortex", "guid": "Vrtx", "version": "0.1.0", "author": "wintoid", "description": "Vortex - multi-mode filter for Disting NT", "tags": ["effect", "filter"]}
```

`tests/Makefile`:
```makefile
CC := c++
CFLAGS := -std=c++11 -Wall -Wextra -g -fsanitize=address,undefined
SRC := test_dsp.cpp
OUTPUT := test_dsp

all: $(OUTPUT)

$(OUTPUT): $(SRC) ../dsp.h
	$(CC) $(CFLAGS) -o $@ $< -lm

run: $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)
	rm -rf $(OUTPUT).dSYM

.PHONY: all run clean
```

**Step 2: Add distingNT_API submodule**

Run: `git submodule add https://github.com/expertsleepersltd/distingNT_API.git`

**Step 3: Commit**

```bash
git add Makefile VERSION LICENSE .gitignore plugin.json tests/Makefile .gitmodules distingNT_API
git commit -m "Project scaffold: Makefile, submodule, test infrastructure"
```

---

### Task 2: DSP Constants and Utilities with Tests

**Files:**
- Create: `dsp.h`
- Create: `tests/test_dsp.cpp`

**Step 1: Write test skeleton and utility tests**

Create `tests/test_dsp.cpp`:
```cpp
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Test macros (same pattern as four)
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void test_##name(); \
    static void run_##name() { \
        tests_run++; \
        printf("  %s ... ", #name); \
        test_##name(); \
        tests_passed++; \
        printf("PASS\n"); \
    } \
    static void test_##name()

#define ASSERT(cond) \
    do { if (!(cond)) { \
        printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } } while(0)

#define ASSERT_NEAR(a, b, eps) \
    do { float _a=(a), _b=(b); if (fabsf(_a-_b) > (eps)) { \
        printf("FAIL\n    %s:%d: %f != %f (eps=%f)\n", \
               __FILE__, __LINE__, (double)_a, (double)_b, (double)(eps)); \
        exit(1); \
    } } while(0)

#include "../dsp.h"

// --- Utility tests ---

TEST(soft_clip_zero)
{
    ASSERT_NEAR(vortex::soft_clip(0.0f), 0.0f, 1e-6f);
}

TEST(soft_clip_unity)
{
    // soft_clip(1) = 1*(27+1)/(27+9) = 28/36 = 0.7778
    ASSERT_NEAR(vortex::soft_clip(1.0f), 28.0f / 36.0f, 1e-4f);
}

TEST(soft_clip_symmetry)
{
    ASSERT_NEAR(vortex::soft_clip(-0.5f), -vortex::soft_clip(0.5f), 1e-6f);
}

TEST(soft_clip_saturation)
{
    // Large values should saturate toward 1/3
    float result = vortex::soft_clip(100.0f);
    ASSERT(result > 0.33f && result < 0.34f);
}

TEST(midi_note_to_freq_a4)
{
    ASSERT_NEAR(vortex::midi_note_to_freq(69.0f), 440.0f, 0.01f);
}

TEST(midi_note_to_freq_c4)
{
    ASSERT_NEAR(vortex::midi_note_to_freq(60.0f), 261.63f, 0.01f);
}

TEST(voct_to_freq_0v)
{
    // 0V = C4 = 261.63 Hz
    ASSERT_NEAR(vortex::voct_to_freq(0.0f), 261.63f, 0.01f);
}

TEST(voct_to_freq_1v)
{
    // 1V = C5 = 523.25 Hz
    ASSERT_NEAR(vortex::voct_to_freq(1.0f), 523.25f, 0.1f);
}

TEST(voct_to_mult_zero)
{
    ASSERT_NEAR(vortex::voct_to_mult(0.0f), 1.0f, 1e-6f);
}

TEST(voct_to_mult_one)
{
    ASSERT_NEAR(vortex::voct_to_mult(1.0f), 2.0f, 1e-6f);
}

TEST(flush_denormal_normal)
{
    ASSERT_NEAR(vortex::flush_denormal(1.0f), 1.0f, 1e-6f);
}

TEST(flush_denormal_zero)
{
    ASSERT_NEAR(vortex::flush_denormal(0.0f), 0.0f, 1e-6f);
}

TEST(cutoff_param_to_hz_min)
{
    // param 0 -> 20 Hz
    ASSERT_NEAR(vortex::cutoff_param_to_hz(0), 20.0f, 0.1f);
}

TEST(cutoff_param_to_hz_mid)
{
    // param 500 -> ~632 Hz (20 * 1000^0.5)
    ASSERT_NEAR(vortex::cutoff_param_to_hz(500), 632.46f, 1.0f);
}

TEST(cutoff_param_to_hz_max)
{
    // param 1000 -> 20000 Hz
    ASSERT_NEAR(vortex::cutoff_param_to_hz(1000), 20000.0f, 1.0f);
}

TEST(resonance_to_damping_zero)
{
    // 0% resonance = Butterworth damping (0.707)
    ASSERT_NEAR(vortex::resonance_to_damping(0), 0.707f, 0.001f);
}

TEST(resonance_to_damping_max)
{
    // 100% resonance = near self-oscillation
    float d = vortex::resonance_to_damping(1000);
    ASSERT(d > 0.0f && d < 0.02f);
}

int main()
{
    printf("Vortex DSP Tests\n");
    printf("================\n\n");

    printf("Utilities:\n");
    run_soft_clip_zero();
    run_soft_clip_unity();
    run_soft_clip_symmetry();
    run_soft_clip_saturation();
    run_midi_note_to_freq_a4();
    run_midi_note_to_freq_c4();
    run_voct_to_freq_0v();
    run_voct_to_freq_1v();
    run_voct_to_mult_zero();
    run_voct_to_mult_one();
    run_flush_denormal_normal();
    run_flush_denormal_zero();
    run_cutoff_param_to_hz_min();
    run_cutoff_param_to_hz_mid();
    run_cutoff_param_to_hz_max();
    run_resonance_to_damping_zero();
    run_resonance_to_damping_max();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
```

**Step 2: Run tests — expect FAIL (dsp.h doesn't exist yet)**

Run: `cd tests && make run`
Expected: Compilation error (missing dsp.h)

**Step 3: Implement dsp.h constants and utilities**

Create `dsp.h`:
```cpp
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
```

**Step 4: Run tests — expect PASS**

Run: `cd tests && make run`
Expected: All 17 tests PASS

**Step 5: Commit**

```bash
git add dsp.h tests/test_dsp.cpp
git commit -m "DSP constants and utility functions with tests"
```

---

### Task 3: First-Order Filter with Tests

**Files:**
- Modify: `dsp.h` (append filter structs)
- Modify: `tests/test_dsp.cpp` (add filter tests)

**Step 1: Add first-order filter tests to `tests/test_dsp.cpp`**

Add before `main()`:
```cpp
// --- First-order filter tests ---

TEST(filter1_lp_passes_dc)
{
    // Low-pass should pass DC (1.0 in -> 1.0 out after settling)
    vortex::Filter1 f;
    vortex::filter1_configure_lp(f, 48000.0f, 1000.0f);
    float out = 0.0f;
    for (int i = 0; i < 4800; i++)  // 100ms settling
        out = f.process_lp(1.0f);
    ASSERT_NEAR(out, 1.0f, 0.001f);
}

TEST(filter1_hp_blocks_dc)
{
    // High-pass should block DC (1.0 in -> 0.0 out after settling)
    vortex::Filter1 f;
    vortex::filter1_configure_hp(f, 48000.0f, 1000.0f);
    float out = 1.0f;
    for (int i = 0; i < 4800; i++)
        out = f.process_hp(1.0f);
    ASSERT_NEAR(out, 0.0f, 0.001f);
}

TEST(filter1_lp_attenuates_high_freq)
{
    // LP at 100Hz should significantly attenuate a 10kHz sine
    vortex::Filter1 f;
    vortex::filter1_configure_lp(f, 48000.0f, 100.0f);

    // Run 10kHz sine for 480 samples (10ms), measure last cycle amplitude
    float maxOut = 0.0f;
    for (int i = 0; i < 4800; i++) {
        float in = sinf(2.0f * vortex::PI * 10000.0f * (float)i / 48000.0f);
        float out = f.process_lp(in);
        if (i > 4320)  // last 10% (settled)
            if (fabsf(out) > maxOut) maxOut = fabsf(out);
    }
    // Should be well attenuated (< -20dB)
    ASSERT(maxOut < 0.1f);
}

TEST(filter1_hp_attenuates_low_freq)
{
    // HP at 5kHz should significantly attenuate a 100Hz sine
    vortex::Filter1 f;
    vortex::filter1_configure_hp(f, 48000.0f, 5000.0f);

    float maxOut = 0.0f;
    for (int i = 0; i < 4800; i++) {
        float in = sinf(2.0f * vortex::PI * 100.0f * (float)i / 48000.0f);
        float out = f.process_hp(in);
        if (i > 4320)
            if (fabsf(out) > maxOut) maxOut = fabsf(out);
    }
    ASSERT(maxOut < 0.1f);
}

TEST(filter1_reset)
{
    vortex::Filter1 f;
    vortex::filter1_configure_lp(f, 48000.0f, 1000.0f);
    // Process some signal
    for (int i = 0; i < 100; i++) f.process_lp(1.0f);
    f.reset();
    ASSERT_NEAR(f.z, 0.0f, 1e-6f);
}
```

Add to `main()`:
```cpp
    printf("\nFirst-order filter:\n");
    run_filter1_lp_passes_dc();
    run_filter1_hp_blocks_dc();
    run_filter1_lp_attenuates_high_freq();
    run_filter1_hp_attenuates_low_freq();
    run_filter1_reset();
```

**Step 2: Run tests — expect FAIL**

Run: `cd tests && make run`
Expected: Compilation error (Filter1 not defined)

**Step 3: Implement Filter1 in `dsp.h`**

Append to `dsp.h` before the closing `}` of namespace:
```cpp
// ============================================================
// First-order state-space filter (6 dB/oct)
// Ported from ivantsov-filters by Yuriy Ivantsov (C++20 -> C++11)
// Reference: https://github.com/yIvantsov/ivantsov-filters
// ============================================================

struct Filter1
{
    float z;        // state variable
    float b0, b1;   // coefficients

    Filter1() : z(0.0f), b0(0.0f), b1(0.0f) {}

    void reset() { z = 0.0f; }

    // Low-pass output: theta*b1 + z
    float process_lp(float x)
    {
        float theta = (x - z) * b0;
        float y = theta * b1 + z;
        z += theta;
        return y;
    }

    // High-pass output: theta*b1
    float process_hp(float x)
    {
        float theta = (x - z) * b0;
        float y = theta * b1;
        z += theta;
        return y;
    }
};

// Configure first-order low-pass coefficients
// Uses Sigma frequency warping for audio-rate modulation quality
inline void filter1_configure_lp(Filter1& f, float sample_rate, float cutoff_hz)
{
    float w = sample_rate / (2.0f * PI * cutoff_hz);
    float sigma = INV_PI;
    if (w > INV_PI)
        sigma = 0.40824999f * (0.05843357f - w * w) / (0.04593294f - w * w);
    float v = sqrtf(w * w + sigma * sigma);
    f.b0 = 1.0f / (0.5f + v);
    f.b1 = 0.5f + sigma;
}

// Configure first-order high-pass coefficients
inline void filter1_configure_hp(Filter1& f, float sample_rate, float cutoff_hz)
{
    float w = sample_rate / (2.0f * PI * cutoff_hz);
    float sigma = INV_PI;
    if (w > INV_PI)
        sigma = 0.40824999f * (0.05843357f - w * w) / (0.04593294f - w * w);
    float v = sqrtf(w * w + sigma * sigma);
    f.b0 = 1.0f / (0.5f + v);
    f.b1 = w;
}
```

**Step 4: Run tests — expect PASS**

Run: `cd tests && make run`
Expected: All 22 tests PASS

**Step 5: Commit**

```bash
git add dsp.h tests/test_dsp.cpp
git commit -m "First-order filter: LP and HP with sigma warping"
```

---

### Task 4: Second-Order Filter with Tests

**Files:**
- Modify: `dsp.h` (append Filter2)
- Modify: `tests/test_dsp.cpp` (add filter2 tests)

**Step 1: Add second-order filter tests to `tests/test_dsp.cpp`**

Add before `main()`:
```cpp
// --- Second-order filter tests ---

TEST(filter2_lp_passes_dc)
{
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 1000.0f, 0.707f, vortex::F2_LP);
    float out = 0.0f;
    for (int i = 0; i < 4800; i++)
        out = vortex::filter2_process(f, 1.0f, vortex::F2_LP);
    ASSERT_NEAR(out, 1.0f, 0.001f);
}

TEST(filter2_hp_blocks_dc)
{
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 1000.0f, 0.707f, vortex::F2_HP);
    float out = 1.0f;
    for (int i = 0; i < 4800; i++)
        out = vortex::filter2_process(f, 1.0f, vortex::F2_HP);
    ASSERT_NEAR(out, 0.0f, 0.001f);
}

TEST(filter2_bp_blocks_dc)
{
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 1000.0f, 0.707f, vortex::F2_BP);
    float out = 1.0f;
    for (int i = 0; i < 4800; i++)
        out = vortex::filter2_process(f, 1.0f, vortex::F2_BP);
    ASSERT_NEAR(out, 0.0f, 0.01f);
}

TEST(filter2_notch_passes_dc)
{
    // Notch passes DC (only rejects at center frequency)
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 1000.0f, 0.707f, vortex::F2_NOTCH);
    float out = 0.0f;
    for (int i = 0; i < 4800; i++)
        out = vortex::filter2_process(f, 1.0f, vortex::F2_NOTCH);
    ASSERT_NEAR(out, 1.0f, 0.001f);
}

TEST(filter2_ap_passes_dc)
{
    // Allpass passes DC (unity gain, only phase shifts)
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 1000.0f, 0.707f, vortex::F2_AP);
    float out = 0.0f;
    for (int i = 0; i < 4800; i++)
        out = vortex::filter2_process(f, 1.0f, vortex::F2_AP);
    ASSERT_NEAR(out, 1.0f, 0.001f);
}

TEST(filter2_lp_attenuates_high_freq)
{
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 100.0f, 0.707f, vortex::F2_LP);
    float maxOut = 0.0f;
    for (int i = 0; i < 4800; i++) {
        float in = sinf(2.0f * vortex::PI * 10000.0f * (float)i / 48000.0f);
        float out = vortex::filter2_process(f, in, vortex::F2_LP);
        if (i > 4320)
            if (fabsf(out) > maxOut) maxOut = fabsf(out);
    }
    // 12dB/oct should attenuate more than 6dB/oct
    ASSERT(maxOut < 0.01f);
}

TEST(filter2_resonance_peak)
{
    // High resonance (low damping) should create a peak at cutoff
    vortex::Filter2 f;
    float cutoff = 1000.0f;
    vortex::filter2_configure(f, 48000.0f, cutoff, 0.05f, vortex::F2_LP);

    // Feed sine at cutoff frequency, should resonate
    float maxOut = 0.0f;
    for (int i = 0; i < 9600; i++) {
        float in = sinf(2.0f * vortex::PI * cutoff * (float)i / 48000.0f) * 0.1f;
        float out = vortex::filter2_process(f, in, vortex::F2_LP);
        if (i > 4800)
            if (fabsf(out) > maxOut) maxOut = fabsf(out);
    }
    // Output should be boosted above input amplitude (0.1)
    ASSERT(maxOut > 0.2f);
}

TEST(filter2_cascade_steeper)
{
    // 4-pole (two cascaded 2nd-order) should attenuate more than single 2nd-order
    float fs = 48000.0f;
    float fc = 500.0f;
    float testFreq = 8000.0f;

    // Single stage
    vortex::Filter2 f1;
    vortex::filter2_configure(f1, fs, fc, 0.707f, vortex::F2_LP);
    float maxSingle = 0.0f;
    for (int i = 0; i < 4800; i++) {
        float in = sinf(2.0f * vortex::PI * testFreq * (float)i / fs);
        float out = vortex::filter2_process(f1, in, vortex::F2_LP);
        if (i > 4320)
            if (fabsf(out) > maxSingle) maxSingle = fabsf(out);
    }

    // Two cascaded stages
    vortex::Filter2 f2a, f2b;
    vortex::filter2_configure(f2a, fs, fc, 0.707f, vortex::F2_LP);
    vortex::filter2_configure(f2b, fs, fc, 0.707f, vortex::F2_LP);
    float maxCascade = 0.0f;
    for (int i = 0; i < 4800; i++) {
        float in = sinf(2.0f * vortex::PI * testFreq * (float)i / fs);
        float mid = vortex::filter2_process(f2a, in, vortex::F2_LP);
        float out = vortex::filter2_process(f2b, mid, vortex::F2_LP);
        if (i > 4320)
            if (fabsf(out) > maxCascade) maxCascade = fabsf(out);
    }

    // Cascade should be significantly more attenuated
    ASSERT(maxCascade < maxSingle * 0.5f);
}

TEST(filter2_reset)
{
    vortex::Filter2 f;
    vortex::filter2_configure(f, 48000.0f, 1000.0f, 0.707f, vortex::F2_LP);
    for (int i = 0; i < 100; i++) vortex::filter2_process(f, 1.0f, vortex::F2_LP);
    f.reset();
    ASSERT_NEAR(f.z0, 0.0f, 1e-6f);
    ASSERT_NEAR(f.z1, 0.0f, 1e-6f);
}
```

Add to `main()`:
```cpp
    printf("\nSecond-order filter:\n");
    run_filter2_lp_passes_dc();
    run_filter2_hp_blocks_dc();
    run_filter2_bp_blocks_dc();
    run_filter2_notch_passes_dc();
    run_filter2_ap_passes_dc();
    run_filter2_lp_attenuates_high_freq();
    run_filter2_resonance_peak();
    run_filter2_cascade_steeper();
    run_filter2_reset();
```

**Step 2: Run tests — expect FAIL**

Run: `cd tests && make run`
Expected: Compilation error (Filter2 not defined)

**Step 3: Implement Filter2 in `dsp.h`**

Append to `dsp.h` before the closing `}` of namespace:
```cpp
// ============================================================
// Second-order state-space filter (12 dB/oct)
// Ported from ivantsov-filters by Yuriy Ivantsov (C++20 -> C++11)
// Reference: https://github.com/yIvantsov/ivantsov-filters
// ============================================================

enum Filter2Type
{
    F2_LP = 0,   // Low-pass
    F2_HP,       // High-pass
    F2_BP,       // Band-pass
    F2_NOTCH,    // Notch (band reject)
    F2_AP        // All-pass
};

struct Filter2
{
    float z0, z1;           // state variables
    float b0, b1, b2, b3;   // coefficients

    Filter2() : z0(0.0f), z1(0.0f), b0(0.0f), b1(0.0f), b2(0.0f), b3(0.0f) {}

    void reset() { z0 = z1 = 0.0f; }

    // Process for LP, Notch, AllPass (output includes z0 term)
    float process_lna(float x)
    {
        float theta = (x - z0 - z1 * b1) * b0;
        float y = theta * b3 + z1 * b2 + z0;
        z0 += theta;
        z1 = -z1 - theta * b1;
        return y;
    }

    // Process for HP, BP (output excludes z0 term)
    float process_hb(float x)
    {
        float theta = (x - z0 - z1 * b1) * b0;
        float y = theta * b3 + z1 * b2;
        z0 += theta;
        z1 = -z1 - theta * b1;
        return y;
    }
};

// Configure second-order filter coefficients
// Uses Sigma frequency warping for audio-rate modulation quality
// damping = 1/(2*Q), e.g. 0.707 = Butterworth, lower = more resonant
inline void filter2_configure(Filter2& f, float sample_rate, float cutoff_hz,
                               float damping, Filter2Type type)
{
    float w = sample_rate / (SQRT2 * PI * cutoff_hz);
    float sigma = SQRT2 * INV_PI;
    if (w > INV_PI * SQRT2)
        sigma = 0.57735268f * (0.11686715f - w * w) / (0.09186588f - w * w);

    float w_sq = w * w;
    float sigma_sq = sigma * sigma;
    float zeta_sq = damping * damping;

    // vk computation (state-space eigenvalue decomposition)
    float t = w_sq * (2.0f * zeta_sq - 1.0f);
    float v = sqrtf(w_sq * w_sq + sigma_sq * (2.0f * t + sigma_sq));
    float k = t + sigma_sq;

    f.b0 = 1.0f / (v + sqrtf(v + k) + 0.5f);
    f.b1 = sqrtf(2.0f * v);

    switch (type)
    {
    case F2_LP:
        f.b2 = 2.0f * sigma_sq / f.b1;
        f.b3 = 0.5f + sigma_sq + SQRT2 * sigma;
        break;
    case F2_HP:
        f.b2 = 2.0f * w_sq / f.b1;
        f.b3 = w_sq;
        break;
    case F2_BP:
        f.b2 = 4.0f * w * damping * sigma / f.b1;
        f.b3 = 2.0f * w * damping * (sigma + INV_SQRT2);
        break;
    case F2_NOTCH:
        f.b2 = 2.0f * (w_sq - sigma_sq) / f.b1;
        f.b3 = 0.5f + w_sq - sigma_sq;
        break;
    case F2_AP:
        f.b2 = f.b1;
        f.b3 = 0.5f + v - sqrtf(v + k);
        break;
    }
}

// Process one sample through a second-order filter
inline float filter2_process(Filter2& f, float x, Filter2Type type)
{
    if (type == F2_HP || type == F2_BP)
        return f.process_hb(x);
    else
        return f.process_lna(x);
}
```

**Step 4: Run tests — expect PASS**

Run: `cd tests && make run`
Expected: All 31 tests PASS

**Step 5: Commit**

```bash
git add dsp.h tests/test_dsp.cpp
git commit -m "Second-order filter: LP/HP/BP/Notch/AP with sigma warping"
```

---

### Task 5: Plugin Implementation (vortex.cpp)

**Files:**
- Create: `vortex.cpp`

This is the Disting NT integration layer. It cannot be unit-tested locally (requires NT API runtime). Follow the exact patterns from meld.cpp and four.cpp.

**Step 1: Create `vortex.cpp`**

```cpp
#include <new>
#include <math.h>
#include <string.h>
#include <distingnt/api.h>
#include "dsp.h"

// --- Algorithm struct ---

struct _vortexAlgorithm : public _NT_algorithm
{
    // Filter state (two stages for 4-pole cascading)
    vortex::Filter1 f1a, f1b;   // first-order filters (LP6/HP6)
    vortex::Filter2 f2a, f2b;   // second-order filters (LP12/HP12/BP/Notch/AP)

    // Cached parameters (set by parameterChanged)
    int mode;             // 0-6: LP6, LP12, HP6, HP12, BP, Notch, AP
    float cutoffHz;       // 20-20000 Hz (from param + MIDI)
    float damping;        // resonance mapped to damping
    int poles;            // 0=2-pole, 1=4-pole
    float drive;          // 0.0-1.0
    float mix;            // 0.0-1.0
    float fmDepth;        // -1.0 to 1.0

    // MIDI state
    float midiCutoffHz;   // from note-on (keyboard tracking)
    uint8_t midiGate;     // 0 or 1
    uint8_t midiNote;     // last note number
    float pitchBendMult;  // from pitch bend wheel
    uint8_t midiChannel;  // 0-15

    float sampleRate;

    _vortexAlgorithm()
    {
        mode = 1;           // LP12
        cutoffHz = 632.0f;  // ~mid-range (param 500)
        damping = 0.707f;   // Butterworth
        poles = 0;          // 2-pole
        drive = 0.0f;
        mix = 1.0f;         // fully wet
        fmDepth = 0.0f;

        midiCutoffHz = 0.0f;
        midiGate = 0;
        midiNote = 60;
        pitchBendMult = 1.0f;
        midiChannel = 0;

        sampleRate = 48000.0f;
    }
};

// --- Parameter indices ---

enum {
    // I/O (3)
    kParamInput,
    kParamOutput,
    kParamOutputMode,

    // Filter (5)
    kParamMode,
    kParamCutoff,
    kParamResonance,
    kParamPoles,
    kParamDrive,

    // Global (3)
    kParamMix,
    kParamFMDepth,
    kParamMidiChannel,
    kParamVersion,

    // CV Inputs (7)
    kParamCVAudioIn,
    kParamCVCutoffVOCT,
    kParamCVCutoffFM,
    kParamCVResonance,
    kParamCVMode,
    kParamCVDrive,
    kParamCVMix,

    kNumParams
};

// --- Enum strings ---

static const char* modeStrings[] = {
    "LP 6dB", "LP 12dB", "HP 6dB", "HP 12dB", "Bandpass", "Notch", "Allpass", NULL
};
static const char* polesStrings[] = { "2-pole", "4-pole", NULL };
static const char* replMixStrings[] = { "Replace", "Mix", NULL };
static const char* versionStrings[] = { VORTEX_VERSION, NULL };

// --- Parameter definitions ---

static _NT_parameter parameters[] = {
    // I/O
    NT_PARAMETER_AUDIO_INPUT( "Input", 0, 0 )
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Output", 1, 1 )

    // Filter
    { "Mode",       0,    6,    1, kNT_unitEnum,       0, modeStrings },
    { "Cutoff",     0, 1000,  500, kNT_unitHasStrings, 0, NULL },
    { "Resonance",  0, 1000,    0, kNT_unitHasStrings, kNT_scaling10, NULL },
    { "Poles",      0,    1,    0, kNT_unitEnum,       0, polesStrings },
    { "Drive",      0, 1000,    0, kNT_unitPercent,    kNT_scaling10, NULL },

    // Global
    { "Mix",        0, 1000, 1000, kNT_unitPercent,    kNT_scaling10, NULL },
    { "FM Depth", -1000, 1000,  0, kNT_unitPercent,    kNT_scaling10, NULL },
    { "MIDI Channel", 1,  16,   1, kNT_unitNone,       0, NULL },

    // Version (read-only)
    { "Version",    0,    0,   0, kNT_unitEnum,        0, versionStrings },

    // CV Inputs
    NT_PARAMETER_AUDIO_INPUT( "Audio In CV",     0, 0 )
    NT_PARAMETER_CV_INPUT( "Cutoff V/OCT CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Cutoff FM CV",       0, 0 )
    NT_PARAMETER_CV_INPUT( "Resonance CV",       0, 0 )
    NT_PARAMETER_CV_INPUT( "Mode CV",            0, 0 )
    NT_PARAMETER_CV_INPUT( "Drive CV",           0, 0 )
    NT_PARAMETER_CV_INPUT( "Mix CV",             0, 0 )
};

// --- Parameter pages ---

static const uint8_t pageIO[] = { kParamInput, kParamOutput, kParamOutputMode };
static const uint8_t pageFilter[] = {
    kParamMode, kParamCutoff, kParamResonance, kParamPoles, kParamDrive
};
static const uint8_t pageGlobal[] = {
    kParamMix, kParamFMDepth, kParamVersion
};
static const uint8_t pageMIDI[] = { kParamMidiChannel };
static const uint8_t pageCV[] = {
    kParamCVAudioIn, kParamCVCutoffVOCT, kParamCVCutoffFM,
    kParamCVResonance, kParamCVMode, kParamCVDrive, kParamCVMix
};

static const _NT_parameterPage pages[] = {
    { .name = "I/O",    .numParams = ARRAY_SIZE(pageIO),      .params = pageIO },
    { .name = "Filter", .numParams = ARRAY_SIZE(pageFilter),  .params = pageFilter },
    { .name = "Global", .numParams = ARRAY_SIZE(pageGlobal),  .params = pageGlobal },
    { .name = "MIDI",   .numParams = ARRAY_SIZE(pageMIDI),    .params = pageMIDI },
    { .name = "CV",     .numParams = ARRAY_SIZE(pageCV),       .params = pageCV },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

// --- MIDI CC mapping ---

// CC14-22 -> 9 value parameters
static const int8_t ccToParam[128] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,                     // 0-13
    kParamMode, kParamCutoff, kParamResonance, kParamPoles,          // 14-17
    kParamDrive, kParamMix, kParamFMDepth, kParamMidiChannel,        // 18-21
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 22-37
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 38-53
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 54-69
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 70-85
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 86-101
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 102-117
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1                                   // 118-127
};

// Scale CC value (0-127) to parameter's min..max range
static int16_t scaleCCToParam( uint8_t ccValue, int paramIndex )
{
    int16_t mn = parameters[paramIndex].min;
    int16_t mx = parameters[paramIndex].max;
    return mn + (int16_t)( (int32_t)ccValue * ( mx - mn ) / 127 );
}

// --- Lifecycle ---

static void calculateRequirements(
    _NT_algorithmRequirements& req,
    const int32_t* specifications )
{
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof( _vortexAlgorithm );
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

static _NT_algorithm* construct(
    const _NT_algorithmMemoryPtrs& ptrs,
    const _NT_algorithmRequirements& req,
    const int32_t* specifications )
{
    _vortexAlgorithm* alg = new ( ptrs.sram ) _vortexAlgorithm();
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    return alg;
}

// --- Parameter changed ---

static void parameterChanged( _NT_algorithm* self, int parameter )
{
    _vortexAlgorithm* p = (_vortexAlgorithm*)self;

    switch ( parameter )
    {
    case kParamMode:
        p->mode = p->v[parameter];
        // Reset filter state when mode changes to avoid transients
        p->f1a.reset(); p->f1b.reset();
        p->f2a.reset(); p->f2b.reset();
        break;
    case kParamCutoff:
        p->cutoffHz = vortex::cutoff_param_to_hz( p->v[parameter] );
        break;
    case kParamResonance:
        p->damping = vortex::resonance_to_damping( p->v[parameter] );
        break;
    case kParamPoles:
        p->poles = p->v[parameter];
        break;
    case kParamDrive:
        p->drive = (float)p->v[parameter] * 0.001f;
        break;
    case kParamMix:
        p->mix = (float)p->v[parameter] * 0.001f;
        break;
    case kParamFMDepth:
        p->fmDepth = (float)p->v[parameter] * 0.001f;
        break;
    case kParamMidiChannel:
        p->midiChannel = p->v[parameter] - 1;  // 1-16 -> 0-15
        break;
    }
}

// --- Audio ---

static void step(
    _NT_algorithm* self,
    float* busFrames,
    int numFramesBy4 )
{
    _vortexAlgorithm* p = (_vortexAlgorithm*)self;
    int numFrames = numFramesBy4 * 4;

    p->sampleRate = (float)NT_globals.sampleRate;

    // Get I/O bus pointers
    const float* audioIn = p->v[kParamInput]
        ? busFrames + ( p->v[kParamInput] - 1 ) * numFrames : NULL;
    float* out = busFrames + ( p->v[kParamOutput] - 1 ) * numFrames;
    bool replace = p->v[kParamOutputMode] == 0;

    // Read CV buses (0 = not connected)
    const float* cvAudioIn = p->v[kParamCVAudioIn]
        ? busFrames + ( p->v[kParamCVAudioIn] - 1 ) * numFrames : NULL;
    const float* cvVOCT = p->v[kParamCVCutoffVOCT]
        ? busFrames + ( p->v[kParamCVCutoffVOCT] - 1 ) * numFrames : NULL;
    const float* cvFM = p->v[kParamCVCutoffFM]
        ? busFrames + ( p->v[kParamCVCutoffFM] - 1 ) * numFrames : NULL;
    const float* cvResonance = p->v[kParamCVResonance]
        ? busFrames + ( p->v[kParamCVResonance] - 1 ) * numFrames : NULL;
    const float* cvMode = p->v[kParamCVMode]
        ? busFrames + ( p->v[kParamCVMode] - 1 ) * numFrames : NULL;
    const float* cvDrive = p->v[kParamCVDrive]
        ? busFrames + ( p->v[kParamCVDrive] - 1 ) * numFrames : NULL;
    const float* cvMix = p->v[kParamCVMix]
        ? busFrames + ( p->v[kParamCVMix] - 1 ) * numFrames : NULL;

    float fs = p->sampleRate;

    for ( int i = 0; i < numFrames; ++i )
    {
        // --- Read input ---
        float input = 0.0f;
        if ( audioIn )
            input = audioIn[i];
        else if ( cvAudioIn )
            input = cvAudioIn[i];
        float dry = input;

        // --- Compute effective mode ---
        int mode = p->mode;
        if ( cvMode )
        {
            // CV mode: ±5V range, quantize to 0-6
            int modeOffset = (int)( cvMode[i] * 1.4f );  // ~5V = 7 steps
            mode = mode + modeOffset;
            if ( mode < 0 ) mode = 0;
            if ( mode > 6 ) mode = 6;
        }

        // --- Compute effective cutoff ---
        float cutoff = p->cutoffHz;

        // MIDI keyboard tracking overrides base cutoff when gate is on
        if ( p->midiGate && p->midiCutoffHz > 0.0f )
            cutoff = p->midiCutoffHz * p->pitchBendMult;

        // V/OCT modulation (exponential)
        if ( cvVOCT )
            cutoff *= vortex::voct_to_mult( cvVOCT[i] );

        // FM modulation (exponential, with attenuverter depth)
        if ( cvFM )
            cutoff *= vortex::voct_to_mult( cvFM[i] * p->fmDepth );

        // Clamp cutoff to safe range
        if ( cutoff < 20.0f ) cutoff = 20.0f;
        if ( cutoff > 20000.0f ) cutoff = 20000.0f;

        // --- Compute effective resonance/damping ---
        float damping = p->damping;
        if ( cvResonance )
        {
            // CV adds to resonance (reduces damping)
            float resoAdd = cvResonance[i] * 0.2f;  // ±5V -> ±1.0 damping range
            damping -= resoAdd;
            if ( damping < 0.01f ) damping = 0.01f;
            if ( damping > 0.707f ) damping = 0.707f;
        }

        // --- Compute effective drive ---
        float drv = p->drive;
        if ( cvDrive )
        {
            drv += cvDrive[i] * 0.2f;
            if ( drv < 0.0f ) drv = 0.0f;
            if ( drv > 1.0f ) drv = 1.0f;
        }

        // --- Compute effective mix ---
        float mix = p->mix;
        if ( cvMix )
        {
            mix += cvMix[i] * 0.2f;
            if ( mix < 0.0f ) mix = 0.0f;
            if ( mix > 1.0f ) mix = 1.0f;
        }

        // --- Apply drive (pre-filter saturation) ---
        float signal = input;
        if ( drv > 0.0f )
        {
            float driveGain = 1.0f + drv * 9.0f;  // 1x to 10x gain
            signal = vortex::soft_clip( signal * driveGain );
        }

        // --- Process through filter ---
        float wet = 0.0f;
        bool fourPole = ( p->poles == 1 );

        switch ( mode )
        {
        case 0: // LP 6dB (first-order LP)
            vortex::filter1_configure_lp( p->f1a, fs, cutoff );
            wet = p->f1a.process_lp( signal );
            if ( fourPole )
            {
                vortex::filter1_configure_lp( p->f1b, fs, cutoff );
                wet = p->f1b.process_lp( wet );
            }
            break;

        case 1: // LP 12dB (second-order LP)
            vortex::filter2_configure( p->f2a, fs, cutoff, damping, vortex::F2_LP );
            wet = vortex::filter2_process( p->f2a, signal, vortex::F2_LP );
            if ( fourPole )
            {
                vortex::filter2_configure( p->f2b, fs, cutoff, damping, vortex::F2_LP );
                wet = vortex::filter2_process( p->f2b, wet, vortex::F2_LP );
            }
            break;

        case 2: // HP 6dB (first-order HP)
            vortex::filter1_configure_hp( p->f1a, fs, cutoff );
            wet = p->f1a.process_hp( signal );
            if ( fourPole )
            {
                vortex::filter1_configure_hp( p->f1b, fs, cutoff );
                wet = p->f1b.process_hp( wet );
            }
            break;

        case 3: // HP 12dB (second-order HP)
            vortex::filter2_configure( p->f2a, fs, cutoff, damping, vortex::F2_HP );
            wet = vortex::filter2_process( p->f2a, signal, vortex::F2_HP );
            if ( fourPole )
            {
                vortex::filter2_configure( p->f2b, fs, cutoff, damping, vortex::F2_HP );
                wet = vortex::filter2_process( p->f2b, wet, vortex::F2_HP );
            }
            break;

        case 4: // Bandpass
            vortex::filter2_configure( p->f2a, fs, cutoff, damping, vortex::F2_BP );
            wet = vortex::filter2_process( p->f2a, signal, vortex::F2_BP );
            if ( fourPole )
            {
                vortex::filter2_configure( p->f2b, fs, cutoff, damping, vortex::F2_BP );
                wet = vortex::filter2_process( p->f2b, wet, vortex::F2_BP );
            }
            break;

        case 5: // Notch
            vortex::filter2_configure( p->f2a, fs, cutoff, damping, vortex::F2_NOTCH );
            wet = vortex::filter2_process( p->f2a, signal, vortex::F2_NOTCH );
            if ( fourPole )
            {
                vortex::filter2_configure( p->f2b, fs, cutoff, damping, vortex::F2_NOTCH );
                wet = vortex::filter2_process( p->f2b, wet, vortex::F2_NOTCH );
            }
            break;

        case 6: // Allpass
            vortex::filter2_configure( p->f2a, fs, cutoff, damping, vortex::F2_AP );
            wet = vortex::filter2_process( p->f2a, signal, vortex::F2_AP );
            if ( fourPole )
            {
                vortex::filter2_configure( p->f2b, fs, cutoff, damping, vortex::F2_AP );
                wet = vortex::filter2_process( p->f2b, wet, vortex::F2_AP );
            }
            break;
        }

        // Flush denormals from filter state
        p->f1a.z = vortex::flush_denormal( p->f1a.z );
        p->f1b.z = vortex::flush_denormal( p->f1b.z );
        p->f2a.z0 = vortex::flush_denormal( p->f2a.z0 );
        p->f2a.z1 = vortex::flush_denormal( p->f2a.z1 );
        p->f2b.z0 = vortex::flush_denormal( p->f2b.z0 );
        p->f2b.z1 = vortex::flush_denormal( p->f2b.z1 );

        // --- Dry/wet mix ---
        float result = dry * ( 1.0f - mix ) + wet * mix;

        // --- Write output ---
        if ( replace )
            out[i] = result;
        else
            out[i] += result;
    }
}

// --- MIDI ---

static void midiMessage(
    _NT_algorithm* self,
    uint8_t byte0,
    uint8_t byte1,
    uint8_t byte2 )
{
    _vortexAlgorithm* p = (_vortexAlgorithm*)self;

    uint8_t status  = byte0 & 0xF0;
    uint8_t channel = byte0 & 0x0F;

    if ( channel != p->midiChannel )
        return;

    switch ( status )
    {
    case 0x90:  // Note On
        if ( byte2 > 0 )
        {
            p->midiNote = byte1;
            p->midiGate = 1;
            p->midiCutoffHz = vortex::midi_note_to_freq( (float)byte1 );
        }
        else
        {
            if ( byte1 == p->midiNote )
                p->midiGate = 0;
        }
        break;

    case 0x80:  // Note Off
        if ( byte1 == p->midiNote )
            p->midiGate = 0;
        break;

    case 0xB0:  // Control Change
    {
        if ( byte1 >= 128 ) break;
        int8_t paramIdx = ccToParam[byte1];
        if ( paramIdx >= 0 )
        {
            int16_t value = scaleCCToParam( byte2, paramIdx );
            NT_setParameterFromAudio( NT_algorithmIndex(self), paramIdx, value );
        }
        break;
    }

    case 0xE0:  // Pitch Bend
    {
        int16_t bend = ( (int16_t)byte2 << 7 ) | byte1;  // 0-16383
        float bendNorm = (float)( bend - 8192 ) / 8192.0f;  // -1 to +1
        // +/-2 semitones pitch bend range (applied to cutoff)
        p->pitchBendMult = exp2f( bendNorm * 2.0f / 12.0f );
        break;
    }
    }
}

// --- Parameter string display ---

static int parameterString( _NT_algorithm* self, int param, int val, char* buff )
{
    // Cutoff: display as Hz
    if ( param == kParamCutoff )
    {
        float hz = vortex::cutoff_param_to_hz( val );
        int len;
        if ( hz >= 1000.0f )
        {
            // Display as kHz
            float khz = hz / 1000.0f;
            len = NT_floatToString( buff, khz, (khz < 10.0f) ? 2 : 1 );
            const char* suffix = " kHz";
            while ( *suffix ) buff[len++] = *suffix++;
        }
        else
        {
            len = NT_floatToString( buff, hz, (hz < 100.0f) ? 1 : 0 );
            const char* suffix = " Hz";
            while ( *suffix ) buff[len++] = *suffix++;
        }
        buff[len] = '\0';
        return len;
    }

    // Resonance: display as percentage
    if ( param == kParamResonance )
    {
        float pct = (float)val * 0.1f;
        int len = NT_floatToString( buff, pct, 1 );
        buff[len++] = '%';
        buff[len] = '\0';
        return len;
    }

    return 0;
}

// --- Factory ---

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('V', 'r', 't', 'x'),
    .name = "Vortex",
    .description = "Vortex v" VORTEX_VERSION " - multi-mode filter",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = NULL,
    .midiRealtime = NULL,
    .midiMessage = midiMessage,
    .tags = kNT_tagEffect | kNT_tagFilterEQ,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
    .serialise = NULL,
    .deserialise = NULL,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = parameterString,
};

// --- Entry point ---

extern "C"
uintptr_t pluginEntry( _NT_selector selector, uint32_t data )
{
    switch ( selector )
    {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)( ( data == 0 ) ? &factory : NULL );
    }
    return 0;
}
```

**Step 2: Commit**

```bash
git add vortex.cpp
git commit -m "Plugin implementation: params, audio processing, MIDI, CV modulation"
```

---

### Task 6: Build Verification

**Step 1: Run desktop tests**

Run: `cd tests && make clean && make run`
Expected: All tests PASS

**Step 2: Build ARM binary**

Run: `make clean && make`
Expected: `plugins/vortex.o` and `plugins/plugin.json` created without errors

If the ARM toolchain is not installed:
Run: `brew install gcc-arm-embedded` (or `brew install arm-none-eabi-gcc`)

**Step 3: Verify output files**

Run: `ls -la plugins/`
Expected: `vortex.o` (non-zero size) and `plugin.json` (with correct content)

Run: `cat plugins/plugin.json`
Expected: `{"name": "Vortex", "guid": "Vrtx", "version": "0.1.0", "author": "wintoid", ...}`

**Step 4: Commit build artifacts if any changes**

No build artifacts should be committed (.gitignore handles this). But verify:
Run: `git status`
Expected: nothing to commit, working tree clean

---

### Task 7: README

**Files:**
- Create: `README.md`

**Step 1: Write README**

Create `README.md` with:
- Project description (multi-mode filter for Disting NT)
- Credit to Yuriy Ivantsov for the filter DSP
- Installation instructions (copy to SD card `plugins/vortex/`)
- Filter modes table (7 modes with pole options)
- Parameters reference
- CV inputs reference
- MIDI CC mapping (CC14-21)
- Building from source
- License (MIT)

**Step 2: Commit**

```bash
git add README.md
git commit -m "Add README with parameters, MIDI map, and filter reference"
```
