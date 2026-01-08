/**
 * @file ModalAttractorsEngine.cpp
 * @brief C++ implementation of DSP engine interface
 *
 * This provides a C-compatible interface to the C++ DSP engine
 * so it can be called from Objective-C++ AU wrapper.
 */

#include "ModalAttractorsAU.h"
#include "ModalParameters.h"
#include "../dsp_core/VoiceAllocator.h"
#include "../dsp_core/TopologyEngine.h"
#include <cstring>

void modal_attractors_engine_init(ModalAttractorsEngine* engine,
                                  float sample_rate,
                                  uint32_t max_polyphony) {
    if (!engine) return;

    memset(engine, 0, sizeof(ModalAttractorsEngine));

    engine->sample_rate = sample_rate;
    engine->max_polyphony = max_polyphony;

    // Create DSP components
    engine->voice_allocator = new VoiceAllocator(max_polyphony);
    engine->topology_engine = new TopologyEngine(max_polyphony);

    // Initialize
    engine->voice_allocator->initialize(sample_rate);

    // Set default parameters
    engine->master_gain = kMasterGain_Default;
    engine->coupling_strength = kCouplingStrength_Default;
    engine->topology_type = kTopology_Default;
    engine->personality = kPersonality_Default;

    // Initialize default mode parameters (harmonic series with slight detuning)
    engine->mode_freq_multipliers[0] = 1.0f;   // Fundamental
    engine->mode_freq_multipliers[1] = 1.01f;  // Slightly detuned
    engine->mode_freq_multipliers[2] = 2.0f;   // Second harmonic
    engine->mode_freq_multipliers[3] = 3.0f;   // Third harmonic

    engine->mode_dampings[0] = 0.5f;
    engine->mode_dampings[1] = 0.6f;
    engine->mode_dampings[2] = 0.8f;
    engine->mode_dampings[3] = 1.0f;

    engine->mode_weights[0] = 1.0f;
    engine->mode_weights[1] = 0.7f;
    engine->mode_weights[2] = 0.5f;
    engine->mode_weights[3] = 0.3f;

    // Initialize default poke parameters
    engine->poke_strength = kPokeStrength_Default;
    engine->poke_duration_ms = kPokeDuration_Default;

    // Set default topology
    engine->topology_engine->generateTopology(
        TopologyType::Ring,
        engine->coupling_strength
    );

    // Set default personality on all voices
    engine->voice_allocator->setPersonality(
        static_cast<node_personality_t>(engine->personality)
    );

    // Apply default poke parameters to all voices
    engine->voice_allocator->setPokeStrength(engine->poke_strength);
    engine->voice_allocator->setPokeDuration(engine->poke_duration_ms);

    engine->initialized = true;
}

void modal_attractors_engine_cleanup(ModalAttractorsEngine* engine) {
    if (!engine) return;

    if (engine->voice_allocator) {
        delete engine->voice_allocator;
        engine->voice_allocator = nullptr;
    }

    if (engine->topology_engine) {
        delete engine->topology_engine;
        engine->topology_engine = nullptr;
    }

    engine->initialized = false;
}

void modal_attractors_engine_note_on(ModalAttractorsEngine* engine,
                                     uint8_t note,
                                     uint8_t velocity) {
    if (!engine || !engine->initialized) return;

    engine->voice_allocator->noteOn(note, velocity);
}

void modal_attractors_engine_note_off(ModalAttractorsEngine* engine,
                                      uint8_t note) {
    if (!engine || !engine->initialized) return;

    engine->voice_allocator->noteOff(note);
}

