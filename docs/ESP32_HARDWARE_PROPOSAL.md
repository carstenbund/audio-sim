# ESP32 Hardware Implementation Proposal
## Distributed Modal Network Synthesizer

**Date:** 2026-01-06
**Version:** 1.0
**Author:** Modal Network Team

---

## Executive Summary

This proposal outlines three architectures for implementing the modal network synthesizer on ESP32 hardware, enabling standalone, embeddable, and scalable physical instruments. The recommended approach uses **one ESP32 per node** with ESP-NOW mesh networking, providing true distributed computation with modular scalability.

### Key Benefits
- **Modular**: Add/remove nodes physically
- **Low-cost**: ~$5 per node (ESP32-C3)
- **Low-latency**: <5ms node-to-node coupling
- **Standalone**: No computer required
- **Scalable**: 8-64+ nodes via mesh

---

## Architecture Options

### Option A: Monolithic (One ESP32 = Full 8-Node Network)

```
┌─────────────────────────────────────┐
│         ESP32 Main Board            │
│  ┌───────────────────────────────┐  │
│  │  All 8 nodes simulated        │  │
│  │  1 kHz simulation loop        │  │
│  │  48 kHz audio generation      │  │
│  └───────────────────────────────┘  │
│                                     │
│  MIDI In ──→ UART                   │
│  I2S Out ──→ 8ch DAC (PCM3168A)    │
└─────────────────────────────────────┘
```

**Pros:**
- Simplest architecture
- Zero network latency
- Lowest part count

