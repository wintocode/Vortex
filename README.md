# Vortex

Multi-mode filter plugin for the [Expert Sleepers Disting NT](https://expert-sleepers.co.uk/distingNT.html).

12 filter modes with pre-filter drive, MIDI keyboard tracking, and 7 CV inputs. Filter DSP ported from [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy Ivantsov.

## Installation

Copy `plugins/vortex.o` and `plugins/plugin.json` to your Disting NT SD card under `plugins/vortex/`.

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

## Parameters

### I/O
| Parameter   | Range       | Default | Description           |
|-------------|-------------|---------|-----------------------|
| Input       | Bus 1-13    | Off     | Audio input bus        |
| Output      | Bus 1-13    | 1       | Audio output bus       |
| Output Mode | Replace/Mix | Replace | Replace or add to bus |

### Filter
| Parameter | Range       | Default | Description                     |
|-----------|-------------|---------|---------------------------------|
| Mode      | 0-11        | LP 12dB | Filter type (see table above)   |
| Cutoff    | 20-20000 Hz | ~632 Hz | Cutoff frequency (exponential)  |
| Resonance | 0-100%      | 0%      | Filter resonance                |
| Drive     | 0-100%      | 0%      | Pre-filter soft-clip saturation |

### Global
| Parameter    | Range      | Default | Description         |
|--------------|------------|---------|---------------------|
| Mix          | 0-100%     | 100%    | Dry/wet blend       |
| FM Depth     | -100..100% | 0%      | FM input attenuverter |
| MIDI Channel | 1-16       | 1       | MIDI input channel  |
| Version      | read-only  | -       | Firmware version    |

### CV Inputs
| Input        | Description                       |
|--------------|-----------------------------------|
| Audio In     | Audio input signal                |
| Cutoff V/OCT | 1V/oct cutoff frequency tracking |
| Cutoff FM    | FM modulation (scaled by FM Depth)|
| Resonance    | CV modulation of resonance        |
| Mode         | CV selection of filter mode       |
| Drive        | CV modulation of drive amount     |
| Mix          | CV control of dry/wet             |

## MIDI

- **Note On/Off:** Sets cutoff frequency via keyboard tracking (MIDI note -> Hz)
- **Pitch Bend:** +/-2 semitones on cutoff frequency
- **CC Mapping:**

| CC  | Parameter    |
|-----|--------------|
| 14  | Mode         |
| 15  | Cutoff       |
| 16  | Resonance    |
| 17  | Drive        |
| 18  | Mix          |
| 19  | FM Depth     |
| 20  | MIDI Channel |

## Building

Requires `arm-none-eabi-c++` (ARM GCC toolchain):

```bash
brew install arm-none-eabi-gcc   # macOS
make                              # builds plugins/vortex.o + plugins/plugin.json
```

Run tests (desktop):

```bash
cd tests && make run
```

## Signal Chain

```
Audio In -> Drive (soft-clip) -> Filter -> Dry/Wet Mix -> Output Bus
```

## Credits

Filter DSP based on [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy Ivantsov, ported from C++20 to C++11.

## License

MIT