void modal_attractors_engine_render(ModalAttractorsEngine* engine,
                                    float* outL,
                                    float* outR,
                                    uint32_t num_frames) {
    if (!engine || !engine->initialized) {
        // Return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    // Update voices at control rate
    // (simplified - in real implementation, only update at control rate intervals)
    engine->voice_allocator->updateVoices();

    // Update coupling between voices
    ModalVoice** voices = new ModalVoice*[engine->max_polyphony];
    for (uint32_t i = 0; i < engine->max_polyphony; i++) {
        voices[i] = engine->voice_allocator->getVoice(i);
    }
    engine->topology_engine->updateCoupling(voices, engine->max_polyphony);
    delete[] voices;

    // Render audio
    engine->voice_allocator->renderAudio(outL, outR, num_frames);

    // Apply master gain
    for (uint32_t i = 0; i < num_frames; i++) {
        outL[i] *= engine->master_gain;
        outR[i] *= engine->master_gain;
    }
}

void modal_attractors_engine_set_parameter(ModalAttractorsEngine* engine,
                                           uint32_t param_id,
                                           float value) {
    if (!engine || !engine->initialized) return;

    switch (param_id) {
        case kParam_MasterGain:
            engine->master_gain = value;
            break;

        case kParam_CouplingStrength:
            engine->coupling_strength = value;
            engine->topology_engine->setCouplingStrength(value);
            break;

        case kParam_Topology: {
            engine->topology_type = static_cast<int>(value);
            // Map int to topology type
            TopologyType topo = TopologyType::Ring;
            switch (engine->topology_type) {
                case 0: topo = TopologyType::Ring; break;
                case 1: topo = TopologyType::SmallWorld; break;
                case 2: topo = TopologyType::Clustered; break;
                case 3: topo = TopologyType::HubSpoke; break;
                case 4: topo = TopologyType::Random; break;
                case 5: topo = TopologyType::Complete; break;
                case 6: topo = TopologyType::None; break;
            }
            engine->topology_engine->generateTopology(topo, engine->coupling_strength);
            break;
        }

        case kParam_Personality: {
            engine->personality = static_cast<int>(value);
            // Map int to personality type
            // 0 = PERSONALITY_RESONATOR (resonant mode - decays to silence)
            // 1 = PERSONALITY_SELF_OSCILLATOR (self-driving mode - continuous sound)
            node_personality_t personality = (engine->personality == 0)
                ? PERSONALITY_RESONATOR
                : PERSONALITY_SELF_OSCILLATOR;
            engine->voice_allocator->setPersonality(personality);
            break;
        }

        // Mode 0 parameters
        case kParam_Mode0_Frequency:
            engine->mode_freq_multipliers[0] = value;
            engine->voice_allocator->setModeParameters(0, value,
                engine->mode_dampings[0], engine->mode_weights[0]);
            break;

        case kParam_Mode0_Damping:
            engine->mode_dampings[0] = value;
            engine->voice_allocator->setModeParameters(0,
                engine->mode_freq_multipliers[0], value, engine->mode_weights[0]);
            break;

        case kParam_Mode0_Weight:
            engine->mode_weights[0] = value;
            engine->voice_allocator->setModeParameters(0,
                engine->mode_freq_multipliers[0], engine->mode_dampings[0], value);
            break;

        // Mode 1 parameters
        case kParam_Mode1_Frequency:
            engine->mode_freq_multipliers[1] = value;
            engine->voice_allocator->setModeParameters(1, value,
                engine->mode_dampings[1], engine->mode_weights[1]);
            break;

        case kParam_Mode1_Damping:
            engine->mode_dampings[1] = value;
            engine->voice_allocator->setModeParameters(1,
                engine->mode_freq_multipliers[1], value, engine->mode_weights[1]);
            break;

        case kParam_Mode1_Weight:
            engine->mode_weights[1] = value;
            engine->voice_allocator->setModeParameters(1,
                engine->mode_freq_multipliers[1], engine->mode_dampings[1], value);
            break;

        // Mode 2 parameters
        case kParam_Mode2_Frequency:
            engine->mode_freq_multipliers[2] = value;
            engine->voice_allocator->setModeParameters(2, value,
                engine->mode_dampings[2], engine->mode_weights[2]);
            break;

        case kParam_Mode2_Damping:
            engine->mode_dampings[2] = value;
            engine->voice_allocator->setModeParameters(2,
                engine->mode_freq_multipliers[2], value, engine->mode_weights[2]);
            break;

        case kParam_Mode2_Weight:
            engine->mode_weights[2] = value;
            engine->voice_allocator->setModeParameters(2,
                engine->mode_freq_multipliers[2], engine->mode_dampings[2], value);
            break;

        // Mode 3 parameters
        case kParam_Mode3_Frequency:
            engine->mode_freq_multipliers[3] = value;
            engine->voice_allocator->setModeParameters(3, value,
                engine->mode_dampings[3], engine->mode_weights[3]);
            break;

        case kParam_Mode3_Damping:
            engine->mode_dampings[3] = value;
            engine->voice_allocator->setModeParameters(3,
                engine->mode_freq_multipliers[3], value, engine->mode_weights[3]);
            break;

        case kParam_Mode3_Weight:
            engine->mode_weights[3] = value;
            engine->voice_allocator->setModeParameters(3,
                engine->mode_freq_multipliers[3], engine->mode_dampings[3], value);
            break;

        // Poke/excitation parameters
        case kParam_PokeStrength:
            engine->poke_strength = value;
            engine->voice_allocator->setPokeStrength(value);
            break;

        case kParam_PokeDuration:
            engine->poke_duration_ms = value;
            engine->voice_allocator->setPokeDuration(value);
            break;

        // Note: Polyphony cannot be changed at runtime (would require reallocation)
        case kParam_Polyphony:
            // Ignore - polyphony is set at initialization only
            break;

        default:
            break;
    }
}
