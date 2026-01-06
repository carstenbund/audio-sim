# ESP32 Modal Resonator - Testing & Rollout Guide

**Incremental testing strategy for distributed modal resonator network**

## Overview

This guide follows a careful incremental approach:
1. **Phase 1**: Single node - syntax & basic systems
2. **Phase 2**: Two nodes - protocol & messaging
3. **Phase 3**: Single node + audio - hear the network
4. **Phase 4**: Two nodes + audio - full distributed system
5. **Phase 5**: Hub + MIDI integration

Each phase validates specific subsystems before adding complexity.

---

## Prerequisites

### Hardware
- ESP32 development boards (start with 2)
- USB cables for flashing/monitoring
- (Phase 3+) Audio DAC hardware:
  - **Option A**: 2× PCM5102A stereo DACs (4 channels total)
  - **Option B**: TDM-capable DAC (CS4385, AK4458)
  - **Option C**: Single stereo DAC (test with 2 channels only)
- Speakers or headphones for audio testing

### Software
- ESP-IDF v4.4+ installed
- Serial monitor (screen, minicom, or `idf.py monitor`)
- (Optional) Audio analysis tools: Audacity, Sonic Visualiser

### Build Setup

```bash
cd esp32/firmware
idf.py set-target esp32
idf.py menuconfig  # Configure node-specific settings
idf.py build
```

---

## Phase 1: Single Node - Syntax Testing

**Goal**: Verify firmware compiles and basic systems initialize correctly.

**Hardware**: 1× ESP32

### 1.1 Configuration

Edit `main/main.c`:
```c
#define MY_NODE_ID 0  // Node A
```

### 1.2 Build and Flash

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 1.3 Expected Output

Look for these lines in serial output:

```
I (1234) MODAL_NODE: Initializing modal node
I (1235) MODAL_NODE: Node personality: RESONATOR
I (1236) MODAL_NODE: Control task started on core 0
I (1237) AUDIO_I2S: Initializing I2S driver for 4-channel output
I (1238) AUDIO_I2S: I2S driver initialized successfully
I (1239) AUDIO_I2S:   Sample rate: 48000 Hz
I (1240) AUDIO_I2S:   Channels: 4 (quad/TDM mode)
I (1241) AUDIO_I2S:   Mode 0 → Channel 0, Mode 1 → Channel 1
I (1242) AUDIO_I2S: Audio task started on core 1
I (1243) ESP_NOW_MGR: ESP-NOW initialized
I (1244) MODAL_NODE: System initialization complete
```

### 1.4 Verification Tests

#### Test 1.4.1: Task Scheduling
Watch for periodic control updates:
```
I (2000) MODAL_NODE: Control loop running (500 Hz)
```

#### Test 1.4.2: Modal State
Add debug output to `control_task()` in `main.c`:

```c
// In control_task(), inside the while loop:
static uint32_t debug_counter = 0;
if (++debug_counter % 500 == 0) {  // Every 1 second
    ESP_LOGI(TAG, "Modal amplitudes: |a0|=%.3f |a1|=%.3f |a2|=%.3f |a3|=%.3f",
             cabsf(g_node.modes[0].a),
             cabsf(g_node.modes[1].a),
             cabsf(g_node.modes[2].a),
             cabsf(g_node.modes[3].a));
}
```

**Expected**: Amplitudes should decay to ~0.0 (resonator personality, no excitation)

#### Test 1.4.3: Self-Test Poke
Add manual poke injection to test modal response:

```c
// In app_main(), after initialization:
vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds

// Send manual poke to self
poke_event_t poke = {
    .source_node_id = MY_NODE_ID,
    .strength = 1.0f,
    .phase_hint = 0.0f
};
float weights[4] = {1.0, 0.8, 0.3, 0.5};
memcpy(poke.mode_weights, weights, sizeof(weights));

xQueueSend(g_poke_queue, &poke, 0);
ESP_LOGI(TAG, "Self-poke injected");
```

**Expected**: Modal amplitudes spike then decay exponentially

### 1.5 Success Criteria

