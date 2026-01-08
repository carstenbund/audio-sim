/**
 * @file VoiceAllocator.h
 * @brief Polyphonic voice allocation and management
 *
 * Manages a pool of ModalVoice instances for polyphonic synthesis.
 * Handles:
 * - Note on/off events
 * - Voice stealing (when all voices are in use)
 * - MIDI note → voice mapping
 */

#ifndef VOICE_ALLOCATOR_H
#define VOICE_ALLOCATOR_H

#include "ModalVoice.h"
#include <cstdint>

/**
 * @brief Default maximum polyphony
 */
#define DEFAULT_MAX_POLYPHONY 16

class VoiceAllocator {
public:
    /**
     * @brief Constructor
     * @param max_polyphony Maximum number of simultaneous voices
     */
    VoiceAllocator(uint32_t max_polyphony = DEFAULT_MAX_POLYPHONY);

    /**
     * @brief Destructor
     */
    ~VoiceAllocator();

    /**
     * @brief Initialize allocator
     * @param sample_rate Sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Handle MIDI note on
     * @param midi_note MIDI note number (0-127)
     * @param velocity Velocity (0-127)
     * @return Pointer to allocated voice, or nullptr if allocation failed
     */
    ModalVoice* noteOn(uint8_t midi_note, uint8_t velocity);

    /**
     * @brief Handle MIDI note off
     * @param midi_note MIDI note number (0-127)
     */
    void noteOff(uint8_t midi_note);

    /**
     * @brief Release all voices
     */
    void allNotesOff();

    /**
     * @brief Apply pitch bend to all active voices
     * @param bend_amount Pitch bend amount (-1.0 to +1.0)
     */
    void setPitchBend(float bend_amount);

    /**
     * @brief Set personality for all voices
     * @param personality Node personality (PERSONALITY_RESONATOR or PERSONALITY_SELF_OSCILLATOR)
     */
    void setPersonality(node_personality_t personality);

    /**
     * @brief Set mode parameters for all voices
     * @param mode_idx Mode index (0-3)
     * @param freq_multiplier Frequency multiplier relative to base
     * @param damping Damping coefficient
     * @param weight Audio weight (0.0-1.0)
     */
    void setModeParameters(uint8_t mode_idx, float freq_multiplier, float damping, float weight);

    /**
     * @brief Set poke strength for future note-on events
     * @param strength Poke strength (0.0-1.0)
     */
    void setPokeStrength(float strength);

    /**
     * @brief Set poke duration for future note-on events
     * @param duration_ms Poke duration in milliseconds
     */
    void setPokeDuration(float duration_ms);

    /**
     * @brief Update all active voices (control rate)
     *
     * Should be called at control rate (500 Hz typically)
     */
    void updateVoices();

    /**
     * @brief Render audio from all active voices
     * @param outL Left channel output buffer
     * @param outR Right channel output buffer
     * @param num_frames Number of frames to render
     */
    void renderAudio(float* outL, float* outR, uint32_t num_frames);

    /**
     * @brief Get voice by index
     * @param voice_idx Voice index [0..max_polyphony-1]
     * @return Pointer to voice, or nullptr if invalid index
     */
    ModalVoice* getVoice(uint32_t voice_idx);

    /**
     * @brief Get maximum polyphony
     * @return Maximum number of voices
     */
    uint32_t getMaxPolyphony() const { return max_polyphony_; }

    /**
     * @brief Get number of active voices
     * @return Number of currently active voices
     */
    uint32_t getActiveVoiceCount() const;

private:
    ModalVoice** voices_;              ///< Voice pool
    uint32_t max_polyphony_;           ///< Maximum polyphony

    int8_t note_to_voice_[128];        ///< MIDI note → voice mapping (-1 = none)
    float pitch_bend_;                 ///< Current pitch bend amount

    // Mode parameters (stored as multipliers for per-voice application)
    float mode_freq_multipliers_[4];   ///< Frequency multipliers per mode
    float mode_dampings_[4];           ///< Damping coefficients per mode
    float mode_weights_[4];            ///< Audio weights per mode

    // Poke parameters (applied at note-on)
    float poke_strength_;              ///< Poke strength for excitation
    float poke_duration_ms_;           ///< Poke duration in milliseconds

    float sample_rate_;                ///< Current sample rate
    bool initialized_;                 ///< Initialization flag

    /**
     * @brief Find free voice for allocation
     * @return Pointer to free voice, or nullptr if none available
     */
    ModalVoice* findFreeVoice();

    /**
     * @brief Steal oldest voice
     * @return Pointer to stolen voice
     */
    ModalVoice* stealOldestVoice();
};

#endif // VOICE_ALLOCATOR_H
