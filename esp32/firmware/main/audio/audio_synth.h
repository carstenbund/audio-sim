/**
 * @file audio_synth.h
 * @brief 48kHz 4-channel audio synthesis from modal state
 *
 * Audio-first design:
 * - Runs independently at 48kHz regardless of network
 * - Modal state directly drives audio output
 * - I2S DMA output for low-latency
 * - 4-channel output (one channel per mode)
 *
 * Channel mapping:
 * - Channel 0: Mode 0 direct output
 * - Channel 1: Mode 1 direct output
 * - Channel 2: Mode 2 direct output
 * - Channel 3: Mode 3 direct output
 *
 * Each mode synthesizes its own sinusoid at its frequency (omega[k]),
 * with amplitude envelope from the mode's complex amplitude |a_k|.
 */

#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

#include <stdint.h>
#include <stdbool.h>
#include "modal_node.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define SAMPLE_RATE 48000
#define AUDIO_BUFFER_SAMPLES 480  // 10ms buffer @ 48kHz
#define NUM_AUDIO_CHANNELS 4      // 4 channels (one per mode)
#define AUDIO_BUFFER_SIZE (AUDIO_BUFFER_SAMPLES * NUM_AUDIO_CHANNELS)  // Total buffer size
#define BITS_PER_SAMPLE 16

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Audio synthesis parameters
 */
typedef struct {
    float sample_rate;                    ///< Sample rate (Hz)
    uint32_t phase_accumulator[MAX_MODES]; ///< Phase accumulators (one per mode)
    float mode_gains[MAX_MODES];          ///< Per-mode gains [0,1]
    float master_gain;                    ///< Master output gain [0,1]
    bool muted;                           ///< Mute flag
} audio_synth_params_t;

/**
 * @brief Audio synthesis state
 */
typedef struct {
    audio_synth_params_t params;
    int16_t buffer[AUDIO_BUFFER_SIZE];  ///< DMA buffer (4-channel interleaved)
    const modal_node_t* node;           ///< Reference to modal node state
    float amplitude_smooth[MAX_MODES];  ///< Smoothed amplitudes per mode
    bool initialized;
} audio_synth_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize audio synthesis engine
 *
 * @param synth Pointer to synthesis state
 * @param node Pointer to modal node (state source)
 *
 * Note: Frequencies are taken from mode parameters (omega[k])
 */
void audio_synth_init(audio_synth_t* synth,
                     const modal_node_t* node);

/**
 * @brief Generate one buffer of audio samples
 *
 * Reads current modal state and generates audio samples.
 * Called by I2S DMA interrupt or audio task.
 *
 * @param synth Pointer to synthesis state
 * @return Pointer to audio buffer (ready for I2S write)
 */
int16_t* audio_synth_generate_buffer(audio_synth_t* synth);

/**
 * @brief Set per-mode gain
 *
 * @param synth Pointer to synthesis state
 * @param mode_idx Mode index [0-3]
 * @param gain Gain [0,1]
 */
void audio_synth_set_mode_gain(audio_synth_t* synth, int mode_idx, float gain);

/**
 * @brief Set master gain
 *
 * @param synth Pointer to synthesis state
 * @param gain Gain [0,1]
 */
void audio_synth_set_gain(audio_synth_t* synth, float gain);

/**
 * @brief Mute/unmute audio
 *
 * @param synth Pointer to synthesis state
 * @param mute Mute flag
 */
void audio_synth_set_mute(audio_synth_t* synth, bool mute);

/**
 * @brief Reset phase (hard sync)
 *
 * @param synth Pointer to synthesis state
 */
void audio_synth_reset_phase(audio_synth_t* synth);

// ============================================================================
// I2S Driver Interface
// ============================================================================

/**
 * @brief Initialize I2S driver for PCM5102A DAC
 *
 * Configures ESP32 I2S peripheral:
 * - 48kHz sample rate
 * - 16-bit samples
 * - Mono or stereo output
 * - DMA buffers
 */
void audio_i2s_init(void);

/**
 * @brief Write audio buffer to I2S
 *
 * @param buffer Pointer to audio samples
 * @param size Buffer size in bytes
 * @return Number of bytes written
 */
size_t audio_i2s_write(const int16_t* buffer, size_t size);

/**
 * @brief Audio task (FreeRTOS)
 *
 * Continuously generates audio and writes to I2S.
 * Runs at high priority on dedicated core.
 *
 * @param pvParameters Task parameters (audio_synth_t*)
 */
void audio_task(void* pvParameters);

// ============================================================================
// Synthesis Helpers
// ============================================================================

/**
 * @brief Fast sine approximation
 *
 * Uses lookup table or fast polynomial for low-latency synthesis.
 *
 * @param phase Phase in radians
 * @return Sine value [-1, 1]
 */
float fast_sin(float phase);

/**
 * @brief Apply smooth envelope (for poke transients)
 *
 * @param t Time normalized [0,1]
 * @return Envelope value [0,1]
 */
float envelope_hann(float t);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_SYNTH_H
