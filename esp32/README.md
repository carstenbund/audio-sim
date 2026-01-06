# ESP32 Distributed Modal Resonator Network

**Embedded implementation of the modal resonator network for ESP32 hardware**

This folder contains the complete codebase for running the distributed modal resonator network on ESP32 microcontrollers using ESP-NOW mesh networking.

---

## Design Philosophy

This implementation follows an **audio-first, asynchronous** design:
- **4 local oscillators per node** (K ≤ 4 complex modes)
- **No hard 1kHz sync requirement** - nodes operate autonomously
- **Event-based "poke" excitation** - network stimulates, doesn't dictate
- **Message-only control plane** - no web server needed
- **Up to 16 nodes** via ESP-NOW

See `docs/DESIGN_SPEC.md` for complete design rationale.

---

## Folder Structure

```
esp32/
├── docs/                      # Design documents and specifications
│   ├── DESIGN_SPEC.md        # Core design philosophy and architecture
│   └── API.md                # Message protocol and API reference
│
├── firmware/                  # Main firmware source code
│   ├── main/                 # Application entry point
│   │   ├── core/            # Modal resonator core (4-mode dynamics)
│   │   ├── audio/           # Audio synthesis (48kHz)
│   │   ├── network/         # ESP-NOW communication
│   │   ├── config/          # Session and configuration management
│   │   └── utils/           # Utilities and helpers
│   │
│   └── components/           # Reusable ESP-IDF components
│       ├── modal_core/      # Modal oscillator library
│       ├── esp_now_mesh/    # ESP-NOW mesh abstraction
│       └── audio_output/    # I2S audio output driver
│
├── hardware/                  # Hardware designs and BOM
│   ├── schematics/          # Circuit schematics
│   └── bom/                 # Bill of materials
│
├── tools/                     # Development and testing tools
│   └── configurator/        # Configuration blob generator
│
└── tests/                     # Unit and integration tests
```

---

## Key Features

### 1. 4-Mode Resonator Core
Each node maintains 4 complex modes:
- **Mode 0**: Audible carrier
- **Mode 1**: Detuning/beating
- **Mode 2**: Timbre modulation
- **Mode 3**: Slow structural state

### 2. Autonomous Operation
- Nodes generate audio independently
- Network input excites/perturbs, doesn't sustain
- Stable operation even with network failures

### 3. Two Node Personalities
- **Resonator**: Decays to silence (percussive)
- **Self-Oscillator**: Continuous sound (drone)

### 4. Event-Based Network
- **Poke messages**: Energy injection events
- **Configuration messages**: Session setup
- **No continuous state sync**: Event-driven only

### 5. Fixed Topologies
Presets for 16-node networks:
- Ring
- Small-world (ring + sparse links)
- Clusters with bridge
- Hub-and-spokes

---

## Hardware Requirements

### Per Node
- **MCU**: ESP32-C3 or ESP32-S3
- **DAC**: PCM5102A (I2S, recommended)
- **Power**: 5V USB or barrel jack
- **Optional**: MIDI input (controller node only)

### Cost
- ~$8 per node (bare components)
- ~$16 per node (with enclosure)

See `hardware/bom/` for detailed parts list.

---

## Getting Started

