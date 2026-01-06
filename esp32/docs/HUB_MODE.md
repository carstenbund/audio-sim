## Hub/Controller Mode

**ESP32 Hub Node for Network Coordination**

The hub node provides centralized coordination for distributed modal resonator networks, including MIDI input, node discovery, and configuration management.

---

## Overview

The hub acts as the **network coordinator** and **MIDI interface** for up to 16 ESP32 nodes. It:

- Receives MIDI input and translates to poke events
- Discovers and registers network nodes
- Distributes configuration
- Coordinates session start/stop
- Uses defaults if no configuration provided

---

## Hub vs Regular Node

| Feature | Hub Mode | Node Mode |
|---------|----------|-----------|
| **Purpose** | Coordinator | Resonator |
| **MIDI Input** | ✓ Yes | ✗ No |
| **Audio Output** | ✗ No | ✓ Yes |
| **Discovery** | Initiates | Responds |
| **Configuration** | Distributes | Receives |
| **Poke Events** | Sends | Receives |

---

## Building Hub Firmware

### Method 1: Using menuconfig

```bash
cd esp32/firmware
idf.py menuconfig
```

Navigate to:
```
Component config → Modal Network Configuration → Hub/Controller Mode
```

Enable and save.

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash
```

### Method 2: Command Line

```bash
cd esp32/firmware
idf.py -DCONFIG_MODAL_HUB_MODE=1 build
```

---

## Hardware Requirements

### Hub Node Hardware

**Required**:
- ESP32-C3 or ESP32-S3 dev board
- 5V USB power

**MIDI Input** (optional but recommended):
- MIDI DIN-5 female connector
- 6N138 optocoupler
- 220Ω and 470Ω resistors
- Connections to GPIO 16 (RX)

**No audio hardware needed** - hub does not generate audio

### MIDI Circuit

```
MIDI In Pin 5 ──┬── 220Ω ──┬── 6N138 (pin 2)
                │          │
                └── 1N4148 ─┘

MIDI In Pin 4 ─────────────── GND

6N138 Pin 6 ─── 470Ω ─── 3.3V
6N138 Pin 6 ─── ESP32 GPIO 16 (RX)
6N138 Pin 3 ─── GND
```

---

## Operation

### Startup Sequence

1. **Initialization** (0-2s)
   - ESP-NOW network initialized
   - Hub controller initialized
   - MIDI UART initialized (if hardware connected)

2. **Discovery** (2-7s)
   - Broadcasts HELLO messages
   - Waits for node responses (5s timeout)
   - Registers responding nodes

3. **Configuration** (7-8s)
   - Sends default configuration to all nodes
   - Waits for acknowledgments

4. **Session Start** (8-9s)
   - Sends START message to all nodes
   - Enters running state

5. **Running** (9s+)
   - Processes MIDI input
   - Sends poke events to nodes
   - Monitors heartbeats

### LED/Serial Output

```
I (2000) HUB_MAIN: Starting node discovery...
I (3124) HUB: Received HELLO from node 1 (Node_001)
I (3156) HUB: Registered node 1 (total: 1)
I (3287) HUB: Received HELLO from node 2 (Node_002)
I (3299) HUB: Registered node 2 (total: 2)
...
I (7000) HUB: Discovery complete: 8 nodes found
I (7100) HUB: Sending default configuration to 8 nodes
I (8000) HUB: Starting session on 8 nodes
I (8100) HUB: Session started - ready for MIDI input
```

---

## MIDI Control

### Channel Assignment

Based on Python implementation (`src/midi_input.py`):

- **Channel 1**: **Trigger notes** (short poke events)
  - Note On → sends 10ms poke to target node
  - Note Off → ignored (poke already sent)
  - Use for percussive/staccato playing

- **Channel 2**: **Drive notes** (sustained pokes)
  - Note On → sends poke every 100ms while held
  - Note Off → stops sending pokes
  - Use for sustained/drone sounds

### Note-to-Node Mapping

Notes are mapped to nodes using modulo:

```
node_id = note % num_nodes
```

**Examples** (8 nodes):
- C4 (60) → node 4
- C#4 (61) → node 5
- D4 (62) → node 6
- ...
- C5 (72) → node 0 (wraps around)

**Examples** (16 nodes):
- C4 (60) → node 12
- C#4 (61) → node 13
- D4 (62) → node 14
- D#4 (63) → node 15
- E4 (64) → node 0 (wraps around)

### Poke Parameters

**Default mode weights**:
```c
mode_weights[4] = {1.0, 0.8, 0.3, 0.5};
```

- Mode 0: Full excitation (carrier)
- Mode 1: 80% (detuning)
- Mode 2: 30% (brightness)
- Mode 3: 50% (sub-bass)

**Strength**: MIDI velocity / 127
**Phase hint**: Random (-1)
**Envelope**: 10ms Hann window

---

## Default Configuration

If no configuration is provided (or `CONFIG_MODAL_USE_DEFAULT_CONFIG=y`), the hub uses built-in presets.

### Ring Topology (16 nodes)

**File**: `presets.c` → `preset_ring_16_resonator()`

```
Topology: Ring
Nodes: 16
Coupling: 0.3
Personality: Resonator

Connections:
Node 0: [15, 1]
Node 1: [0, 2]
Node 2: [1, 3]
...
Node 15: [14, 0]
```

**Modes**:
- Mode 0: 440 Hz (A4)
- Mode 1: 442 Hz (2 Hz beating)
- Mode 2: 880 Hz (octave up)
- Mode 3: 55 Hz (sub-bass)

### Other Presets

Available in `presets.c`:

1. **`preset_ring_16_resonator()`** - Ring, resonators
2. **`preset_small_world_8_oscillator()`** - Small-world, self-oscillators
3. **`preset_clusters_16()`** - 2 clusters with bridge
4. **`preset_hub_spoke_16()`** - Hub-and-spokes (reserved)

To change default, modify `hub_controller.c:hub_send_default_config()`.

---

## Network Protocol

### Discovery

```
Hub                     Node
 |                       |
 |------- HELLO -------->| (broadcast)
 |<------ HELLO ---------| (response)
 |------- OFFER -------->| (config offer)
 |<------ JOIN ----------| (accept)
