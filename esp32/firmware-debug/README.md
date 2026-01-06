# ESP32 Modal Resonator - Debug Firmware

**Debug-enabled build with integrated test harnesses and simulators**

This is a debug version of the ESP32 Modal Resonator firmware with added:
- Test infrastructure for all 5 rollout phases
- MIDI simulator (no hardware required)
- Hub simulator (single-node testing)
- Network traffic generator
- Verbose debug logging
- Modal state monitoring utilities

## Table of Contents

1. [Quick Start](#quick-start)
2. [Test Phases](#test-phases)
3. [Configuration](#configuration)
4. [Building and Flashing](#building-and-flashing)
5. [Running Tests](#running-tests)
6. [Simulators](#simulators)
7. [Debug Utilities](#debug-utilities)
8. [Updating Production Code](#updating-production-code)

---

## Quick Start

### Phase 1: Single Node Syntax Testing

```bash
cd firmware-debug
idf.py menuconfig
# Navigate to "Debug and Testing Configuration"
# Set "Test Phase" to 1
# Enable "Auto-run Tests on Boot"
# Save and exit

idf.py build flash monitor
```

Expected output:
```
I (2000) DEBUG_TEST: === Running Test Phase 1: Single Node Syntax ===
I (2010) DEBUG_TEST: [Test 1.1] Task Scheduling
I (2020) DEBUG_TEST:   ✓ Audio task running on core 1
I (2030) DEBUG_TEST:   ✓ Control task running on core 0
I (2040) DEBUG_TEST:   ✓ Network task running on core 0
I (2050) DEBUG_TEST:   ✓ PASSED
...
I (3000) DEBUG_TEST: === Test Summary ===
I (3010) DEBUG_TEST: Tests run: 3
I (3020) DEBUG_TEST: Passed: 3
I (3030) DEBUG_TEST: Failed: 0
```

---

## Test Phases

The debug firmware supports **5 incremental test phases** that build complexity gradually:

### Phase 1: Single Node Syntax Testing
**Purpose**: Verify basic firmware functionality without network

**Tests**:
- `1.1` Task scheduling (audio, control, network on correct cores)
- `1.2` Modal state updates (integration loop running)
- `1.3` Self-poke injection (internal excitation works)

**Hardware**: Single ESP32 + serial monitor

**Expected Duration**: 30 seconds

---

### Phase 2: Two-Node Protocol Testing
**Purpose**: Verify ESP-NOW messaging and peer discovery

**Tests**:
- `2.1` Peer discovery (nodes find each other)
- `2.2` Poke transmission (bidirectional messaging)
- `2.3` Network statistics (latency, packet loss)

**Hardware**: Two ESP32 nodes (no audio required)

**Expected Duration**: 2-3 minutes

**Configuration**:
- Node A: `MY_NODE_ID = 0`
- Node B: `MY_NODE_ID = 1`

---

### Phase 3: Single Node + Audio
**Purpose**: Verify audio synthesis and 4-channel output

**Tests**:
- `3.1` Channel isolation (each mode → correct channel)
- `3.2` Beating test (440Hz + 442Hz = 2Hz beating)
- `3.3` Modal decay timing (verify damping constants)
- `3.4` Self-oscillator mode (unstable mode grows)
- `3.5` Multi-mode harmonic response (frequency spacing)

**Hardware**: Single ESP32 + 4-channel DAC + speakers/scope

**Expected Duration**: 5 minutes (requires listening/measurement)

**Audio Output**:
- Channel 0: Mode 0 (440 Hz)
- Channel 1: Mode 1 (442 Hz)
- Channel 2: Mode 2 (880 Hz)
- Channel 3: Mode 3 (55 Hz)

---

### Phase 4: Distributed Audio
**Purpose**: Verify network-driven audio responses

**Tests**:
- `4.1` Network-driven audio (remote poke → local audio)
- `4.2` Configuration distribution (hub → node config)

**Hardware**: Two ESP32 nodes with audio output

**Expected Duration**: 5 minutes

---

### Phase 5: MIDI Integration
**Purpose**: Verify MIDI input and note routing

**Tests**:
- `5.1` MIDI hardware test (receive MIDI notes)
- `5.2` Note routing (MIDI note → correct node poke)
- `5.3` Polyphonic performance (multiple simultaneous notes)

**Hardware**: Hub node + MIDI controller + multiple nodes

**Expected Duration**: 10 minutes

---

## Configuration

### Using `menuconfig`

```bash
idf.py menuconfig
```

Navigate to **"Debug and Testing Configuration"**:

| Option | Description | Default |
|--------|-------------|---------|
| **Enable Debug Features** | Master enable for all debug functionality | YES |
| **Test Phase (1-5)** | Which test phase to run | 1 |
| **Auto-run Tests on Boot** | Automatically run tests after startup | YES |
| **Verbose Debug Logging** | Print modal state, network stats, timing | YES |
| **Enable MIDI Simulator** | Simulate MIDI input without hardware | NO |
| **Enable Hub Simulator** | Simplified hub for single-node testing | NO |
| **Enable Poke Injection Utilities** | Manual poke injection for testing | YES |
| **Enable Network Traffic Generator** | Stress testing with synthetic messages | NO |
| **Enable Debug Monitor Task** | Background task that logs system state | YES |

### Example Configurations

#### Phase 1: Single Node Syntax
```
DEBUG_ENABLE = y
DEBUG_TEST_PHASE = 1
DEBUG_AUTO_RUN_TESTS = y
DEBUG_VERBOSE_LOGGING = y
DEBUG_ENABLE_MIDI_SIMULATOR = n
DEBUG_ENABLE_HUB_SIMULATOR = n
DEBUG_MONITOR_TASK_ENABLED = y
```

#### Phase 3: Audio Testing with Hub Simulator
```
DEBUG_ENABLE = y
DEBUG_TEST_PHASE = 3
DEBUG_AUTO_RUN_TESTS = y
DEBUG_VERBOSE_LOGGING = y
DEBUG_ENABLE_MIDI_SIMULATOR = n
DEBUG_ENABLE_HUB_SIMULATOR = y  # <-- Enables hub simulator
DEBUG_MONITOR_TASK_ENABLED = y
```

#### Phase 5: MIDI with Simulator
```
DEBUG_ENABLE = y
DEBUG_TEST_PHASE = 5
DEBUG_AUTO_RUN_TESTS = y
DEBUG_VERBOSE_LOGGING = y
DEBUG_ENABLE_MIDI_SIMULATOR = y  # <-- Simulates MIDI input
DEBUG_ENABLE_HUB_SIMULATOR = n
DEBUG_MONITOR_TASK_ENABLED = y
```

---

## Building and Flashing

### Prerequisites

- ESP-IDF v5.0+ installed and configured
- ESP32 hardware
- USB cable for serial connection

### Build Process

```bash
cd /home/user/audio-sim/esp32/firmware-debug

# Configure
idf.py menuconfig
# Set test phase and debug options

# Build
idf.py build

# Flash and monitor
idf.py flash monitor

# Or separate commands:
idf.py flash
idf.py monitor
```

### Build Targets

```bash
# Clean build
idf.py clean build

# Specific flash baud rate
idf.py -p /dev/ttyUSB0 -b 921600 flash

# Monitor only
idf.py monitor
```

### Serial Monitor

Press `Ctrl+]` to exit monitor.

Useful monitor filters:
```bash
# Show only test results
idf.py monitor | grep "DEBUG_TEST"

# Show only modal state
idf.py monitor | grep "MODAL"

# Show only network events
idf.py monitor | grep "ESP_NOW"
```

---

## Running Tests

### Automatic Test Execution

If `DEBUG_AUTO_RUN_TESTS = y`, tests run automatically 2 seconds after boot:

```
I (1000) MODAL_NODE: All tasks started successfully
I (3000) MODAL_NODE: Auto-running test phase 1
I (3010) DEBUG_TEST: === Running Test Phase 1 ===
...
I (5000) DEBUG_TEST: === Test Summary ===
I (5010) DEBUG_TEST: Tests run: 3
I (5020) DEBUG_TEST: Passed: 3
I (5030) DEBUG_TEST: Failed: 0
```

### Manual Test Execution

If `DEBUG_AUTO_RUN_TESTS = n`, tests must be triggered manually (requires custom trigger mechanism).

### Test Output Format

Each test prints:
- **Test ID**: `[Test X.Y] Description`
- **Progress**: Step-by-step progress messages
- **Result**: `✓ PASSED` or `✗ FAILED: reason`

Example:
```
I (2000) DEBUG_TEST: [Test 1.2] Modal State Updates
I (2010) DEBUG_TEST:   Initial |a0| = 0.000
I (2020) DEBUG_TEST:   After 100ms: |a0| = 0.000
I (2500) DEBUG_TEST:   Integration running
I (2510) DEBUG_TEST:   ✓ PASSED: Modal integration verified
```

---

## Simulators

### MIDI Simulator

**Purpose**: Simulate MIDI note events without MIDI hardware

**Enable**: `DEBUG_ENABLE_MIDI_SIMULATOR = y`

**Behavior**:
- Generates MIDI Note On/Off events every 2 seconds
- Cycles through notes C4, E4, G4, C5 (60, 64, 67, 72)
- Simulates velocity (strength) variations
- Useful for Phase 5 testing without MIDI controller

**Output**:
```
I (5000) MIDI_SIM: MIDI Note On: note=60 velocity=100
I (7000) MIDI_SIM: MIDI Note Off: note=60
I (7010) MIDI_SIM: MIDI Note On: note=64 velocity=80
```

### Hub Simulator

**Purpose**: Simplified hub functionality for single-node testing

**Enable**: `DEBUG_ENABLE_HUB_SIMULATOR = y`

**Behavior**:
- Sends HELLO messages every 5 seconds
- Responds to JOIN requests with OFFER
- Distributes basic configuration
- Sends periodic START/STOP commands

**Output**:
```
I (5000) HUB_SIM: Broadcasting HELLO
I (5100) HUB_SIM: Received JOIN from node 1
I (5110) HUB_SIM: Sending OFFER to node 1
```

**Use Cases**:
- Phase 1 testing (simulate hub discovery)
- Phase 3 testing (single node + audio, no separate hub hardware)

### Network Traffic Generator

**Purpose**: Stress testing with synthetic ESP-NOW messages

**Enable**: `DEBUG_NETWORK_TRAFFIC_GEN = y`

**Behavior**:
- Generates high-rate poke messages
- Tests packet loss, latency, congestion
- Configurable message rate and payload size

**Warning**: High traffic may interfere with normal operation. Use only for stress testing.

---

## Debug Utilities

### Modal State Monitor

**Function**: `debug_print_modal_state(&ctx)`

**Output**:
```
I (1000) DEBUG: [MODAL] |a0|=0.123 |a1|=0.456 |a2|=0.078 |a3|=0.234
```

Prints complex amplitude magnitudes for all 4 modes.

### Network Statistics Monitor

**Function**: `debug_print_network_stats(&ctx)`

**Output**:
```
I (1000) DEBUG: [NETWORK] Peers: 2 | TX: 45 RX: 42 | Latency: 12ms
```

Prints peer count, TX/RX packet counts, average latency.

### Poke Injection

**Function**: `debug_inject_poke(&ctx, strength, mode_weights)`

**Example**:
```c
// Inject poke to mode 0 only
float weights[4] = {1.0f, 0.0f, 0.0f, 0.0f};
debug_inject_poke(&ctx, 1.0f, weights);

// Inject poke to all modes equally
float weights[4] = {0.25f, 0.25f, 0.25f, 0.25f};
debug_inject_poke(&ctx, 0.5f, weights);
```

Useful for manual testing and verification.

### Debug Monitor Task

**Purpose**: Background task that periodically logs system state

**Enable**: `DEBUG_MONITOR_TASK_ENABLED = y`

**Output** (every 5 seconds):
```
I (5000) DEBUG_MON: === System State ===
I (5010) DEBUG_MON: [MODAL] |a0|=0.123 |a1|=0.456 |a2|=0.078 |a3|=0.234
I (5020) DEBUG_MON: [NETWORK] Peers: 2 | TX: 45 RX: 42 | Latency: 12ms
I (5030) DEBUG_MON: [AUDIO] Master gain: 0.70 | Muted: NO
I (5040) DEBUG_MON: [SYSTEM] Uptime: 5000ms | Free heap: 234KB
```

---

## Updating Production Code

Once testing is complete and bugs are fixed, update the production firmware:

### Workflow

1. **Test in Debug Build**:
   ```bash
   cd firmware-debug
   # Make changes to source files
   idf.py build flash monitor
   # Verify fixes work
   ```

2. **Copy Changes to Production**:
   ```bash
   # Copy modified files to production firmware
   cp firmware-debug/main/core/modal_node.c firmware/main/core/
   cp firmware-debug/main/audio/audio_synth.c firmware/main/audio/
   # ... (copy all modified files)
   ```

3. **Test Production Build**:
   ```bash
   cd firmware
   idf.py build flash monitor
   # Verify production build works without debug overhead
   ```

4. **Commit Changes**:
   ```bash
   git add firmware/main/core/modal_node.c
   git commit -m "Fix modal integration timing issue (from debug testing)"
   ```

### Files to Update

Typically modified files:
- `main/core/modal_node.c` - Modal dynamics fixes
- `main/audio/audio_synth.c` - Audio synthesis fixes
- `main/network/esp_now_manager.c` - Network fixes
- `main/network/protocol.c` - Protocol fixes
- `main/config/session_config.c` - Configuration fixes

**Do NOT copy**:
- `main/debug/*` - Debug-specific files
- Modified `main.c` - Contains debug-specific initialization

---

## Troubleshooting

### Tests Fail to Run

**Symptom**: No test output after boot

**Check**:
1. `DEBUG_ENABLE = y` in menuconfig
2. `DEBUG_AUTO_RUN_TESTS = y` in menuconfig
3. Flash successful (no errors during `idf.py flash`)
4. Serial monitor connected (`idf.py monitor`)

### Compilation Errors

**Symptom**: Build fails with missing symbols

**Fix**:
```bash
# Clean and rebuild
idf.py clean
idf.py build
```

**Common Issues**:
- Missing `#include "debug/debug_test.h"` in main.c
- `debug/debug_test.c` not in CMakeLists.txt SRCS
- `debug` not in CMakeLists.txt INCLUDE_DIRS

### Network Tests Fail (Phase 2)

**Symptom**: Nodes don't discover each other

**Check**:
1. Both nodes have **different** `MY_NODE_ID` (0 and 1)
2. Both nodes powered and running
3. Nodes within WiFi range (~10m)
4. ESP-NOW initialized successfully (check logs)

**Debug**:
```
I (1000) ESP_NOW: ESP-NOW initialized
I (1010) ESP_NOW: My MAC: AA:BB:CC:DD:EE:FF
I (5000) ESP_NOW: Discovery timeout, no peers found  # <-- Issue
```

### Audio Not Working (Phase 3)

**Symptom**: No audio output on DAC

**Check**:
1. I2S pins connected: BCK=25, WS=26, DIN=27
2. DAC powered and configured correctly
3. Modes are active (check modal state logs)
4. Master gain > 0 (`audio_gain = 0.7` in main.c)
5. Not muted (`audio_synth_set_mute(&g_audio, false)`)

**Debug**:
```
I (1000) AUDIO_I2S: I2S driver initialized successfully
I (1010) AUDIO_SYNTH: Audio synthesis initialized (48kHz, 4 channels)
I (2000) DEBUG_MON: [MODAL] |a0|=0.000 |a1|=0.000 |a2|=0.000 |a3|=0.000  # <-- No excitation
```

Try manual poke:
```c
float weights[4] = {1.0f, 0.0f, 0.0f, 0.0f};
debug_inject_poke(&g_debug_ctx, 1.0f, weights);
```

### High CPU Usage

**Symptom**: System becomes unresponsive

**Cause**: Too many debug features enabled simultaneously

**Fix**: Disable unused features:
```
DEBUG_VERBOSE_LOGGING = n        # Reduces logging overhead
DEBUG_MONITOR_TASK_ENABLED = n   # Disables monitoring task
DEBUG_NETWORK_TRAFFIC_GEN = n    # Disable traffic generator
```

---

## Reference

### Test Phase Summary

| Phase | Description | Hardware | Duration | Key Tests |
|-------|-------------|----------|----------|-----------|
| **1** | Single node syntax | 1 ESP32 | 30s | Tasks, modal state, self-poke |
| **2** | Two-node protocol | 2 ESP32 | 2-3min | Discovery, poke TX, network stats |
| **3** | Single node audio | 1 ESP32 + DAC | 5min | Channel isolation, beating, decay |
| **4** | Distributed audio | 2 ESP32 + DAC | 5min | Network audio, config distribution |
| **5** | MIDI integration | Hub + MIDI + nodes | 10min | MIDI RX, note routing, polyphony |

### Debug Configuration Quick Reference

```
CONFIG_DEBUG_ENABLE=y                    # Master enable
CONFIG_DEBUG_TEST_PHASE=1                # Test phase (1-5)
CONFIG_DEBUG_AUTO_RUN_TESTS=y            # Auto-run on boot
CONFIG_DEBUG_VERBOSE_LOGGING=y           # Verbose logs
CONFIG_DEBUG_ENABLE_MIDI_SIMULATOR=n     # MIDI simulator
CONFIG_DEBUG_ENABLE_HUB_SIMULATOR=n      # Hub simulator
CONFIG_DEBUG_POKE_INJECTION_ENABLED=y    # Poke utilities
CONFIG_DEBUG_NETWORK_TRAFFIC_GEN=n       # Traffic generator
CONFIG_DEBUG_MONITOR_TASK_ENABLED=y      # Monitor task
```

### Related Documentation

- **Production Firmware**: `../firmware/README.md`
- **Testing Guide**: `../TESTING.md` (849 lines, comprehensive)
- **4-Channel Audio**: `../docs/AUDIO_4CHANNEL.md`
- **Design Spec**: `../DESIGN_SPEC.md`
- **Implementation**: `../IMPLEMENTATION.md`

---

## Support

For issues or questions:
1. Check `TESTING.md` for detailed test procedures
2. Review serial output for error messages
3. Check hardware connections
4. Verify configuration settings

---

**Last Updated**: 2026-01-06
**Version**: 1.0 (Debug Build)
**Firmware Version**: 1.0 (DEBUG BUILD)