### Prerequisites
- ESP-IDF v5.0+ (https://docs.espressif.com/projects/esp-idf/)
- Python 3.8+ with esp-idf tools

### Build and Flash (Single Node)

```bash
cd firmware/main
idf.py set-target esp32c3
idf.py menuconfig  # Configure node ID, audio settings
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Network Setup (Multi-Node)

1. Flash each ESP32 with unique `node_id`
2. Power on all nodes (they auto-discover via ESP-NOW)
3. Send configuration blob from controller
4. Send `START` message to begin session

---

## Development Roadmap

### Phase 1: Single Node (Complete) ✓
- [x] 4-mode modal resonator core
- [x] Audio synthesis at 48kHz
- [x] I2S output driver
- [x] FreeRTOS task structure

### Phase 2: Two-Node Coupling (In Progress)
- [ ] ESP-NOW message protocol
- [ ] Poke event handling
- [ ] Latency measurement
- [ ] Coupled oscillation validation

### Phase 3: 16-Node Network (Planned)
- [ ] Auto-discovery and registration
- [ ] Configuration blob distribution
- [ ] Session management
- [ ] Topology presets

### Phase 4: Production Features (Future)
- [ ] LED status indicators
- [ ] Battery power support
- [ ] OTA firmware updates
- [ ] Mobile app configurator

---

## Configuration Example

```json
{
  "session_id": "test_ring_16",
  "topology": "ring",
  "num_nodes": 16,
  "nodes": [
    {
      "node_id": 0,
      "personality": "resonator",
      "modes": [
        {"omega": 440.0, "gamma": 0.5, "weight": 1.0},
        {"omega": 442.0, "gamma": 0.6, "weight": 0.8},
        {"omega": 880.0, "gamma": 1.0, "weight": 0.3},
        {"omega": 55.0,  "gamma": 0.1, "weight": 0.5}
      ],
      "neighbors": [1, 15],
      "coupling_strength": 0.3
    }
  ]
}
```

See `docs/API.md` for full configuration schema.

---

## Message Protocol

### Discovery
- `HELLO` → Broadcast presence
- `OFFER` → Controller offers to configure
- `JOIN` → Node accepts configuration

### Configuration
- `CFG_BEGIN` → Start config transfer
- `CFG_CHUNK` → Config data packet
- `CFG_END` → Finish config transfer
- `CFG_ACK`/`CFG_NACK` → Confirmation

### Runtime
- `POKE` → Excitation event
- `START`/`STOP` → Session control

See `firmware/main/network/protocol.h` for message structures.

---

## Audio Architecture

### Rate Separation
- **48 kHz**: Audio synthesis (I2S DMA)
- **200-1000 Hz**: Modal integration (FreeRTOS task)
- **Event-based**: Network messages (ESP-NOW)

### Audio Mapping (Mode 0 Example)
```c
float carrier_freq = 440.0f;  // Base frequency
float amp = cabsf(modes[0]);   // Mode 0 amplitude
float phase_mod = cargf(modes[2]) * 0.1f;  // Mode 2 modulation

for (int i = 0; i < buffer_size; i++) {
    float t = phase / SAMPLE_RATE;
    float sample = amp * sinf(2 * M_PI * carrier_freq * t + phase_mod);
    audio_buffer[i] = (int16_t)(sample * 32767.0f);
    phase += 1.0f;
}
```

---

## Testing

### Unit Tests
```bash
cd tests
pytest test_modal_core.py
```

### Hardware-in-Loop
```bash
cd firmware/main
idf.py monitor  # Watch node output
# Send test poke from another node
```

### Network Latency Test
```bash
python tools/network_bench.py --nodes 16 --topology ring
```

---

## Troubleshooting

### Audio Quality Issues
- Check I2S wiring (BCK, WS, DIN)
- Verify PCM5102A power supply (3.3V clean)
- Reduce CPU load (lower control rate)

### Network Latency
- Check WiFi channel congestion
- Reduce packet size
- Use wired I2C backup for critical links

### CPU Overload
- Profile with `xtensa-esp32-elf-gprof`
- Optimize modal integration (use FPU)
- Pin tasks to specific cores

---

## Contributing

This is an experimental research project. Contributions welcome:
- Firmware optimization
- New topology presets
- Hardware designs (PCB, enclosures)
- Documentation improvements

---

## References

- **Original Proposal**: `../docs/ESP32_HARDWARE_PROPOSAL.md`
- **Design Spec**: `docs/DESIGN_SPEC.md`
- **ESP-NOW Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
- **ESP-IDF**: https://docs.espressif.com/projects/esp-idf/

---

**Status**: Implementation in progress
**Target**: 16-node prototype by Q1 2026
**Hardware Cost**: ~$128 (16 nodes)