```

### Configuration

```
Hub                     Node
 |                       |
 |---- CFG_BEGIN ------->|
 |---- CFG_CHUNK ------->|
 |---- CFG_CHUNK ------->|
 |---- CFG_END --------->|
 |<----- CFG_ACK --------| (success)
```

### Runtime

```
Hub                     Node
 |                       |
 |------- START -------->| (session start)
 |                       |
 |------- POKE --------->| (MIDI event)
 |------- POKE --------->|
 |                       |
 |<---- HEARTBEAT -------| (every 5s)
```

---

## API Reference

### Hub Initialization

```c
hub_controller_t hub;
esp_now_manager_t network;

esp_now_manager_init(&network, 0);  // Hub is node 0
hub_controller_init(&hub, 0, &network, true);  // Use defaults
hub_midi_init(&hub);
```

### Discovery

```c
hub_start_discovery(&hub, 5000);  // 5s timeout
vTaskDelay(pdMS_TO_TICKS(5000));  // Wait
uint8_t num_nodes = hub_get_num_registered(&hub);
```

### Configuration

```c
if (hub.use_default_config) {
    hub_send_default_config(&hub);
} else {
    hub_send_config(&hub, &custom_config);
}
```

### Session Control

```c
hub_start_session(&hub);  // Start all nodes
// ... session running ...
hub_stop_session(&hub);   // Stop all nodes
```

### MIDI Processing

```c
// In MIDI task loop
while (1) {
    hub_midi_process(&hub);  // Process UART input
    hub_process_drive_notes(&hub);  // Send sustained pokes (ch 2)
    vTaskDelay(pdMS_TO_TICKS(1));
}
```

---

## Monitoring

### Serial Output

```c
hub_print_status(&hub);
```

Output:
```
I (30000) HUB: === Hub Status ===
I (30000) HUB: State: 4
I (30000) HUB: Registered nodes: 8
I (30000) HUB: Active MIDI notes: 3
I (30000) HUB: Pokes sent: 1247
I (30000) HUB: Discovery attempts: 1
I (30000) HUB:   Node 0: registered=1 configured=1 running=1
I (30000) HUB:   Node 1: registered=1 configured=1 running=1
...
```

### Heartbeat Monitoring

Hub checks for stale nodes (no heartbeat in 10s):

```c
I (45000) HUB: Node 3 heartbeat timeout (12345 ms)
```

---

## Troubleshooting

### No Nodes Found

**Problem**: Discovery timeout, 0 nodes registered

**Solutions**:
1. Check node firmware is flashed and running
2. Verify ESP-NOW initialized on both hub and nodes
3. Check WiFi channel (must match)
4. Reduce distance (<10m for testing)

### MIDI Not Working

**Problem**: No MIDI input detected

**Solutions**:
1. Check MIDI hardware (6N138 circuit)
2. Verify GPIO 16 connections
3. Test with MIDI monitor software first
4. Check baud rate (31250)
5. Verify UART initialization in logs

### Nodes Not Responding to Pokes

**Problem**: Hub sends pokes but nodes don't respond

**Solutions**:
1. Verify session started (`hub_start_session()`)
2. Check node audio output (nodes generate audio, not hub)
3. Verify poke messages received (node serial output)
4. Check coupling strength (not too weak)

---

## Performance

### CPU Usage (Hub)

- MIDI task: ~5%
- Discovery task: ~10% (during discovery only)
- Heartbeat task: <1%
- **Total**: ~15% average

### Memory Usage

- Hub state: ~2 KB
- Network manager: ~4 KB
- Total: ~6 KB

### Network Traffic

- Discovery: Burst (~100 packets over 5s)
- Runtime: ~10-100 pokes/s (depends on MIDI activity)
- Heartbeats: 1 packet/5s per node

---

## Example: Complete Hub Session

```c
void app_main(void) {
    // 1. Initialize
    nvs_flash_init();
    esp_now_manager_t network;
    esp_now_manager_init(&network, 0);

    hub_controller_t hub;
    hub_controller_init(&hub, 0, &network, true);
    hub_midi_init(&hub);

    // 2. Discovery
    hub_start_discovery(&hub, 5000);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 3. Configuration
    hub_send_default_config(&hub);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 4. Start session
    hub_start_session(&hub);

    // 5. MIDI loop
    while (1) {
        hub_midi_process(&hub);
        hub_process_drive_notes(&hub);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

---

## Future Enhancements

### Phase 3
- [ ] Custom configuration loading (JSON/binary)
- [ ] Web interface for monitoring
- [ ] MIDI CC mapping (coupling, damping, etc.)
- [ ] Multi-hub support (>16 nodes)

### Phase 4
- [ ] Bluetooth MIDI (BLE)
- [ ] OSC input support
- [ ] Real-time parameter control
- [ ] Recording/playback of poke sequences

---

**See Also**:
- `API.md` - Message protocol reference
- `DESIGN_SPEC.md` - Audio-first design philosophy
- `IMPLEMENTATION.md` - Python → C mapping

**Hub Code**:
- `config/hub_controller.h` - Hub API
- `config/hub_controller.c` - Hub implementation
- `hub_main.c` - Hub entry point
- `config/presets.c` - Default configurations
