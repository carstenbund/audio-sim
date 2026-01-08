/**
 * @file ModalAttractorsAU.h
 * @brief Main Audio Unit class for Modal Attractors plugin
 *
 * This is the primary AU implementation that bridges macOS Audio Unit SDK
 * with the C++ DSP engine.
 *
 * Note: This is a skeleton/template. Full implementation requires:
 * - AudioUnit SDK headers (from Xcode)
 * - Proper Objective-C++ implementation
 * - AU validation and testing
 */

#ifndef MODAL_ATTRACTORS_AU_H
#define MODAL_ATTRACTORS_AU_H

#include <cstdint>

// NOTE: This file is a C++ header skeleton. The actual AU implementation
// would be in Objective-C++ (.mm file) and would include:
// - <AudioUnit/AudioUnit.h>
// - <AudioToolbox/AudioToolbox.h>
// - AUInstrumentBase subclass

// For now, this serves as a placeholder to define the interface

/**
 * @brief Modal Attractors AU Plugin Class (C++ interface)
 *
 * The actual implementation will be in Objective-C++:
 * @interface ModalAttractorsAU : AUInstrumentBase
 */

// Forward declarations
class VoiceAllocator;
class TopologyEngine;

/**
 * @brief C++ wrapper for AU plugin state
 *
 * This struct holds the DSP engine state that the Objective-C++
 * AU wrapper will manage.
 */
struct ModalAttractorsEngine {
    VoiceAllocator* voice_allocator;
    TopologyEngine* topology_engine;

    float sample_rate;
    uint32_t max_polyphony;

    // Parameter cache (updated from AU parameter changes)
    float master_gain;
    float coupling_strength;
    int topology_type;
    int personality;

    bool initialized;
};

/**
 * @brief Initialize the DSP engine
 * @param engine Engine state struct
 * @param sample_rate Sample rate in Hz
 * @param max_polyphony Maximum number of voices
 */
void modal_attractors_engine_init(ModalAttractorsEngine* engine,
                                  float sample_rate,
                                  uint32_t max_polyphony);

/**
 * @brief Clean up engine resources
 * @param engine Engine state struct
 */
void modal_attractors_engine_cleanup(ModalAttractorsEngine* engine);

/**
 * @brief Process MIDI note on
 * @param engine Engine state
 * @param note MIDI note number
 * @param velocity MIDI velocity (0-127)
 */
void modal_attractors_engine_note_on(ModalAttractorsEngine* engine,
                                     uint8_t note,
                                     uint8_t velocity);

/**
 * @brief Process MIDI note off
 * @param engine Engine state
 * @param note MIDI note number
 */
void modal_attractors_engine_note_off(ModalAttractorsEngine* engine,
                                      uint8_t note);

/**
 * @brief Render audio
 * @param engine Engine state
 * @param outL Left channel output
 * @param outR Right channel output
 * @param num_frames Number of frames to render
 */
void modal_attractors_engine_render(ModalAttractorsEngine* engine,
                                    float* outL,
                                    float* outR,
                                    uint32_t num_frames);

/**
 * @brief Update parameter
 * @param engine Engine state
 * @param param_id Parameter ID
 * @param value Parameter value
 */
void modal_attractors_engine_set_parameter(ModalAttractorsEngine* engine,
                                           uint32_t param_id,
                                           float value);

#endif // MODAL_ATTRACTORS_AU_H
