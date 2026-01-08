/**
 * @file VoiceAllocator.cpp
 * @brief Voice allocation and management implementation
 */

#include "VoiceAllocator.h"
#include <cstring>
#include <algorithm>

VoiceAllocator::VoiceAllocator(uint32_t max_polyphony)
    : max_polyphony_(max_polyphony)
    , pitch_bend_(0.0f)
    , sample_rate_(48000.0f)
    , initialized_(false)
{
    // Allocate voice pool
    voices_ = new ModalVoice*[max_polyphony];
    for (uint32_t i = 0; i < max_polyphony; i++) {
        voices_[i] = new ModalVoice(static_cast<uint8_t>(i));
    }

    // Initialize note mapping to -1 (no voice assigned)
    memset(note_to_voice_, -1, sizeof(note_to_voice_));
}

VoiceAllocator::~VoiceAllocator() {
    // Delete all voices
    if (voices_) {
        for (uint32_t i = 0; i < max_polyphony_; i++) {
            delete voices_[i];
        }
        delete[] voices_;
    }
}

void VoiceAllocator::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize all voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        voices_[i]->initialize(sample_rate);
    }

    initialized_ = true;
}

ModalVoice* VoiceAllocator::noteOn(uint8_t midi_note, uint8_t velocity) {
    if (!initialized_ || midi_note > 127) return nullptr;

    // Check if this note is already playing
    int8_t existing_voice = note_to_voice_[midi_note];
    if (existing_voice >= 0) {
        // Re-trigger existing voice
        ModalVoice* voice = voices_[existing_voice];
        float vel_normalized = velocity / 127.0f;
        voice->noteOn(midi_note, vel_normalized);
        voice->setPitchBend(pitch_bend_);
        return voice;
    }

    // Find free voice
    ModalVoice* voice = findFreeVoice();
    if (!voice) {
        // No free voices, steal oldest
        voice = stealOldestVoice();
    }

    if (voice) {
        // Allocate voice
        float vel_normalized = velocity / 127.0f;
        voice->noteOn(midi_note, vel_normalized);
        voice->setPitchBend(pitch_bend_);

        // Update mapping
        for (uint32_t i = 0; i < max_polyphony_; i++) {
            if (voices_[i] == voice) {
                note_to_voice_[midi_note] = static_cast<int8_t>(i);
                break;
            }
        }
    }

    return voice;
}

void VoiceAllocator::noteOff(uint8_t midi_note) {
    if (midi_note > 127) return;

    int8_t voice_idx = note_to_voice_[midi_note];
    if (voice_idx >= 0 && voice_idx < static_cast<int8_t>(max_polyphony_)) {
        voices_[voice_idx]->noteOff();
        note_to_voice_[midi_note] = -1;
    }
}

void VoiceAllocator::allNotesOff() {
    // Release all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->noteOff();
        }
    }

    // Clear note mapping
    memset(note_to_voice_, -1, sizeof(note_to_voice_));
}

void VoiceAllocator::setPitchBend(float bend_amount) {
    pitch_bend_ = bend_amount;

    // Apply to all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->setPitchBend(bend_amount);
        }
    }
}

void VoiceAllocator::setPersonality(node_personality_t personality) {
    // Apply to all voices (both active and inactive)
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        voices_[i]->setPersonality(personality);
    }
}

void VoiceAllocator::updateVoices() {
    if (!initialized_) return;

    // Update all active voices at control rate
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->updateModal();
        }
    }
}

void VoiceAllocator::renderAudio(float* outL, float* outR, uint32_t num_frames) {
    if (!initialized_) {
        // Return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    // Clear output buffers
    memset(outL, 0, num_frames * sizeof(float));
    memset(outR, 0, num_frames * sizeof(float));

    // Temporary buffers for each voice
    float* tempL = new float[num_frames];
    float* tempR = new float[num_frames];

    // Mix all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            // Render voice
            voices_[i]->renderAudio(tempL, tempR, num_frames);

            // Mix into output
            for (uint32_t j = 0; j < num_frames; j++) {
                outL[j] += tempL[j];
                outR[j] += tempR[j];
            }
        }
    }

    // Clean up
    delete[] tempL;
    delete[] tempR;
}

ModalVoice* VoiceAllocator::getVoice(uint32_t voice_idx) {
    if (voice_idx >= max_polyphony_) return nullptr;
    return voices_[voice_idx];
}

uint32_t VoiceAllocator::getActiveVoiceCount() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            count++;
        }
    }
    return count;
}

ModalVoice* VoiceAllocator::findFreeVoice() {
    // Find first inactive voice
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (!voices_[i]->isActive()) {
            return voices_[i];
        }
    }
    return nullptr;
}

ModalVoice* VoiceAllocator::stealOldestVoice() {
    // Find oldest active voice
    ModalVoice* oldest = nullptr;
    uint32_t max_age = 0;

    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            uint32_t age = voices_[i]->getAge();
            if (age > max_age) {
                max_age = age;
                oldest = voices_[i];
            }
        }
    }

    // Force release the oldest voice
    if (oldest) {
        oldest->reset();
    }

    return oldest;
}