- ✅ Firmware builds without errors
- ✅ All 3 tasks start (Audio, Control, Network)
- ✅ No crashes or resets in first 60 seconds
- ✅ Modal amplitudes decay as expected
- ✅ Self-poke causes amplitude spike

**If tests fail**: Check serial output for errors, verify ESP-IDF version

---

## Phase 2: Two Nodes - Protocol Testing

**Goal**: Verify ESP-NOW peer discovery and message delivery.

**Hardware**: 2× ESP32

### 2.1 Configuration

**Node A** (`main/main.c`):
```c
#define MY_NODE_ID 0
```

**Node B** (separate build/flash):
```c
#define MY_NODE_ID 1
```

### 2.2 Build and Flash Both Nodes

```bash
# Node A
idf.py -p /dev/ttyUSB0 flash monitor

# Node B (in separate terminal)
idf.py -p /dev/ttyUSB1 flash monitor
```

### 2.3 Test 2.3.1: Peer Discovery

**Expected on both nodes**:
```
I (1500) ESP_NOW_MGR: ESP-NOW initialized
I (1501) ESP_NOW_MGR: My MAC: AA:BB:CC:DD:EE:FF
```

Nodes should automatically discover each other (if implementing discovery). If manual peer addition is needed:

```c
// In app_main(), add peer manually for testing:
uint8_t peer_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};  // Other node's MAC
esp_now_add_peer(&g_network, (MY_NODE_ID == 0) ? 1 : 0, peer_mac);
```

### 2.4 Test 2.4.2: Manual Poke Transmission

Add to Node A's `app_main()`:

```c
// After 5 seconds, send poke to Node B
vTaskDelay(pdMS_TO_TICKS(5000));

network_message_t msg;
float weights[4] = {1.0, 0.8, 0.3, 0.5};
size_t msg_size = protocol_create_poke(&msg, 0, 1, 0.8f, -1.0f, weights);

if (esp_now_send_message(&g_network, 1, &msg, msg_size)) {
    ESP_LOGI(TAG, "Poke sent to Node B");
} else {
    ESP_LOGE(TAG, "Failed to send poke");
}
```

**Node A expected**:
```
I (5000) MODAL_NODE: Poke sent to Node B
I (5001) ESP_NOW_MGR: TX: type=0x30 to=1 len=36
```

**Node B expected**:
```
I (5001) ESP_NOW_MGR: RX: type=0x30 from=0 len=36
I (5001) MODAL_NODE: Received POKE from node 0, strength=0.800
I (5001) MODAL_NODE: Modal amplitudes: |a0|=0.800 |a1|=0.640 |a2|=0.240 |a3|=0.400
```

### 2.5 Test 2.5.3: Bidirectional Messaging

Modify both nodes to send pokes every 2 seconds:

```c
// Add to control_task() on both nodes:
static uint32_t poke_counter = 0;
if (++poke_counter % 1000 == 0) {  // Every 2 seconds @ 500Hz
    network_message_t msg;
    float weights[4] = {0.5, 0.5, 0.5, 0.5};
    uint8_t target = (MY_NODE_ID == 0) ? 1 : 0;

    protocol_create_poke(&msg, MY_NODE_ID, target, 0.3f, -1.0f, weights);
    esp_now_send_message(&g_network, target, &msg, sizeof(msg_poke_t));
}
```

**Expected**: Both nodes show increasing amplitudes, creating ping-pong dynamics.

### 2.6 Test 2.6.4: Network Statistics

Add statistics logging to `control_task()`:

```c
static uint32_t stats_counter = 0;
if (++stats_counter % 2500 == 0) {  // Every 5 seconds
    ESP_LOGI(TAG, "ESP-NOW stats: TX=%d RX=%d Lost=%d",
             g_network.tx_count, g_network.rx_count, g_network.tx_fail_count);
}
```

**Expected**: TX and RX counts increase, Lost = 0

### 2.7 Test 2.7.5: Latency Measurement

Add timestamp to poke messages:

```c
// In protocol.h, add to msg_poke_t:
uint32_t send_timestamp_ms;

// Node A: Set timestamp before send
msg.poke.send_timestamp_ms = esp_timer_get_time() / 1000;

// Node B: Calculate latency on receive
uint32_t now_ms = esp_timer_get_time() / 1000;
uint32_t latency_ms = now_ms - msg->poke.send_timestamp_ms;
ESP_LOGI(TAG, "Poke latency: %d ms", latency_ms);
```

**Expected**: Latency < 5ms

### 2.8 Success Criteria

- ✅ Both nodes discover each other
- ✅ Poke messages delivered successfully
- ✅ Node B responds to pokes (amplitudes increase)
- ✅ No packet loss over 100+ messages
- ✅ Latency < 5ms
- ✅ Bidirectional ping-pong works

**If tests fail**:
- Check MAC addresses in logs
- Verify both nodes on same WiFi channel
- Reduce distance between nodes
- Check RSSI (should be > -70 dBm)

---

## Phase 3: Single Node + Audio - "Hear the Network"

**Goal**: Verify 4-channel audio output works correctly.

**Hardware**:
- 1× ESP32
- Audio DAC (2× PCM5102A or TDM DAC)
- 4× speakers/headphones

### 3.1 Hardware Wiring

**For 2× PCM5102A (stereo DACs)**:

```
ESP32          DAC #1 (Ch 0,1)    DAC #2 (Ch 2,3)
------         ----------------    ----------------
GPIO 25  BCK → BCK                 BCK
GPIO 26  WS  → LRCK                LRCK
GPIO 27  DIN → DIN                 (not connected)*
GPIO 14  DIN2→ (not connected)     DIN

*Note: ESP32 I2S has only one DIN pin. For true 4-channel,
       you need TDM DAC or ESP32-S3 with dual I2S.
```

**For testing with single stereo DAC**: Only channels 0,1 will work.

### 3.2 Test 3.2.1: Channel Isolation

Add channel test routine to `app_main()`:

```c
void test_channel_isolation(void) {
    ESP_LOGI(TAG, "=== Channel Isolation Test ===");

    for (int k = 0; k < 4; k++) {
        ESP_LOGI(TAG, "Testing Channel %d (Mode %d: %.0f Hz)",
                 k, k, g_node.modes[k].params.omega / (2 * M_PI));

        // Mute all channels
        for (int i = 0; i < 4; i++) {
            audio_synth_set_mode_gain(&g_audio, i, 0.0f);
        }

        // Unmute current channel
        audio_synth_set_mode_gain(&g_audio, k, 0.7f);

        // Inject poke to excite this mode
        poke_event_t poke = {
            .source_node_id = MY_NODE_ID,
            .strength = 1.0f,
            .phase_hint = 0.0f
        };
        float weights[4] = {0, 0, 0, 0};
        weights[k] = 1.0f;
        memcpy(poke.mode_weights, weights, sizeof(weights));
        xQueueSend(g_poke_queue, &poke, 0);

        // Listen for 3 seconds
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // Restore all channels
    for (int i = 0; i < 4; i++) {
        audio_synth_set_mode_gain(&g_audio, i, 0.7f);
    }

    ESP_LOGI(TAG, "=== Channel Test Complete ===");
}

// Call from app_main():
vTaskDelay(pdMS_TO_TICKS(5000));  // Wait for system stabilization
test_channel_isolation();
```

**Expected Audio**:
- **Channel 0 (Mode 0)**: 440 Hz tone, decays over ~2 seconds
- **Channel 1 (Mode 1)**: 442 Hz tone, decays over ~1.7 seconds
- **Channel 2 (Mode 2)**: 880 Hz tone (octave up), decays over ~1 second
- **Channel 3 (Mode 3)**: 55 Hz low rumble, decays over ~10 seconds

### 3.3 Test 3.3.2: Beating Test

Enable channels 0 and 1 together:

```c
void test_beating(void) {
    ESP_LOGI(TAG, "=== Beating Test (Ch 0+1: 440Hz + 442Hz = 2Hz beat) ===");

    // Enable only ch 0 and 1
    audio_synth_set_mode_gain(&g_audio, 0, 0.7f);
    audio_synth_set_mode_gain(&g_audio, 1, 0.7f);
    audio_synth_set_mode_gain(&g_audio, 2, 0.0f);
    audio_synth_set_mode_gain(&g_audio, 3, 0.0f);

    // Poke both modes equally
    poke_event_t poke = {
        .source_node_id = MY_NODE_ID,
        .strength = 1.0f,
        .phase_hint = 0.0f
    };
    float weights[4] = {1.0, 1.0, 0.0, 0.0};
    memcpy(poke.mode_weights, weights, sizeof(weights));
    xQueueSend(g_poke_queue, &poke, 0);

    ESP_LOGI(TAG, "Listen for 2 Hz beating (amplitude modulation)");
    vTaskDelay(pdMS_TO_TICKS(5000));
}
```

**Expected**: Hear 2 Hz amplitude modulation (beating) as 440Hz and 442Hz interfere.

### 3.4 Test 3.4.3: Modal Decay Timing

Measure actual decay time constants:

```c
void test_decay_timing(void) {
    ESP_LOGI(TAG, "=== Decay Timing Test ===");

    // Solo mode 0
    for (int i = 1; i < 4; i++) {
        audio_synth_set_mode_gain(&g_audio, i, 0.0f);
    }
    audio_synth_set_mode_gain(&g_audio, 0, 0.7f);

    // Poke mode 0
    poke_event_t poke = {
        .source_node_id = MY_NODE_ID,
        .strength = 1.0f,
        .phase_hint = 0.0f
    };
    float weights[4] = {1.0, 0.0, 0.0, 0.0};
    memcpy(poke.mode_weights, weights, sizeof(weights));
    xQueueSend(g_poke_queue, &poke, 0);

    // Log amplitude every 100ms
    for (int i = 0; i < 50; i++) {  // 5 seconds
        float amp = cabsf(g_node.modes[0].a);
        ESP_LOGI(TAG, "t=%d ms, |a0|=%.3f", i * 100, amp);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Expected: exponential decay with γ=0.5 → T_63%%=2.0s");
}
```

**Expected**: Amplitude drops to 37% (1/e) in ~2 seconds.

### 3.5 Test 3.5.4: Self-Oscillator Mode

```c
void test_self_oscillator(void) {
    ESP_LOGI(TAG, "=== Self-Oscillator Test ===");

    // Change to self-oscillator personality
    g_node.personality = PERSONALITY_SELF_OSCILLATOR;
    ESP_LOGI(TAG, "Changed to SELF_OSCILLATOR mode");

    // Initial poke to start oscillation
    poke_event_t poke = {
        .source_node_id = MY_NODE_ID,
        .strength = 0.1f,  // Small kick
        .phase_hint = 0.0f
    };
    float weights[4] = {1.0, 0.0, 0.0, 0.0};
    memcpy(poke.mode_weights, weights, sizeof(weights));
    xQueueSend(g_poke_queue, &poke, 0);

    // Monitor amplitude settling to limit cycle
    for (int i = 0; i < 100; i++) {  // 10 seconds
        float amp = cabsf(g_node.modes[0].a);
        ESP_LOGI(TAG, "t=%d ms, |a0|=%.3f", i * 100, amp);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Expected: amplitude settles to ~0.5-0.8 (limit cycle)");

    // Restore resonator mode
    g_node.personality = PERSONALITY_RESONATOR;
}
```

**Expected**: After initial transient, amplitude settles to stable limit cycle value.

### 3.6 Test 3.6.5: Full Multi-Mode Response

```c
void test_multi_mode(void) {
    ESP_LOGI(TAG, "=== Multi-Mode Response Test ===");

    // Enable all channels
    for (int i = 0; i < 4; i++) {
        audio_synth_set_mode_gain(&g_audio, i, 0.7f);
    }

    // Poke all modes with different weights
    poke_event_t poke = {
        .source_node_id = MY_NODE_ID,
        .strength = 0.8f,
        .phase_hint = -1.0f  // Random phase
    };
    float weights[4] = {1.0, 0.8, 0.3, 0.5};
    memcpy(poke.mode_weights, weights, sizeof(weights));
    xQueueSend(g_poke_queue, &poke, 0);

    ESP_LOGI(TAG, "Listen for rich harmonic content (all 4 modes)");
    vTaskDelay(pdMS_TO_TICKS(5000));
}
```