**Cons:**
- Fixed 8-node limit
- Not modular (can't add/remove nodes)
- Requires expensive 8-channel DAC (~$15)
- Single point of failure

**Use Case:** Prototype, fixed 8-node instrument

---

### Option B: Fully Distributed (One ESP32 = One Node) ⭐ **RECOMMENDED**

```
┌──────────┐     ┌──────────┐     ┌──────────┐
│  Node 0  │ ←─→ │  Node 1  │ ←─→ │  Node 2  │
│  ESP32   │     │  ESP32   │     │  ESP32   │
│  ┌────┐  │     │  ┌────┐  │     │  ┌────┐  │
│  │ a₀ │  │     │  │ a₁ │  │     │  │ a₂ │  │
│  └────┘  │     │  └────┘  │     │  └────┘  │
│  DAC Out │     │  DAC Out │     │  DAC Out │
└──────────┘     └──────────┘     └──────────┘
       ↑              ↑              ↑
       └──────────────┴──────────────┘
            ESP-NOW Mesh Network
              (coupling terms)
```

**Pros:**
- **Truly modular**: Add nodes by plugging in hardware
- **Scalable**: 8 → 16 → 32+ nodes easily
- **Distributed**: Computation spread across nodes
- **Cheap**: $5/node (ESP32-C3 + DAC)
- **Fault-tolerant**: One node failure doesn't crash system
- **Physical cables**: Can also connect via I2C/CAN for deterministic coupling

**Cons:**
- Network latency (~2-5ms via ESP-NOW)
- More complex firmware (distributed sync)
- Requires mesh network management

**Use Case:** Modular performance instrument, installations, research

---

### Option C: Hybrid (One ESP32 = 4 Nodes)

```
┌─────────────────────┐     ┌─────────────────────┐
│   ESP32 Board A     │     │   ESP32 Board B     │
│  ┌─────────────┐    │ ←─→ │    ┌─────────────┐  │
│  │ Nodes 0-3   │    │     │    │ Nodes 4-7   │  │
│  └─────────────┘    │     │    └─────────────┘  │
│  4ch I2S DAC        │     │    4ch I2S DAC      │
└─────────────────────┘     └─────────────────────┘
```

**Pros:**
- Balance of modularity and simplicity
- Lower network overhead
- Can use stereo DACs (2× CS4334)

**Cons:**
- Less flexible than Option B
- More expensive than Option A
- Still requires inter-board communication

**Use Case:** Fixed multi-board instruments

---

## Recommended Architecture: Fully Distributed (Option B)

### Why This Approach?

1. **True Modularity**: Physical nodes = network nodes
2. **Scalability**: Start with 8, expand to 64+
3. **Cost-Effective**: ~$40 for 8-node system vs. $60+ for Option A
4. **Educational**: Each node is self-contained, easier to understand
5. **Fault-Tolerant**: System degrades gracefully if nodes fail
6. **Research-Friendly**: Easy to reconfigure topology

---

## Hardware Specification (Per Node)

### Core Module: ESP32-C3-DevKitM-1 (~$5)
- **MCU**: ESP32-C3 (RISC-V, 160 MHz)
- **RAM**: 400 KB SRAM
- **Flash**: 4 MB
- **WiFi**: 2.4 GHz 802.11b/g/n (for ESP-NOW)
- **GPIO**: 22 pins
- **ADC**: 12-bit, 6 channels
- **UART**: 2 (one for MIDI, one for debug)

### Audio Output: PWM DAC + Filter (~$2)

**Option 1: Simple PWM (Cheapest)**
```
ESP32 GPIO ──→ PWM (80 kHz) ──→ RC Filter ──→ Audio Out
                                 (10kΩ + 100nF)
```
- Pros: $0.50/node, no external ICs
- Cons: 8-bit effective resolution, more noise
- SNR: ~50 dB (acceptable for experimental)

**Option 2: External DAC - MCP4725 (~$2)**
```
ESP32 I2C ──→ MCP4725 (12-bit DAC) ──→ Audio Out
```
- Pros: 12-bit resolution, cleaner output
- Cons: I2C bandwidth limits sample rate to ~10 kHz
- SNR: ~70 dB

**Option 3: PCM5102A I2S DAC (~$3)** ⭐ **RECOMMENDED**
```
ESP32 I2S ──→ PCM5102A ──→ Audio Out (3.5mm jack)
              (32-bit DAC)
```
- Pros: Professional audio quality, 48 kHz capable
- Cons: Slightly more expensive, one extra IC per node
- SNR: ~100 dB (studio quality)

### MIDI Input (Central Controller Only)

**Hardware:**
- MIDI DIN-5 connector
- 6N138 optocoupler for isolation
- UART2 @ 31250 baud

**Circuit:**
```
MIDI In (Pin 5) ──→ 220Ω ──→ 6N138 ──→ ESP32 RX
MIDI In (Pin 4) ──────────→ GND
```

### Power Supply

**Per Node:**
- 5V USB or barrel jack input
- AMS1117-3.3 LDO regulator
- ~200mA peak per node
- 8 nodes = 1.6A @ 5V total

**Options:**
1. USB hub (cheap, flexible)
2. Single 5V/3A supply with distribution board
3. Individual USB wall adapters

### Interconnect Options

**Option 1: ESP-NOW (Wireless) ⭐ PRIMARY**
- Built-in WiFi, no extra hardware
- ~2-5ms latency
- 250 bytes/packet sufficient for coupling data
- Auto-discovery via broadcast

**Option 2: I2C Bus (Wired Backup)**
- Twisted pair cable between nodes
- Daisy-chain or star topology
- Deterministic timing
- Up to 1 MHz (1 µs per byte)
- For ultra-low latency setups

**Option 3: CAN Bus (Industrial)**
- For noisy environments
- Requires MCP2515 ($2) + TJA1050 transceiver ($1)
- Deterministic, reliable
- Overkill for most uses

---

## Software Architecture

### Firmware Structure (Per Node)

```c
// Main FreeRTOS tasks
void simulation_task(void *pvParameters) {
    // Core: 1 kHz network simulation
    // 1ms timestep loop
    // Complex arithmetic for 1 node × 2 modes
}

void audio_task(void *pvParameters) {
    // Core: 48 kHz audio generation
    // I2S DMA output
    // Phase accumulator for sine generation
}

void network_task(void *pvParameters) {
    // Core: ESP-NOW message handling
    // Receive coupling terms from neighbors
    // Send own state to neighbors
}

void midi_task(void *pvParameters) {
    // Controller node only
    // Parse MIDI messages
    // Broadcast to nodes via ESP-NOW
}
```

### Memory Layout (Per Node)

```c
typedef struct {
    complex_t a[2];           // 2 modes × 8 bytes = 16 bytes
    complex_t drive;          // 8 bytes
    float omega[2];           // 8 bytes (mode frequencies)
    float gamma[2];           // 8 bytes (damping)
    float coupling;           // 4 bytes
    uint8_t node_id;          // 1 byte
    uint8_t num_neighbors;    // 1 byte
    uint8_t neighbor_ids[8];  // 8 bytes
} node_state_t;

// Total: ~60 bytes per node state
```

### Communication Protocol (ESP-NOW)

**Message Format:**
```c
typedef struct {
    uint8_t msg_type;         // 1 = state, 2 = MIDI, 3 = sync
    uint8_t node_id;          // Sender ID
    uint16_t timestamp_ms;    // For sync
    union {
        struct {
            float a0_real;    // Re(a[0])
            float a0_imag;    // Im(a[0])
            float amplitude;  // |a[0]|
        } state;
        struct {
            uint8_t note;
            uint8_t velocity;
            uint8_t channel;
        } midi;
    } payload;
} esp_now_message_t;

// Total: 16 bytes per message
```

**Update Cycle:**
1. Simulation step (1ms)
2. Broadcast state to neighbors (~500 µs)
3. Receive neighbor states (~500 µs)
4. Compute coupling term
5. Repeat

**Latency Budget:**
- Simulation: 0.5 ms
- ESP-NOW TX: 0.2 ms
- Network propagation: 1-3 ms
- ESP-NOW RX: 0.2 ms
- Total: ~2-5 ms coupling delay

---

## Audio Output Strategy

### Per-Node Audio Generation

Each ESP32 generates its own channel at 48 kHz:

```c
// I2S DMA buffer callback (every 10 ms @ 48 kHz)
void i2s_write_task() {
    float freq = midi_to_hz(current_note);
    float vel = current_velocity;
    float amp = cabs(state.a[0]);  // Network amplitude

    for (int i = 0; i < BUFFER_SIZE; i++) {
        float t = phase / SAMPLE_RATE;
        float sample = amp * vel * sin(2 * PI * freq * t);

        // Convert to 16-bit PCM
        i2s_buffer[i] = (int16_t)(sample * 32767.0f);
        phase += 1.0f;
    }

    i2s_write(I2S_NUM_0, i2s_buffer, ...);
}
```

### Multi-Node Audio Routing

**Option 1: Individual Outputs (Modular)**
- Each node has 3.5mm jack output
- Mix in DAW or external mixer
- Best for installations/multi-speaker setups

**Option 2: Central Audio Hub**
- One ESP32 aggregates audio via ESP-NOW
- Outputs mixed signal via I2S
- Good for headphone monitoring

**Option 3: Bluetooth A2DP**
- Each node streams via BT (latency ~50ms)
- For wireless monitoring only

---

## MIDI Input Handling

### Central Controller Architecture

One "controller node" (can be node 0) handles MIDI:

```c
void midi_controller_task() {
    midi_message_t msg;

    while (1) {
        if (uart_read_bytes(UART_NUM_2, &msg, 3, 10)) {
            // Parse MIDI message
            uint8_t cmd = msg.status & 0xF0;
            uint8_t channel = msg.status & 0x0F;

            if (cmd == 0x90) {  // Note On
                uint8_t note = msg.data1;
                uint8_t velocity = msg.data2;
                uint8_t target_node = note % 8;

                // Broadcast to target node via ESP-NOW
                esp_now_message_t nm = {
                    .msg_type = 2,
                    .node_id = 0,
                    .payload.midi = {note, velocity, channel}
                };

                esp_now_send(node_mac[target_node], &nm, sizeof(nm));
            }
        }
    }
}
```

### MIDI-to-Node Routing

- **Channel 1**: Trigger notes → `node = note % 8`
- **Channel 2**: Drive notes → broadcast to all nodes with velocity
- **Control Change**: System parameters (coupling, damping)

---

## Scalability Considerations

### Network Topology

**Ring (8 nodes):**
```
Node 0 ←→ Node 1 ←→ ... ←→ Node 7 ←→ Node 0
```
- Matches current simulation
- Each node has 2 neighbors
- 16 total connections

**Complete Graph (All-to-All):**
```
Every node connected to every other node
```
- 8 nodes = 28 connections
- ESP-NOW supports up to 20 peers
- Need subgraph approach for >8 nodes

**Hierarchical (Clusters):**
```
Cluster 0 (Nodes 0-7) ←→ Cluster 1 (Nodes 8-15)
```
- Scale to 64+ nodes
- One "gateway" node per cluster
- 2-tier coupling: strong intra-cluster, weak inter-cluster

### Performance Scaling

| Nodes | ESP32s | Topology      | Latency  | Cost   |
|-------|--------|---------------|----------|--------|
| 8     | 8      | Ring          | 2-5 ms   | $64    |
| 16    | 16     | 2× Ring       | 3-7 ms   | $128   |
| 32    | 32     | Hierarchical  | 5-15 ms  | $256   |
| 64    | 64     | Hierarchical  | 10-30 ms | $512   |

---

## Development Roadmap

### Phase 1: Single-Node Prototype (2 weeks)
**Goal:** Validate ESP32 can run simulation at 1 kHz

- [ ] Port core simulation to C (ESP-IDF)
- [ ] Implement complex arithmetic (no stdlib)
- [ ] Test 1 kHz loop timing with FreeRTOS
- [ ] Validate float performance (hardware FPU)
- [ ] Implement I2S audio output
- [ ] Benchmark: CPU usage, memory, latency

**Deliverables:**
- Single ESP32 running 1-node simulation
- Audio output via PCM5102A
- Serial debug output

---

### Phase 2: Two-Node Coupling (2 weeks)
**Goal:** Validate ESP-NOW communication and coupling

- [ ] Implement ESP-NOW discovery and pairing
- [ ] Define coupling message protocol
- [ ] Test roundtrip latency (ping-pong)
- [ ] Implement neighbor state exchange
- [ ] Compute coupling term from neighbor data
- [ ] Validate coupled oscillations

**Deliverables:**
- 2 ESP32s coupled via ESP-NOW
- Measurable synchronization behavior
- Latency profiling

---

### Phase 3: 8-Node Ring (3 weeks)
**Goal:** Full modal network on hardware

- [ ] Scale to 8 nodes
- [ ] Implement ring topology routing
- [ ] Add MIDI input on controller node
- [ ] Test MIDI-to-node mapping
- [ ] Implement order parameter computation
- [ ] Optimize network bandwidth

**Deliverables:**
- 8-node hardware modal synthesizer
- MIDI control via single controller
- 8 individual audio outputs

---

### Phase 4: Advanced Features (4 weeks)
**Goal:** Production-ready instrument

- [ ] Auto-discovery and plug-and-play nodes
- [ ] LED status indicators per node
- [ ] Web interface for monitoring (WiFi)
- [ ] SD card logging for experiments
- [ ] Battery power option (LiPo)
- [ ] 3D-printed enclosures
- [ ] BLE MIDI for wireless input
- [ ] Firmware OTA updates

**Deliverables:**
- User-friendly modular instrument
- Documentation and assembly guide
- Example patches and performances

---

## Bill of Materials (8-Node System)

| Component                  | Qty | Unit Price | Total  | Notes                          |
|----------------------------|-----|------------|--------|--------------------------------|
| ESP32-C3-DevKitM-1         | 8   | $5.00      | $40.00 | Core MCU boards                |
| PCM5102A DAC breakout      | 8   | $3.00      | $24.00 | Audio output                   |
| MIDI DIN-5 connector       | 1   | $1.00      | $1.00  | Controller only                |
| 6N138 optocoupler          | 1   | $0.50      | $0.50  | MIDI isolation                 |
| Resistors/capacitors       | -   | -          | $5.00  | Passives (bulk)                |
| USB cables (power)         | 8   | $2.00      | $16.00 | Power delivery                 |
| 3.5mm audio jacks          | 8   | $0.50      | $4.00  | Audio outputs                  |
| Perfboard/PCB              | 8   | $2.00      | $16.00 | Mounting                       |
| Enclosure (3D printed)     | 8   | $3.00      | $24.00 | Optional                       |
| **TOTAL**                  |     |            | **$130.50** | **Complete 8-node system** |

**Cost Breakdown:**
- Core electronics: $90.50
- Enclosures: $24.00 (optional)
- Cables/connectors: $16.00

**Per-Node Cost:** $16.31 (with enclosure) or $11.31 (bare bones)

---

## Challenges and Mitigations

### Challenge 1: Network Latency
**Problem:** ESP-NOW latency (2-5ms) may affect coupling dynamics

**Mitigations:**
1. Use predictive coupling (extrapolate neighbor state)
2. Implement sync protocol (shared global clock)
3. Add I2C wired backup for low-latency setups
4. Tune coupling strength to tolerate latency

**Fallback:** Use Option C (hybrid) for latency-critical apps

---

### Challenge 2: Clock Synchronization
**Problem:** Each ESP32 has independent clock, drift over time

**Mitigations:**
1. One node = master clock, broadcasts sync packets
2. NTP-style synchronization every 100ms
3. Software PLL to align timesteps
4. Use hardware SYNC pin for critical setups

**Validation:** Test drift over 10-minute runs, measure coupling quality

---

### Challenge 3: Packet Loss
**Problem:** ESP-NOW not guaranteed delivery, may lose coupling data

**Mitigations:**
1. Redundant broadcasts (send state 2x)
2. Interpolate missing neighbor data
3. Detect loss and flag in diagnostics
4. Use ESP-NOW ACK mode for critical messages

**Validation:** Stress test with RF interference, measure packet loss rate

---

### Challenge 4: Audio Quality
**Problem:** PWM DAC may have too much noise

**Mitigations:**
1. Use PCM5102A for production (recommended)
2. Add post-DAC LC filter for PWM output
3. Shield audio circuitry from ESP32 RF
4. Use differential output if needed

**Validation:** Measure SNR and THD with audio analyzer

---

### Challenge 5: Real-Time Performance
**Problem:** FreeRTOS task scheduling may miss 1ms deadline

**Mitigations:**
1. Pin simulation task to Core 0 (dedicated)
2. Use high-priority FreeRTOS task (priority 10+)
3. Disable WiFi during simulation (use ESP-NOW only)
4. Profile with GPIO toggle + scope
5. Optimize floating-point code (use ESP32 FPU)

**Validation:** Measure worst-case execution time (WCET) with jitter

---

## Alternative: Hybrid Approach for Production

For a **commercial product**, consider:

**Central Brain + Distributed Audio:**
- 1× ESP32-S3 (dual-core, 240 MHz): Runs full simulation
- 8× ESP32-C3 (simple): Audio rendering only
- Communication: I2S or SPI for audio samples
- Cost: $60 total (cheaper than 8× full nodes)

**Advantages:**
- Lowest latency (no network in simulation loop)
- Simpler firmware (centralized logic)
- Easier to debug and validate

**Disadvantages:**
- Not modular (can't add nodes dynamically)
- Single point of failure

**Use Case:** Fixed-format commercial instruments where cost is critical

---

## Conclusion and Recommendation

### Recommended Path: **Distributed (Option B)**

**Start Here:**
1. Phase 1: Single-node prototype on ESP32-C3
2. Phase 2: Two-node coupling validation
3. Phase 3: 8-node ring system
4. Phase 4: Polish and productize

**Timeline:** 12 weeks to production-ready prototype
**Budget:** $150 for full 8-node dev setup
**Risk:** Medium (network latency is main unknown)

### Why This Wins:
- **Modularity**: Physical embodiment of network structure
- **Scalability**: 8 → 64+ nodes possible
- **Cost**: $16/node for studio-quality audio
- **Educational**: Each node is tangible, hackable
- **Unique**: No commercial product does this

### Next Steps:
1. Order 2× ESP32-C3 + 2× PCM5102A for Phase 1/2 (~$30)
2. Port core simulation to C (ESP-IDF framework)
3. Build prototype, validate performance
4. Report back on feasibility

---

## Appendix A: Code Skeleton (ESP-IDF)

```c
// node_firmware.c - ESP32 modal network node

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_now.h"
#include "driver/i2s.h"
#include <complex.h>

#define SAMPLE_RATE 48000
#define SIM_RATE 1000  // 1 kHz simulation

typedef struct {
    float complex a[2];  // 2 modes
    float omega[2];
    float gamma[2];
    float coupling;
    uint8_t node_id;
} node_t;

node_t node_state;
float complex neighbor_a[8];  // Neighbor states

// Simulation task: 1 kHz loop
void simulation_task(void *arg) {
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        // Compute coupling term from neighbors
        float complex coupling_sum = 0;
        for (int i = 0; i < node_state.num_neighbors; i++) {
            coupling_sum += neighbor_a[i];
        }
        coupling_sum *= node_state.coupling / node_state.num_neighbors;

        // Simulate one timestep (modal oscillator ODE)
        for (int k = 0; k < 2; k++) {
            float complex da = (
                I * node_state.omega[k] * node_state.a[k]
                - node_state.gamma[k] * node_state.a[k]
                + coupling_sum
            ) * 0.001;  // dt = 1ms

            node_state.a[k] += da;
        }

        // Broadcast state to neighbors
        esp_now_broadcast_state();

        // Sleep until next 1ms tick
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));
    }
}

// Audio task: 48 kHz I2S output
void audio_task(void *arg) {
    int16_t samples[480];  // 10ms buffer
    float phase = 0;

    while (1) {
        float freq = current_freq;
        float vel = current_vel;
        float amp = cabsf(node_state.a[0]);

        for (int i = 0; i < 480; i++) {
            float t = phase / SAMPLE_RATE;
            float sample = amp * vel * sinf(2 * M_PI * freq * t);
            samples[i] = (int16_t)(sample * 32767.0f);
            phase += 1.0f;
        }

        i2s_write(I2S_NUM_0, samples, sizeof(samples), portMAX_DELAY);
    }
}

void app_main() {
    // Initialize ESP-NOW
    esp_now_init();
    esp_now_register_recv_cb(on_data_recv);

    // Initialize I2S audio
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        // ... (full config)
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    // Start tasks
    xTaskCreatePinnedToCore(simulation_task, "sim", 4096, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(audio_task, "audio", 4096, NULL, 5, NULL, 1);
}
```

---

## Appendix B: References

1. **ESP32-C3 Datasheet**: [espressif.com](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
2. **ESP-NOW Protocol**: [ESP-NOW User Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
3. **PCM5102A Datasheet**: [TI](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)
4. **FreeRTOS Real-Time**: [freertos.org](https://www.freertos.org/Documentation/RTOS_book.html)
5. **I2S Audio on ESP32**: [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)

---

**End of Proposal**

For questions or collaboration:
- GitHub: [audio-sim](https://github.com/carstenbund/audio-sim)
- Branch: `claude/add-modal-synth-player-BCxOw`
