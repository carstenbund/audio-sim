/**
 * @file ModalVoice.h
 * @brief C++ wrapper for modal_node C core
 *
 * Provides object-oriented interface to the ported ESP32 modal oscillator.
 * Adds AU-specific features:
 * - MIDI note/velocity tracking
 * - Pitch bend support
 * - Voice state management
 */

#ifndef MODAL_VOICE_H
#define MODAL_VOICE_H

#include "../esp32_port/modal_node.h"
#include "../esp32_port/audio_synth.h"
#include <cstdint>
#include <complex>

class ModalVoice {
public:
    /**
     * @brief Voice state enumeration
     */
    enum class State {
        Inactive,   ///< Voice not playing
        Attack,     ///< Note on, attack phase
        Sustain,    ///< Sustaining (self-oscillator only)
        Release     ///< Note off, release phase
    };

    /**
     * @brief Constructor
     * @param voice_id Unique voice identifier (0-15 typically)
     */
    ModalVoice(uint8_t voice_id);

    /**
     * @brief Destructor
     */
    ~ModalVoice();

    /**
     * @brief Initialize voice with sample rate
     * @param sample_rate Sample rate in Hz
     */
    void initialize(float sample_rate);

    /**
     * @brief Trigger note on
     * @param midi_note MIDI note number (0-127)
     * @param velocity Note velocity (0.0-1.0)
     */
    void noteOn(uint8_t midi_note, float velocity);

    /**
     * @brief Trigger note off
     */
    void noteOff();

    /**
     * @brief Apply pitch bend
     * @param bend_amount Pitch bend amount (-1.0 to +1.0)
     * @param bend_range Pitch bend range in semitones (default 2.0)
     */
    void setPitchBend(float bend_amount, float bend_range = 2.0f);

    /**
     * @brief Update modal state (call at control rate)
     */
    void updateModal();

    /**
     * @brief Render audio block
     * @param outL Left channel output
     * @param outR Right channel output
     * @param num_frames Number of frames to render
     */
    void renderAudio(float* outL, float* outR, uint32_t num_frames);

    /**
     * @brief Apply coupling input from other voices
     * @param coupling_inputs Array of 4 coupling inputs (one per mode)
     */
    void applyCoupling(const float coupling_inputs[MAX_MODES]);

    /**
     * @brief Get voice state
     * @return Current voice state
     */
    State getState() const { return state_; }

    /**
     * @brief Check if voice is active
     * @return True if voice is playing
     */
    bool isActive() const {
        return state_ != State::Inactive;
    }

    /**
     * @brief Get MIDI note number
     * @return Current MIDI note
     */
    uint8_t getMIDINote() const { return midi_note_; }

    /**
     * @brief Get note velocity
     * @return Velocity (0.0-1.0)
     */
    float getVelocity() const { return velocity_; }

    /**
     * @brief Get voice age (for voice stealing)
     * @return Number of update cycles since note on
     */
    uint32_t getAge() const { return age_; }

    /**
     * @brief Get current amplitude
     * @return Amplitude (0.0-1.0)
     */
    float getAmplitude() const;

    /**
     * @brief Get mode 0 complex amplitude (for coupling broadcast)
     * @return Complex amplitude as C++ std::complex
     */
    std::complex<float> getMode0Amplitude() const {
        modal_complexf_t c = modal_node_get_mode0(&node_);
        return std::complex<float>(c.re, c.im);
    }

    /**
     * @brief Set mode parameters
     * @param mode_idx Mode index (0-3)
     * @param freq_hz Frequency in Hz
     * @param damping Damping coefficient
     * @param weight Audio weight (0.0-1.0)
     */
    void setMode(uint8_t mode_idx, float freq_hz, float damping, float weight);

    /**
     * @brief Set node personality
     * @param personality Resonator or self-oscillator
     */
    void setPersonality(node_personality_t personality);

    /**
     * @brief Reset voice state
     */
    void reset();

private:
    uint8_t voice_id_;              ///< Voice identifier
    modal_node_t node_;             ///< Core modal node (C struct)
    audio_synth_t synth_;           ///< Audio synthesis state

    State state_;                   ///< Voice state
    uint8_t midi_note_;             ///< Current MIDI note
    float velocity_;                ///< Note velocity (0.0-1.0)
    float pitch_bend_;              ///< Pitch bend amount (-1.0 to +1.0)

    uint32_t age_;                  ///< Voice age counter
    uint32_t samples_since_update_; ///< Sample counter for control rate
    uint32_t samples_per_update_;   ///< Samples between control updates

    float sample_rate_;             ///< Current sample rate

    /**
     * @brief Update mode frequencies based on MIDI note and pitch bend
     */
    void updateFrequencies();

    /**
     * @brief Update voice state machine
     */
    void updateState();
};

#endif // MODAL_VOICE_H
