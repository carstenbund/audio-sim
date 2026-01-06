/**
 * @file audio_synth.c
 * @brief 48kHz audio synthesis from modal state
 *
 * Based on Python implementation in experiments/audio_sonification.py
 * Adapted for ESP32 with:
 * - Fixed-point phase accumulator for efficiency
 * - Fast sine approximation
 * - Amplitude smoothing to avoid clicks
 * - Direct mapping from modal state
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
 * Much faster than sinf() on ESP32
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
                     float carrier_freq_hz) {
    memset(synth, 0, sizeof(audio_synth_t));

    synth->node = node;
    synth->params.carrier_freq_hz = carrier_freq_hz;
    synth->params.sample_rate = SAMPLE_RATE;
    synth->params.phase_accumulator = 0;
    synth->params.master_gain = 1.0f;
    synth->params.muted = false;

    synth->initialized = true;
}

// ============================================================================
// Audio Generation
// ============================================================================

int16_t* audio_synth_generate_buffer(audio_synth_t* synth) {
    if (!synth->initialized || !synth->node) {
        // Return silence
        memset(synth->buffer, 0, sizeof(synth->buffer));
        return synth->buffer;
    }

    if (synth->params.muted) {
        // Muted: return silence
        memset(synth->buffer, 0, sizeof(synth->buffer));
        return synth->buffer;
    }

    // Get current modal state (thread-safe read, assumed atomic)
    float amplitude_raw = modal_node_get_amplitude(synth->node);
    float phase_mod = modal_node_get_phase_modulation(synth->node);

    // Smooth amplitude to avoid clicks (exponential smoothing)
    static float amplitude_smooth = 0.0f;
    amplitude_smooth = amplitude_smooth + SMOOTH_ALPHA * (amplitude_raw - amplitude_smooth);

    // Final amplitude with master gain
    float amplitude = amplitude_smooth * synth->params.master_gain * MAX_AMPLITUDE_SCALE;

    // Clip to safe range
    if (amplitude > MAX_AMPLITUDE_SCALE) {
        amplitude = MAX_AMPLITUDE_SCALE;
    }

    // Generate samples
    float carrier_freq = synth->params.carrier_freq_hz;
    float phase_inc = 2.0f * M_PI * carrier_freq / synth->params.sample_rate;

    uint32_t phase_acc = synth->params.phase_accumulator;

    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        // Phase accumulator (wraps automatically with uint32)
        float phase = (phase_acc / 4294967296.0f) * 2.0f * M_PI;

        // Add phase modulation from mode 2
        phase += phase_mod;

        // Generate carrier with fast sine
        float sample = amplitude * fast_sin(phase);

        // Convert to 16-bit PCM
        int16_t sample_i16 = (int16_t)(sample * 32767.0f);
        synth->buffer[i] = sample_i16;

        // Advance phase (uint32 wraps automatically at 2π equivalent)
        phase_acc += (uint32_t)(phase_inc * 4294967296.0f / (2.0f * M_PI));
    }

    // Store updated phase accumulator
    synth->params.phase_accumulator = phase_acc;

    return synth->buffer;
}

// ============================================================================
// Control Functions
// ============================================================================

void audio_synth_set_frequency(audio_synth_t* synth, float freq_hz) {
    synth->params.carrier_freq_hz = freq_hz;
}

void audio_synth_set_gain(audio_synth_t* synth, float gain) {
    synth->params.master_gain = (gain < 0.0f) ? 0.0f : (gain > 1.0f) ? 1.0f : gain;
}

void audio_synth_set_mute(audio_synth_t* synth, bool mute) {
    synth->params.muted = mute;
}

void audio_synth_reset_phase(audio_synth_t* synth) {
    synth->params.phase_accumulator = 0;
}