**Expected**: Rich, complex tone with fundamental (440Hz), beating (442Hz), brightness (880Hz), and sub-bass (55Hz).

### 3.7 Success Criteria

- ✅ All 4 channels output distinct frequencies
- ✅ Channel isolation works (solo mode outputs correct frequency)
- ✅ Beating audible when mixing 440Hz + 442Hz
- ✅ Modal decay matches expected time constants (γ values)
- ✅ Self-oscillator mode sustains tone
- ✅ No clicks, pops, or distortion
- ✅ Amplitude smoothing prevents discontinuities

**Audio Quality Checks**:
- Use spectrum analyzer to verify frequencies (±1 Hz accuracy)
- THD < 1% (with fast_sin approximation)
- No audible aliasing or artifacts

---

## Phase 4: Two Nodes + Audio - Full Distributed System

**Goal**: Verify network-driven audio and distributed dynamics.

**Hardware**:
- 2× ESP32 with audio DACs
- Or 1× ESP32 with audio (listen to network-driven responses)

### 4.1 Configuration

**Node A**: Audio enabled, ID=0
**Node B**: Audio enabled (or disabled), ID=1

### 4.2 Test 4.2.1: Network-Driven Audio

**Node B** sends periodic pokes to Node A:

```c
// Add to Node B's control_task():
static uint32_t poke_timer = 0;
if (++poke_timer % 500 == 0) {  // Every 1 second
    network_message_t msg;
    float weights[4] = {1.0, 0.8, 0.3, 0.5};
    protocol_create_poke(&msg, 1, 0, 0.5f, -1.0f, weights);
    esp_now_send_message(&g_network, 0, &msg, sizeof(msg_poke_t));
    ESP_LOGI(TAG, "Sent poke to Node A");
}
```

**Node A Expected**:
- Audio output responds rhythmically (1 poke/second)
- Modal amplitudes spike on each poke
- Decay between pokes

**Listen for**: 1 Hz rhythm on Node A's audio output.

### 4.3 Test 4.3.2: Mutual Coupling

Both nodes poke each other based on modal state:

```c
// Add to both nodes' control_task():
static uint32_t coupling_timer = 0;
if (++coupling_timer % 250 == 0) {  // Every 500ms
    float my_amplitude = cabsf(g_node.modes[0].a);

    if (my_amplitude > 0.3f) {  // Threshold
        uint8_t target = (MY_NODE_ID == 0) ? 1 : 0;
        network_message_t msg;
        float weights[4] = {0.5, 0.5, 0.2, 0.1};
        float strength = my_amplitude * 0.3f;  // Proportional coupling

        protocol_create_poke(&msg, MY_NODE_ID, target, strength, -1.0f, weights);
        esp_now_send_message(&g_network, target, &msg, sizeof(msg_poke_t));
    }
}
```

**Expected**:
- Self-sustaining oscillations between nodes
- Rich temporal patterns
- Both nodes' audio outputs show correlated dynamics

### 4.4 Test 4.4.3: Configuration Distribution

**Setup**: Build Node A as hub

```bash
idf.py menuconfig
# Component config → Modal Resonator → Hub Mode: Yes
idf.py build flash
```

**Hub distributes ring config**:

```c
// In hub_main.c, after discovery:
hub_send_default_config(&hub);
```

**Node B Expected**:
```
I (10000) MODAL_NODE: CFG_BEGIN: size=3584 chunks=18 crc=0x12345678
I (10100) MODAL_NODE: CFG_CHUNK: idx=0 size=200
...
I (10900) MODAL_NODE: CFG_END: received 18/18 chunks
I (10901) MODAL_NODE: Configuration loaded successfully
I (10902) MODAL_NODE: Configuration applied to modal node
```

