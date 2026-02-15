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
