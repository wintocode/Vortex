# Vortex - Multi-mode Filter for Disting NT

## Overview

Vortex is a mono multi-mode filter plugin for the Expert Sleepers Disting NT. It implements decramped IIR state-space filters ported from the [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) library (C++20 → C++11), designed for clean audio-rate modulation.

**GUID:** `Vrtx` (`NT_MULTICHAR('V', 'r', 't', 'x')`)
**Author:** wintoid
**Tag:** `kNT_tagEffect`
**Version:** 0.1.0

## Signal Chain

```
Audio In → Input Gain → Drive (soft-clip) → Filter → Dry/Wet Mix → Output Bus
```

## Filter Modes

| # | Mode   | Order | Base Slope  | With 4-pole |
|---|--------|-------|-------------|-------------|
| 0 | LP 6   | 1st   | -6 dB/oct  | -12 dB/oct  |
| 1 | LP 12  | 2nd   | -12 dB/oct | -24 dB/oct  |
| 2 | HP 6   | 1st   | +6 dB/oct  | +12 dB/oct  |
| 3 | HP 12  | 2nd   | +12 dB/oct | +24 dB/oct  |
| 4 | BP     | 2nd   | bandpass    | narrower    |
| 5 | Notch  | 2nd   | band reject | deeper      |
| 6 | AP     | 2nd   | allpass     | more phase  |

**Cascading:** A "Poles" parameter (2/4) cascades two filter instances. For 1st order types, 4-pole gives 12 dB/oct; for 2nd order, 4-pole gives 24 dB/oct.

## Filter Implementation

Ported from ivantsov-filters state-space design:

- **First order filters** use a single state variable `z` with coefficients `b[0..1]` (plus `b[2]` for shelves). The `processed()` function computes a theta term and derives the output based on filter type.

- **Second order filters** use two state variables `z[0..1]` with coefficients `b[0..3]` (plus `b[4]` for shelves). The update involves a theta computation with cross-coupling through `b[1]`.

- **Frequency warping** (`Warp::Sigma`): A scaled approximation of the ideal bilinear transform that corrects frequency response at high frequencies. Enabled by default for audio-rate modulation quality.

- **Resonance mapping:** The ivantsov library uses "damping" = 1/(2Q). We map our 0-100% Resonance parameter as: `damping = lerp(0.707, 0.01, resonance)` where 0% = Butterworth (Q=0.707) and 100% = near self-oscillation (Q=50).

## Parameters

### I/O Page
| Parameter   | Range      | Default | Description               |
|-------------|------------|---------|---------------------------|
| Input Bus   | 1-13       | 1       | Audio input bus            |
| Output Bus  | 1-13       | 1       | Audio output bus           |
| Output Mode | Replace/Mix| Replace | Replace or add to bus     |

### Filter Page
| Parameter  | Range       | Default | Description                              |
|------------|-------------|---------|------------------------------------------|
| Mode       | 0-6         | 1       | Filter type (LP6/LP12/HP6/HP12/BP/N/AP)  |
| Cutoff     | 20-20000 Hz | 1000    | Filter cutoff frequency                  |
| Resonance  | 0-100%      | 0       | Filter resonance (Q)                     |
| Poles      | 2 / 4       | 2       | Single or cascaded stages                |
| Drive      | 0-100%      | 0       | Pre-filter soft-clip saturation          |

### Global Page
| Parameter    | Range    | Default | Description              |
|--------------|----------|---------|--------------------------|
| Mix          | 0-100%   | 100     | Dry/wet blend            |
| MIDI Channel | 1-16     | 1       | MIDI input channel       |
| Version      | readonly | -       | Firmware version display |

### CV Inputs (8 assignable)
| CV Input     | Description                          |
|--------------|--------------------------------------|
| Audio In     | Audio input signal                   |
| Cutoff V/OCT | 1V/oct cutoff frequency tracking    |
| Cutoff FM    | FM modulation input                  |
| FM Depth     | Attenuverter for FM (-100% to +100%) |
| Resonance    | CV modulation of resonance           |
| Mode         | CV selection of filter mode          |
| Drive        | CV modulation of drive amount        |
| Mix          | CV control of dry/wet                |

## MIDI Control

- **Note On/Off:** Sets base cutoff frequency via keyboard tracking (MIDI note → Hz)
- **Pitch Bend:** ±2 semitones on cutoff frequency
- **CC Mapping:** CCs 14+ mapped to all value parameters (consistent with meld/four)

## Architecture

### File Structure
```
Vortex/
├── vortex.cpp          # NT plugin: params, MIDI, step(), factory
├── dsp.h               # Ported ivantsov filters (C++11) + drive utils
├── Makefile            # ARM Cortex-M7 cross-compilation
├── plugin.json         # NT Gallery manifest
├── VERSION             # Semantic version
├── LICENSE             # MIT
├── .gitignore
├── .gitmodules         # distingNT_API submodule
├── distingNT_API/      # Expert Sleepers SDK
├── plugins/            # Build output (gitignored)
├── tests/
│   ├── Makefile        # Desktop test build
│   └── test_dsp.cpp   # Unit tests for filter math
└── docs/
    └── plans/
```

### C++20 → C++11 Porting Strategy

The ivantsov-filters library uses C++20 features that must be adapted:

| C++20 Feature              | C++11 Replacement                          |
|----------------------------|--------------------------------------------|
| `concept is_algebraic`     | Remove (we only use `float`)               |
| `std::numbers::pi_v<T>`   | `static constexpr float PI = 3.14159265f`  |
| `std::numbers::sqrt2_v<T>`| `static constexpr float SQRT2 = 1.41421356f`|
| `std::numbers::inv_pi_v<T>`| `static constexpr float INV_PI = 0.31830989f`|
| `auto` return types        | Explicit `void`/`float` return types       |
| `requires` clauses         | Remove (single concrete type)              |
| `enum struct` scoped enums | Keep (C++11 supports `enum class`)         |
| `std::array`               | Keep (C++11)                               |
| Structured bindings        | Separate variable declarations             |
| Template aliases           | Direct struct usage                        |

The state-space math and coefficient calculations remain identical.

### DSP Details

**Drive:** `soft_clip(x) = x * (27 + x*x) / (27 + 9*x*x)` — rational saturator (same as four/meld).

**Denormal prevention:** `flush_denormal()` on filter state after each sample.

**Cutoff frequency mapping:** Exponential, V/OCT-compatible. Base from parameter (20-20000 Hz log scale), modulated by MIDI note, V/OCT CV, and FM CV with depth.

**Resonance mapping:** Linear interpolation of damping factor. `damping = 0.707 * (1 - resonance) + 0.01 * resonance`. Butterworth at 0%, near self-oscillation at 100%.

## Implementation Notes

- No `src/` directory (flat layout, per submission requirements)
- Build target: ARM Cortex-M7 with FPU (`-mcpu=cortex-m7 -mfpu=fpv5-d16`)
- C++11 standard (`-std=c++11`)
- Optimized for size (`-Os`)
- Version macro: `VORTEX_VERSION`