**Verify**: Node B's modal frequencies change to match distributed config.

### 4.5 Test 4.5.4: Channel-Specific Network Response

**Node B** pokes different modes on Node A:

```c
// Test 1: Poke mode 0 only
void poke_mode_0(void) {
    network_message_t msg;
    float weights[4] = {1.0, 0.0, 0.0, 0.0};
    protocol_create_poke(&msg, 1, 0, 0.8f, -1.0f, weights);
    esp_now_send_message(&g_network, 0, &msg, sizeof(msg_poke_t));
}

// Test 2: Poke mode 2 only (brightness)
void poke_mode_2(void) {
    network_message_t msg;
    float weights[4] = {0.0, 0.0, 1.0, 0.0};
    protocol_create_poke(&msg, 1, 0, 0.8f, -1.0f, weights);
    esp_now_send_message(&g_network, 0, &msg, sizeof(msg_poke_t));
}
```

**Expected**:
- Test 1: Node A outputs mainly on Channel 0 (440 Hz)
- Test 2: Node A outputs mainly on Channel 2 (880 Hz)

### 4.6 Test 4.6.5: Multi-Node Audio Recording

**Setup**: Connect Node A's 4-channel output to audio interface, record in Audacity.

**Procedure**:
1. Node B sends varied pokes (different modes, strengths, rates)
2. Record 30 seconds of Node A's output
3. Analyze spectrum on each channel

**Expected Spectrum** (using FFT):
- **Channel 0**: Peak at 440 Hz
- **Channel 1**: Peak at 442 Hz
- **Channel 2**: Peak at 880 Hz
- **Channel 3**: Peak at 55 Hz

**Verify**: All channels show expected frequencies with dynamic amplitudes.

### 4.7 Success Criteria

- ✅ Node A audio responds to Node B pokes
- ✅ Network latency < 5ms (audio reacts immediately)
- ✅ Configuration distribution works (chunks received, applied)
- ✅ Different poke weights excite different modes/channels
- ✅ Mutual coupling creates stable or oscillating dynamics
- ✅ Multi-channel recording shows correct frequencies on each channel
- ✅ No audio dropouts during network activity

---

## Phase 5: Hub + MIDI Integration

**Goal**: Hub receives MIDI, coordinates network, distributes pokes.

**Hardware**:
- 1× ESP32 Hub (with MIDI input on GPIO16)
- 2-16× ESP32 Nodes (with audio outputs)
- MIDI controller or USB-MIDI interface

### 5.1 Hub Hardware Setup

**MIDI Input Circuit**:
```
MIDI DIN Jack (5-pin)
Pin 4 (current source) → +5V via 220Ω resistor
Pin 5 (current sink)    → GPIO16 via optocoupler (6N138)
                        → GND via 220Ω resistor
```

**Or use USB-MIDI adapter** connected to ESP32 UART.

### 5.2 Test 5.2.1: MIDI Reception

**Hub logs**:
```
I (5000) HUB: MIDI Note On: note=60 vel=100 ch=0
I (5001) HUB: Target node: 60 % 2 = 0
I (5002) HUB: Sending poke to node 0, strength=0.787
```

**Node A logs** (node 0):
```
I (5003) MODAL_NODE: Received POKE from hub, strength=0.787
```

### 5.3 Test 5.3.2: Note-to-Node Mapping

