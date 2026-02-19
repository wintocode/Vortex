# Vortex

A multi-mode filter plugin for the [Expert Sleepers Disting NT](https://expert-sleepers.co.uk/distingNT.html).

Vortex offers 12 filter modes from gentle 6 dB/oct slopes to steep 24 dB/oct cascades, with pre-filter drive, dry/wet mix, and 7 CV inputs. Use it for subtractive synthesis, DJ-style filter sweeps, resonant acid lines, or as a CV-controlled spectral shaper.

Filter DSP ported from [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy Ivantsov — decramped IIR state-space filters with sigma frequency warping for clean audio-rate modulation.

## Installation

1. Download or build `vortex.o` and `plugin.json` from the `plugins/` directory.
2. Copy both files into a folder on your Disting NT SD card under `plugins/` (e.g. `plugins/vortex/`).
3. Power on the Disting NT — Vortex will appear in the algorithm list under **Effects**.

## Signal Chain

```
Audio In -> Drive (soft-clip) -> Filter -> Dry/Wet Mix -> Output Bus
```

## Filter Modes

| #  | Mode    | Slope       | Description                    |
|----|---------|-------------|--------------------------------|
| 0  | LP 6dB  | -6 dB/oct   | 1st-order low-pass             |
| 1  | LP 12dB | -12 dB/oct  | 2nd-order low-pass             |
| 2  | LP 24dB | -24 dB/oct  | Cascaded 2nd-order low-pass    |
| 3  | HP 6dB  | +6 dB/oct   | 1st-order high-pass            |
| 4  | HP 12dB | +12 dB/oct  | 2nd-order high-pass            |
| 5  | HP 24dB | +24 dB/oct  | Cascaded 2nd-order high-pass   |
| 6  | BP      | bandpass    | 2nd-order band-pass            |
| 7  | BP+     | bandpass    | Cascaded (narrower)            |
| 8  | Notch   | band reject | 2nd-order notch                |
| 9  | Notch+  | band reject | Cascaded (deeper)              |
| 10 | AP      | allpass     | 2nd-order all-pass             |
| 11 | AP+     | allpass     | Cascaded (more phase rotation) |

The 6 dB modes are gentle 1st-order filters — no resonance control. The 12 dB and 24 dB modes are 2nd-order (or cascaded 2nd-order) with full resonance support up to near self-oscillation. The "+" variants (BP+, Notch+, AP+) cascade two filter stages for steeper response.

## Parameters

Parameters are organized into pages on the Disting NT display.

### I/O

| Parameter   | Range       | Default | Description |
|-------------|-------------|---------|-------------|
| Input       | Bus 0-13    | Off     | Audio input bus (0 = disconnected) |
| Output      | Bus 1-13    | 1       | Audio output bus |
| Output Mode | Replace/Mix | Replace | Replace the bus signal or mix into it |

### Filter

| Parameter | Range       | Default | Description |
|-----------|-------------|---------|-------------|
| Mode      | 0-11        | LP 12dB | Filter type (see table above) |
| Cutoff    | 20-20000 Hz | ~632 Hz | Cutoff frequency — exponential scaling for even response across the audio range |
| Resonance | 0-100%      | 0%      | Filter resonance. 0% = Butterworth (flat passband), 100% = near self-oscillation. Only affects 12 dB and 24 dB modes. |
| Drive     | 0-100%      | 0%      | Pre-filter soft-clip saturation. Boosts the signal 1x-10x then applies a smooth rational saturator for warm overdrive without hard clipping. |

### Global

| Parameter | Range        | Default | Description |
|-----------|--------------|---------|-------------|
| Mix       | 0-100%       | 100%    | Dry/wet blend. 0% = fully dry (bypass), 100% = fully wet |
| FM Depth  | -100 to 100% | 0%     | Attenuverter for the FM CV input. Controls how much the FM CV modulates the cutoff frequency. Negative values invert the modulation. |
| Version   | read-only    | -       | Displays the current firmware version |

### CV Inputs

Each CV input can be assigned to any bus on the Disting NT (0 = disconnected).

| Input        | Effect |
|--------------|--------|
| Audio In     | Audio input signal. Used when no audio bus is selected on the I/O page. |
| Cutoff V/OCT | 1V/oct cutoff frequency tracking. Multiplies the base cutoff exponentially — 1V doubles the frequency. |
| Cutoff FM    | FM modulation input. Scaled by the FM Depth parameter — at +100% depth, 1V doubles the cutoff; at -100%, 1V halves it. |
| Resonance    | Modulates resonance amount (±20% of range per volt) |
| Mode         | CV selection of filter mode (±5V sweeps all 12 modes) |
| Drive        | Modulates drive amount (±20% of range per volt) |
| Mix          | Modulates dry/wet blend (±20% of range per volt) |

## Patching Tips

- **Subtractive synth** — Feed a sawtooth oscillator into Audio In, set LP 24dB, Resonance at 30-50%, and modulate Cutoff with an envelope via V/OCT CV for classic analog-style patches.
- **Acid bass** — LP 12dB with high resonance (70-90%), moderate drive (30-50%), and a fast envelope on cutoff. The resonance peak creates the characteristic squelch.
- **DJ filter sweep** — Use LP 24dB or HP 24dB with Mix at 100%. Sweep Cutoff manually or via CV for dramatic build-ups and breakdowns.
- **Parallel filtering** — Set Output Mode to Mix and use multiple Vortex instances with different modes (e.g. LP + HP) on the same bus for creative crossover effects.
- **Warm saturation** — Even without filtering, use Drive at 40-60% with Mix at 100% in AP mode for transparent soft-clip warmth.
- **Phaser effect** — AP or AP+ mode with cutoff modulated by a slow LFO creates phase-shifting effects. Mix dry and wet signals for comb filtering.

## Building from Source

Requirements:
- `arm-none-eabi-c++` (ARM GCC toolchain)
- The [Disting NT API](https://github.com/expertsleepersltd/distingNT_API) (included as `distingNT_API/`)

```bash
brew install arm-none-eabi-gcc   # macOS
make                              # builds plugins/vortex.o + plugins/plugin.json
```

Run tests (desktop):

```bash
cd tests && make run
```

## Credits

Filter DSP based on [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy Ivantsov, ported from C++20 to C++11.

## License

MIT — see [LICENSE](LICENSE).

## Author

wintocode
