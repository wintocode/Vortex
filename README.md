# Vortex

Multi-mode filter plugin for the [Expert Sleepers Disting NT](https://expert-sleepers.co.uk/distingNT.html).

7 filter modes with 2/4-pole cascading, pre-filter drive, MIDI keyboard tracking, and 8 CV inputs. Filter DSP ported from [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy Ivantsov.

## Installation

Copy `plugins/vortex.o` and `plugins/plugin.json` to your Disting NT SD card under `plugins/vortex/`.

## Filter Modes

| # | Mode    | Order | Slope      | With 4-pole |
|---|---------|-------|------------|-------------|
| 0 | LP 6dB  | 1st   | -6 dB/oct  | -12 dB/oct  |
| 1 | LP 12dB | 2nd   | -12 dB/oct | -24 dB/oct  |
| 2 | HP 6dB  | 1st   | +6 dB/oct  | +12 dB/oct  |
| 3 | HP 12dB | 2nd   | +12 dB/oct | +24 dB/oct  |
| 4 | BP      | 2nd   | bandpass   | narrower    |
| 5 | Notch   | 2nd   | band reject| deeper      |
| 6 | AP      | 2nd   | allpass    | more phase  |

The **Poles** parameter (2/4) cascades two filter instances for steeper slopes.

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
| Mode      | 0-6         | LP 12dB | Filter type                     |
| Cutoff    | 20-20000 Hz | ~632 Hz | Cutoff frequency (exponential)  |
| Resonance | 0-100%      | 0%      | Filter resonance                |
| Poles     | 2/4         | 2-pole  | Single or cascaded stages       |
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
| 17  | Poles        |
| 18  | Drive        |
| 19  | Mix          |
| 20  | FM Depth     |
| 21  | MIDI Channel |

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