Play notes on MIDI keyboard:
- **Note 60 (C4)**: → Node 0
- **Note 61 (C#4)**: → Node 1
- **Note 62 (D4)**: → Node 0
- **Note 63 (D#4)**: → Node 1

**Verify**: Notes distribute in round-robin fashion.

### 5.4 Test 5.4.3: Polyphonic Performance

Play **C major chord** (C-E-G, notes 60-64-67):
- Note 60 → Node 0
- Note 64 → Node 0
- Note 67 → Node 1

**Expected**: Both nodes triggered, rich harmonic content.

### 5.5 Test 5.5.4: Velocity Sensitivity

Play same note at different velocities:
- **Low velocity (40)**: strength = 40/127 = 0.31
- **High velocity (127)**: strength = 1.0

**Expected**: Louder notes produce higher amplitudes.

### 5.6 Test 5.6.5: Channel Routing

**MIDI Channel 1** (triggers):
- Quick attack, decay

**MIDI Channel 2** (sustained):
- Longer sustain (if implemented)

**Verify**: Different channels produce different envelope behaviors.

### 5.7 Success Criteria

- ✅ Hub receives MIDI correctly
- ✅ Notes map to correct nodes (modulo distribution)
- ✅ Velocity controls poke strength
- ✅ Polyphonic chords work (multiple nodes triggered)
- ✅ No missed notes at moderate tempos (< 120 BPM)
- ✅ Real-time performance with <10ms latency

---

## Troubleshooting

### No Serial Output
- Check USB cable, serial port (`ls /dev/ttyUSB*`)
- Verify baud rate: 115200
- Try `idf.py monitor` instead of screen/minicom

### Node Won't Flash
- Hold BOOT button, press RESET, release BOOT
- Check `idf.py flash` for errors
- Verify ESP-IDF installed correctly

### Nodes Don't Discover Each Other
- Check WiFi channel (ESP-NOW uses same channel)
- Reduce distance (<10m initially)
- Check MAC addresses in logs
- Manually add peers if auto-discovery not implemented

### No Audio Output
1. Check I2S wiring (BCK=25, WS=26, DIN=27)
2. Verify DAC powered (3.3V or 5V)
3. Check DAC VCC pin connected
4. Verify speakers/headphones connected
5. Add manual poke to trigger audio

### Distorted Audio
- Reduce master gain: `audio_synth_set_gain(&g_audio, 0.5f)`
- Reduce per-mode gains
- Check clipping (amplitudes > 1.0)
- Verify DAC supports 16-bit input

### Clicks/Pops on Poke Events
- Verify amplitude smoothing: `SMOOTH_ALPHA = 0.12`
- Check for discontinuities in phase accumulator
- Ensure modal integration runs smoothly

### Network Packet Loss
- Reduce poke rate
- Check RSSI (should be > -70 dBm)
- Move nodes closer
- Check ESP-NOW queue not overflowing

### Config Distribution Fails
- Check checksum logs (CRC mismatch?)
- Verify chunk count matches
- Increase inter-chunk delay in hub
- Monitor serial for CFG_NACK messages

---

## Performance Benchmarks

### Expected Metrics

| Metric | Target | Measured |
|--------|--------|----------|
| CPU usage (total) | <20% | ____% |
| Audio task CPU | <15% | ____% |
| Control task CPU | <3% | ____% |
| Network latency | <5ms | ____ms |
| Audio latency | <15ms | ____ms |
| Packet loss | 0% | ____% |
| Audio THD | <1% | ____% |

### How to Measure

**CPU usage**:
```c
TaskStatus_t task_status;
vTaskGetInfo(audio_task_handle, &task_status, pdTRUE, eRunning);
uint32_t cpu_percent = task_status.ulRunTimeCounter / total_runtime * 100;
```

**Network latency**: Add timestamp to poke messages (see Phase 2)

**Audio latency**: Modal integration (2ms) + buffer (10ms) + I2S (1ms) = 13ms

**Packet loss**: Monitor TX vs RX counts

**THD**: Use audio analyzer or Audacity spectrum analysis

---

## Next Steps After Successful Testing

1. **Scale to 4+ nodes**: Test larger networks
2. **Topology experiments**: Ring, small-world, clusters
3. **Performance tuning**: Optimize for lower latency
4. **Audio quality**: Higher sample rates, better DACs
5. **Visualization**: Add LEDs or OLED display
6. **Mobile app**: Bluetooth configuration interface

---

## Support & Resources

- **Documentation**: `esp32/docs/`
- **Issue Tracker**: Report bugs in project repository
- **ESP-IDF Docs**: https://docs.espressif.com/
- **Forum**: ESP32 community forums

**Last Updated**: 2026-01-06
**Version**: 1.0 (Initial rollout testing guide)
