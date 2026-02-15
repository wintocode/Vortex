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
    // At x=3, soft_clip converges to exactly 1.0 (3x compression)
    ASSERT_NEAR(vortex::soft_clip(3.0f), 1.0f, 1e-4f);
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
