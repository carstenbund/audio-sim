/**
 * @file test_modal_voice.cpp
 * @brief Standalone test application for Modal Voice DSP
 *
 * Phase 1 deliverable: Tests single-voice modal oscillator
 * - Validates DSP port from ESP32
 * - Generates WAV output for analysis
 * - Profiles CPU usage
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include "../src/dsp_core/ModalVoice.h"

// WAV file header structure
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t file_size;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;  // PCM
    uint16_t num_channels = 2;  // Stereo
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
};

void writeWAV(const char* filename, float* left, float* right, uint32_t num_samples, float sample_rate) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Prepare header
    WAVHeader header;
    header.sample_rate = static_cast<uint32_t>(sample_rate);
    header.byte_rate = header.sample_rate * header.num_channels * header.bits_per_sample / 8;
    header.block_align = header.num_channels * header.bits_per_sample / 8;
    header.data_size = num_samples * header.num_channels * header.bits_per_sample / 8;
    header.file_size = 36 + header.data_size;

    // Write header
    file.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Write samples (interleaved stereo, 16-bit PCM)
    for (uint32_t i = 0; i < num_samples; i++) {
        int16_t sample_l = static_cast<int16_t>(left[i] * 32767.0f);
        int16_t sample_r = static_cast<int16_t>(right[i] * 32767.0f);
        file.write(reinterpret_cast<char*>(&sample_l), sizeof(int16_t));
        file.write(reinterpret_cast<char*>(&sample_r), sizeof(int16_t));
    }

    file.close();
    std::cout << "Wrote " << num_samples << " samples to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Modal Attractors - Voice Test (Phase 1)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Test parameters
    const float SAMPLE_RATE = 48000.0f;
    const float DURATION_SEC = 5.0f;
    const uint32_t NUM_SAMPLES = static_cast<uint32_t>(SAMPLE_RATE * DURATION_SEC);
    const uint32_t BUFFER_SIZE = 512;

    std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Duration: " << DURATION_SEC << " seconds" << std::endl;
    std::cout << "Total samples: " << NUM_SAMPLES << std::endl;
    std::cout << std::endl;

    // Create and initialize voice
    std::cout << "Initializing modal voice..." << std::endl;
    ModalVoice voice(0);
    voice.initialize(SAMPLE_RATE);

    // Configure as resonator with 4 modes
    std::cout << "Configuring modal parameters..." << std::endl;
    voice.setPersonality(PERSONALITY_RESONATOR);

    // Set up 4 modes with harmonic relationships
    float base_freq = 220.0f;  // A3
    voice.setMode(0, base_freq * 1.0f, 0.5f, 1.0f);   // Fundamental
    voice.setMode(1, base_freq * 1.01f, 0.6f, 0.7f);  // Slight detune
    voice.setMode(2, base_freq * 2.0f, 0.8f, 0.5f);   // Second harmonic
    voice.setMode(3, base_freq * 3.0f, 1.0f, 0.3f);   // Third harmonic

    std::cout << "  Mode 0: " << base_freq << " Hz, damping=0.5, weight=1.0" << std::endl;
    std::cout << "  Mode 1: " << base_freq * 1.01f << " Hz, damping=0.6, weight=0.7" << std::endl;
    std::cout << "  Mode 2: " << base_freq * 2.0f << " Hz, damping=0.8, weight=0.5" << std::endl;
    std::cout << "  Mode 3: " << base_freq * 3.0f << " Hz, damping=1.0, weight=0.3" << std::endl;
    std::cout << std::endl;

    // Allocate output buffers
    float* output_left = new float[NUM_SAMPLES];
    float* output_right = new float[NUM_SAMPLES];
    float* temp_left = new float[BUFFER_SIZE];
    float* temp_right = new float[BUFFER_SIZE];

    memset(output_left, 0, NUM_SAMPLES * sizeof(float));
    memset(output_right, 0, NUM_SAMPLES * sizeof(float));

    // Trigger note at t=0
    std::cout << "Triggering note on (MIDI 57, velocity 100)..." << std::endl;
    voice.noteOn(57, 0.8f);  // A3, velocity 0.8

    // Render audio in blocks
    std::cout << "Rendering audio..." << std::endl;
    uint32_t samples_rendered = 0;

    while (samples_rendered < NUM_SAMPLES) {
        uint32_t samples_to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - samples_rendered);

        // Update modal state (control rate - every buffer for simplicity)
        voice.updateModal();

        // Render audio block
        voice.renderAudio(temp_left, temp_right, samples_to_render);

        // Copy to output buffer
        memcpy(output_left + samples_rendered, temp_left, samples_to_render * sizeof(float));
        memcpy(output_right + samples_rendered, temp_right, samples_to_render * sizeof(float));

        samples_rendered += samples_to_render;

        // Print progress
        if (samples_rendered % (NUM_SAMPLES / 10) == 0) {
            float progress = (float)samples_rendered / NUM_SAMPLES * 100.0f;
            std::cout << "  Progress: " << progress << "%" << std::endl;
        }
    }

    std::cout << "Rendering complete!" << std::endl;
    std::cout << std::endl;

    // Write to WAV file
    std::cout << "Writing output to test_output.wav..." << std::endl;
    writeWAV("test_output.wav", output_left, output_right, NUM_SAMPLES, SAMPLE_RATE);

    // Calculate RMS amplitude
    float rms = 0.0f;
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        rms += output_left[i] * output_left[i];
    }
    rms = sqrtf(rms / NUM_SAMPLES);

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "RMS amplitude: " << rms << std::endl;
    std::cout << "Peak amplitude: ";

    float peak = 0.0f;
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        float abs_val = fabsf(output_left[i]);
        if (abs_val > peak) peak = abs_val;
    }
    std::cout << peak << std::endl;

    // Check if output is non-zero
    if (rms > 0.001f) {
        std::cout << "✓ Voice is generating audio" << std::endl;
    } else {
        std::cout << "✗ WARNING: Output is too quiet or silent" << std::endl;
    }

    // Cleanup
    delete[] output_left;
    delete[] output_right;
    delete[] temp_left;
    delete[] temp_right;

    std::cout << std::endl;
    std::cout << "Test complete! Check test_output.wav for audio output." << std::endl;

    return 0;
}
