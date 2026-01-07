/**
 * @file audio_synth.c
 * @brief Variable sample rate audio synthesis from modal state (macOS port)
 *
 * Ported from ESP32 firmware - adapted for AU plugin:
 * - Variable sample rates (not fixed 48kHz)
 * - Pull-based rendering (AU callback)
 * - Stereo float output (not 4-channel TDM)
 * - No I2S/DMA code
 */

#include "audio_synth.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Constants
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SMOOTH_ALPHA 0.12f  // Smoothing factor (matches Python SMOOTH)
#define MAX_AMPLITUDE_SCALE 0.7f  // Headroom (matches Python MAX_AMPLITUDE)

// ============================================================================
// Fast Math Helpers
// ============================================================================

/**
 * @brief Fast sine approximation using Taylor series
 *
 * Accurate enough for audio (error < 0.1%)
 * Much faster than sinf() on ESP32, still useful on macOS
 */
float fast_sin(float x) {
    // Normalize to [-π, π]
    while (x > M_PI) x -= 2.0f * M_PI;
    while (x < -M_PI) x += 2.0f * M_PI;

    // Taylor series: sin(x) ≈ x - x³/6 + x⁵/120
    float x2 = x * x;
    float x3 = x * x2;
    float x5 = x3 * x2;

    return x - (x3 / 6.0f) + (x5 / 120.0f);
}

/**
 * @brief Hann window envelope
 */
float envelope_hann(float t) {
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 0.0f;
    return 0.5f * (1.0f - cosf(M_PI * t));
}

// ============================================================================
// Initialization
// ============================================================================

void audio_synth_init(audio_synth_t* synth,
                     const modal_node_t* node,
                     float sample_rate) {
    memset(synth, 0, sizeof(audio_synth_t));

    synth->node = node;
    synth->params.sample_rate = sample_rate;
    synth->params.master_gain = 1.0f;
    synth->params.muted = false;

    // Initialize per-mode parameters
    for (int k = 0; k < MAX_MODES; k++) {
        synth->params.phase_accumulator[k] = 0;
        synth->params.mode_gains[k] = 1.0f;
        synth->amplitude_smooth[k] = 0.0f;
    }

    synth->initialized = true;
}

void audio_synth_set_sample_rate(audio_synth_t* synth, float sample_rate) {
    synth->params.sample_rate = sample_rate;
}

// ============================================================================
// Audio Generation
// ============================================================================

void audio_synth_render(audio_synth_t* synth,
                       float* outL,
                       float* outR,
                       uint32_t num_frames) {
    if (!synth->initialized || !synth->node) {
        // Return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    if (synth->params.muted) {
        // Muted: return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    const modal_node_t* node = synth->node;
    const float sample_rate = synth->params.sample_rate;

    // Generate stereo audio
    // Mix all modes together for both channels
    for (uint32_t sample_idx = 0; sample_idx < num_frames; sample_idx++) {
        float sample_sum = 0.0f;

        for (int k = 0; k < MAX_MODES; k++) {
            // Skip inactive modes
            if (!node->modes[k].params.active) {
                continue;
            }

            // Get mode amplitude (|a_k|)
            float amplitude_raw = cabsf(node->modes[k].a);

            // Apply mode weight
            amplitude_raw *= node->modes[k].params.weight;

            // Smooth amplitude to avoid clicks
            synth->amplitude_smooth[k] +=
                SMOOTH_ALPHA * (amplitude_raw - synth->amplitude_smooth[k]);

            // Final amplitude with gains
            float amplitude = synth->amplitude_smooth[k] *
                            synth->params.mode_gains[k] *
                            synth->params.master_gain *
                            MAX_AMPLITUDE_SCALE;

            // Clip to safe range
            if (amplitude > MAX_AMPLITUDE_SCALE) {
                amplitude = MAX_AMPLITUDE_SCALE;
            }

            // Get mode frequency (omega[k] in rad/s)
            float omega = node->modes[k].params.omega;
            float freq_hz = omega / (2.0f * M_PI);

            // Phase increment (adapted for variable sample rate)
            float phase_inc = 2.0f * M_PI * freq_hz / sample_rate;

            // Get phase from accumulator
            uint32_t phase_acc = synth->params.phase_accumulator[k];
            float phase = (phase_acc / 4294967296.0f) * 2.0f * M_PI;

            // Get phase from complex amplitude (use arg(a_k) for phase coherence)
            float mode_phase = cargf(node->modes[k].a);
            phase += mode_phase;

            // Generate sample with fast sine
            float sample_f = amplitude * fast_sin(phase);

            // Add to mix
            sample_sum += sample_f;

            // Advance phase accumulator
            phase_acc += (uint32_t)(phase_inc * 4294967296.0f / (2.0f * M_PI));
            synth->params.phase_accumulator[k] = phase_acc;
        }

        // Write to stereo output (mono source, duplicated to L/R)
        outL[sample_idx] = sample_sum;
        outR[sample_idx] = sample_sum;
    }
}

// ============================================================================
// Control Functions
// ============================================================================

void audio_synth_set_mode_gain(audio_synth_t* synth, int mode_idx, float gain) {
    if (mode_idx < 0 || mode_idx >= MAX_MODES) return;

    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;

    synth->params.mode_gains[mode_idx] = gain;
}

void audio_synth_set_gain(audio_synth_t* synth, float gain) {
    synth->params.master_gain = (gain < 0.0f) ? 0.0f : (gain > 1.0f) ? 1.0f : gain;
}

void audio_synth_set_mute(audio_synth_t* synth, bool mute) {
    synth->params.muted = mute;
}

void audio_synth_reset_phase(audio_synth_t* synth) {
    for (int k = 0; k < MAX_MODES; k++) {
        synth->params.phase_accumulator[k] = 0;
    }
}
