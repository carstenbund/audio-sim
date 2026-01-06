# 4-Channel Audio System

**ESP32 Modal Resonator - Multi-Channel Audio Output**

This document describes the 4-channel audio system where each modal oscillator drives its own independent audio channel.

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Hardware Requirements](#hardware-requirements)
4. [Channel Mapping](#channel-mapping)
5. [I2S Configuration](#i2s-configuration)
6. [Audio Synthesis](#audio-synthesis)
7. [Performance Characteristics](#performance-characteristics)
8. [Hardware Setup Examples](#hardware-setup-examples)

---

## Overview

The ESP32 modal resonator now outputs **4 independent audio channels at 48kHz**, with each of the 4 modal oscillators driving its own channel. This enables:

- **Spatial audio**: Each mode can be routed to different speakers
- **Independent processing**: External effects per mode
- **Rich timbral control**: Modes don't interfere in the mix
- **Direct modal-to-audio mapping**: |a_k(t)| → Channel k amplitude

### Key Features

✅ **48kHz sample rate** across all 4 channels
✅ **16-bit PCM** per channel
✅ **TDM/Quad I2S** output format
✅ **Independent phase accumulators** per mode
✅ **Per-mode gain control**
✅ **Amplitude smoothing** to prevent clicks

---

## Architecture

### Signal Flow

```
Modal Node (4 modes)
    ↓
Mode 0: a₀(t) → |a₀| → Sine(ω₀·t + arg(a₀)) → Channel 0
Mode 1: a₁(t) → |a₁| → Sine(ω₁·t + arg(a₁)) → Channel 1
Mode 2: a₂(t) → |a₂| → Sine(ω₂·t + arg(a₂)) → Channel 2
Mode 3: a₃(t) → |a₃| → Sine(ω₃·t + arg(a₃)) → Channel 3
    ↓
4-Channel Interleaved Buffer: [ch0, ch1, ch2, ch3, ch0, ch1, ...]
    ↓
I2S DMA (TDM mode)
    ↓
DAC/Audio Interface
```

### Data Flow

1. **Modal Integration** (Control Task, 500Hz):
   - Updates complex amplitudes a_k(t)
   - Applies coupling, damping, excitation

2. **Audio Synthesis** (Audio Task, 48kHz):
   - Reads |a_k| and arg(a_k) from each mode
   - Generates sinusoid at frequency ω_k
   - Applies amplitude envelope and smoothing
   - Outputs to channel k

3. **I2S Output** (DMA, hardware-driven):
   - Interleaves 4 channels: [ch0, ch1, ch2, ch3, ...]
   - Transmits via TDM format
   - Auto-clears on buffer completion

---

## Hardware Requirements

### Minimum Requirements

- **ESP32** (any variant with I2S peripheral)
- **TDM-capable DAC** or **4× stereo DACs**
- **GPIO pins**: BCK (25), WS (26), DIN (27)

### DAC Options

#### Option 1: TDM DAC (Recommended)

Multi-channel TDM DACs that support 4+ channels:

| Model | Channels | I2S Mode | Notes |
|-------|----------|----------|-------|
| PCM5102A (x2) | 2×2 = 4 | Stereo | Use 2 I2S peripherals |
| CS4385 | 8 | TDM | Professional, expensive |
| AK4458 | 8 | TDM | High-end audio |

#### Option 2: Quad DAC Boards

Use 4 mono DACs or 2 stereo DACs:

```
ESP32 I2S (TDM) → TDM Decoder → 4× DACs → 4× Outputs
```

**Example TDM Decoder**: PCM1865 (ADC with TDM) can be used in reverse

#### Option 3: Multi-I2S (ESP32-S3)

ESP32-S3 has 2 I2S peripherals:
- I2S0 → Channels 0,1 (stereo)
- I2S1 → Channels 2,3 (stereo)

---

## Channel Mapping

### Mode → Channel Assignment

| Channel | Mode | Default Frequency | Role |
|---------|------|-------------------|------|
| **0** | Mode 0 | 440 Hz (A4) | Carrier / fundamental |
| **1** | Mode 1 | 442 Hz | Detuning / beating |
| **2** | Mode 2 | 880 Hz (A5) | Brightness / timbre |
| **3** | Mode 3 | 55 Hz | Sub-bass / structure |

### Interleaved Buffer Format

**Buffer size**: 480 samples/channel × 4 channels = **1920 int16_t samples**

**Memory layout** (10ms buffer @ 48kHz):
```
[sample 0]:  ch0[0], ch1[0], ch2[0], ch3[0]
[sample 1]:  ch0[1], ch1[1], ch2[1], ch3[1]
[sample 2]:  ch0[2], ch1[2], ch2[2], ch3[2]
...
[sample 479]: ch0[479], ch1[479], ch2[479], ch3[479]
```

**Index calculation**:
```c
buffer_index = sample_idx * NUM_CHANNELS + channel_idx;
```

---

## I2S Configuration

### TDM Mode Settings

```c
i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,
    .sample_rate = 48000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_MULTIPLE,  // Multi-channel
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 480 * 4,  // 480 samples/ch × 4 ch
    .use_apll = true,        // Better clock accuracy
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
    .bits_cfg = {
        .sample_bits = 16,
        .slot_bits = 16,
        .slot_mode = I2S_SLOT_MODE_QUAD  // 4 channels
    }
};
```

### Pin Configuration

| Pin | GPIO | Function | Signal |
|-----|------|----------|--------|
| BCK | 25 | Bit Clock | 48kHz × 16 bits × 4 ch = 3.072 MHz |
| WS | 26 | Word Select | 48kHz frame sync |
| DIN | 27 | Data Out | Serial audio data |

### Clock Rates

- **Sample Rate**: 48,000 Hz
- **Bit Clock**: 48,000 × 16 × 4 = 3.072 MHz
- **MCLK**: 48,000 × 256 = 12.288 MHz

---

## Audio Synthesis

### Per-Mode Synthesis

Each mode k generates its own sinusoid:

```c
// For each mode k = 0, 1, 2, 3:
float amplitude = |a_k(t)| × weight_k × mode_gain_k × master_gain
float frequency = ω_k / (2π)  // Mode frequency in Hz
float phase = φ_accumulator_k + arg(a_k)  // Phase coherence

sample_k = amplitude × sin(phase)
channel_k_output = (int16_t)(sample_k × 32767)
```

### Amplitude Smoothing

To prevent clicks when amplitude changes suddenly (e.g., on poke events):

```c
amplitude_smooth_k += α × (amplitude_raw_k - amplitude_smooth_k)
```

where α = 0.12 (smoothing coefficient, ~8ms time constant @ 48kHz)

### Phase Coherence

The audio synthesis uses the complex amplitude's phase:

```c
mode_phase = arg(a_k)  // Phase from modal dynamics
phase_total = phase_accumulator + mode_phase
```

This ensures the audio output phase-locks to the modal oscillator's phase, maintaining coherence across the network.

---

## Performance Characteristics

### CPU Usage

| Task | Core | Frequency | CPU Load |
|------|------|-----------|----------|
| Audio synthesis | Core 1 | 48kHz | ~15% |
| Control loop | Core 0 | 500Hz | ~2% |
| Network | Core 0 | Event | <1% |

**Total**: ~18% CPU (plenty of headroom)

### Memory Usage

| Component | Size | Location |
|-----------|------|----------|
| Audio buffer | 3.84 KB | Stack |
| DMA buffers | 15.36 KB | DMA RAM |
| Synthesis state | 96 bytes | Static |
| Modal node state | 256 bytes | Static |

**Total**: ~19.6 KB

### Latency

| Stage | Latency |
|-------|---------|
| Modal integration | 2 ms (500Hz) |
| Audio buffering | 10 ms (DMA buffer) |
| I2S transmission | <1 ms |
| **Total** | **~13 ms** |

---

## Hardware Setup Examples

### Example 1: Stereo Output (2 channels)

Use only channels 0 and 1, ignore channels 2 and 3:

```c
// Disable modes 2 and 3
audio_synth_set_mode_gain(&synth, 2, 0.0f);
audio_synth_set_mode_gain(&synth, 3, 0.0f);
```

**Hardware**: Standard stereo DAC (PCM5102A)
- Left channel ← Mode 0
- Right channel ← Mode 1

### Example 2: Quad Speaker Array

**Hardware**: 4× mono amplifiers + speakers

**Spatial layout**:
```
    Speaker 2 (Mode 2, 880Hz)
          ↑
Speaker 1 ← Center → Speaker 0
  (Mode 1)         (Mode 0, 440Hz)
          ↓
    Speaker 3 (Mode 3, 55Hz)
```

**Result**: Spatial separation of frequency components

### Example 3: External Effects Chain

**Hardware**: 4 channels → 4× effect pedals → mixer

```
Mode 0 (440Hz) → Reverb → Mixer Ch1
Mode 1 (442Hz) → Chorus → Mixer Ch2
Mode 2 (880Hz) → Distortion → Mixer Ch3
Mode 3 (55Hz) → Compressor → Mixer Ch4
```

**Result**: Independent processing per frequency band

### Example 4: Recording/Analysis

**Hardware**: USB audio interface (4+ channels)

**Use case**: Record each mode separately for:
- Spectral analysis
- Modal amplitude tracking
- Network dynamics visualization
- Post-production mixing

---

## Code Examples

### Initialize Audio System

```c
#include "audio/audio_synth.h"
#include "core/modal_node.h"

modal_node_t node;
audio_synth_t synth;

void setup_audio(void) {
    // Initialize modal node with 4 modes
    modal_node_init(&node);
    modal_node_set_mode(&node, 0, freq_to_omega(440.0f), 0.5f, 1.0f);  // A4
    modal_node_set_mode(&node, 1, freq_to_omega(442.0f), 0.6f, 0.8f);  // Detuned
    modal_node_set_mode(&node, 2, freq_to_omega(880.0f), 1.0f, 0.3f);  // A5
    modal_node_set_mode(&node, 3, freq_to_omega(55.0f), 0.1f, 0.5f);   // A1

    // Initialize audio synthesis (frequencies from mode parameters)
    audio_synth_init(&synth, &node);

    // Initialize I2S in 4-channel mode
    audio_i2s_init();
}
```

### Adjust Per-Mode Gain

```c
// Solo mode 0 (mute others)
audio_synth_set_mode_gain(&synth, 0, 1.0f);  // Full
audio_synth_set_mode_gain(&synth, 1, 0.0f);  // Mute
audio_synth_set_mode_gain(&synth, 2, 0.0f);  // Mute
audio_synth_set_mode_gain(&synth, 3, 0.0f);  // Mute

// Or balance modes:
audio_synth_set_mode_gain(&synth, 0, 0.8f);  // Carrier
audio_synth_set_mode_gain(&synth, 1, 0.6f);  // Detune
audio_synth_set_mode_gain(&synth, 2, 0.3f);  // Brightness
audio_synth_set_mode_gain(&synth, 3, 0.5f);  // Sub
```

### Audio Task (FreeRTOS)

```c
void audio_task(void* params) {
    audio_synth_t* synth = (audio_synth_t*)params;

    while (1) {
        // Generate 10ms of 4-channel audio
        int16_t* buffer = audio_synth_generate_buffer(synth);

        // Write to I2S (blocks until DMA ready)
        audio_i2s_write(buffer, AUDIO_BUFFER_SIZE * sizeof(int16_t));
    }
}
```

---

## Troubleshooting

### No Audio Output

**Check**:
1. I2S pins connected correctly (BCK=25, WS=26, DIN=27)
2. DAC powered and configured
3. Modes are active: `modal_node_set_mode(...)` called
4. Not muted: `audio_synth_set_mute(&synth, false)`
5. Master gain > 0: `audio_synth_set_gain(&synth, 0.7f)`

### Distorted Audio

**Causes**:
- Amplitude too high (clipping)
- Master gain > 1.0
- Multiple modes with high amplitudes

**Fix**:
```c
// Reduce master gain
audio_synth_set_gain(&synth, 0.5f);

// Or reduce per-mode gains
for (int k = 0; k < 4; k++) {
    audio_synth_set_mode_gain(&synth, k, 0.6f);
}
```

### Only 2 Channels Working

**Cause**: DAC doesn't support TDM/quad mode

**Fix**: Use stereo mode instead:
```c
// Disable channels 2 and 3
audio_synth_set_mode_gain(&synth, 2, 0.0f);
audio_synth_set_mode_gain(&synth, 3, 0.0f);
```

### Clicking/Popping Sounds

**Cause**: Amplitude discontinuities

**Fix**: Amplitude smoothing is automatic, but ensure:
```c
// SMOOTH_ALPHA is set correctly (audio_synth.c)
#define SMOOTH_ALPHA 0.12f  // ~8ms time constant
```

### Wrong Channel Output

**Check**: Channel mapping in DAC
- Some DACs have non-standard TDM slot ordering
- Verify with oscilloscope or audio analyzer

---

## Future Enhancements

### Planned Features

- [ ] Dynamic channel routing (map any mode to any channel)
- [ ] Per-channel EQ/filtering
- [ ] Channel crossfade/morphing
- [ ] Spatial audio panning (HRTF)
- [ ] Higher bit depth (24-bit, 32-bit float)
- [ ] Higher sample rates (96kHz, 192kHz)

### Hardware Expansion

- [ ] Support for I2S MEMS microphones (input)
- [ ] Bluetooth audio output (A2DP)
- [ ] USB audio class (UAC2)
- [ ] SPDIF optical output

---

## References

- ESP32 I2S Documentation: [esp-idf/api-reference/peripherals/i2s](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- TDM Format: [I2S Bus Specification](https://www.sparkfun.com/datasheets/BreakoutBoards/I2SBUS.pdf)
- Modal Synthesis: [Design Spec](../DESIGN_SPEC.md)
- Implementation Details: [../IMPLEMENTATION.md](../IMPLEMENTATION.md)

---

**Last Updated**: 2026-01-06
**Version**: 1.0 (4-channel audio system)
