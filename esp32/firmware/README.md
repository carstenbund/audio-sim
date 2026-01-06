# ESP32 Firmware

**Modal Resonator Node Firmware - ESP-IDF Project**

---

## Quick Start

### Prerequisites

1. **Install ESP-IDF v5.0+**
   ```bash
   # Clone ESP-IDF
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   git checkout v5.1
   ./install.sh
   source export.sh
   ```

2. **Verify Installation**
   ```bash
   idf.py --version
   # Should show: ESP-IDF v5.1 or higher
   ```

### Build and Flash

```bash
cd firmware
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Configuration

### Set Node ID

Before flashing, configure the node ID in `main/main.c`:

```c
#define MY_NODE_ID 0  // Change this for each node
```

**Or** use menuconfig:
```bash
idf.py menuconfig
# → Component config → Modal Node → Node ID
```

### Audio Settings

Configure audio in menuconfig:
```bash
idf.py menuconfig
# → Component config → Audio
#   - Sample Rate: 48000 Hz
#   - Bits per Sample: 16
#   - I2S GPIO: BCK=25, WS=26, DIN=27
```

### Network Settings

ESP-NOW is auto-configured. To change WiFi channel:
```bash
idf.py menuconfig
# → Component config → WiFi → WiFi channel
```

---

## Project Structure

```
firmware/
├── CMakeLists.txt          # Root build file
├── sdkconfig.defaults      # Default configuration
├── main/                   # Application code
│   ├── main.c             # Entry point
│   ├── CMakeLists.txt     # Component build file
│   ├── core/              # Modal resonator
│   │   ├── modal_node.h
│   │   └── modal_node.c
│   ├── audio/             # Audio synthesis
│   │   ├── audio_synth.h
│   │   ├── audio_synth.c
│   │   ├── audio_i2s.h
│   │   └── audio_i2s.c
│   ├── network/           # ESP-NOW
│   │   ├── protocol.h
│   │   ├── protocol.c
│   │   ├── esp_now_manager.h
│   │   └── esp_now_manager.c
│   ├── config/            # Configuration
│   │   ├── session_config.h
│   │   ├── session_config.c
│   │   ├── topology.c
│   │   └── presets.c
│   └── utils/             # Utilities
│       ├── complex_math.c
│       └── crc32.c
└── components/            # Reusable libraries (future)
```

---

## Implementation Status

### Completed ✓
- [x] Project structure
- [x] Header files and APIs
- [x] CMake build system
- [x] Main entry point
- [x] Task architecture (audio/control/network)

### In Progress 🚧
- [ ] Modal node implementation (core/modal_node.c)
- [ ] Audio synthesis (audio/audio_synth.c)
- [ ] I2S driver (audio/audio_i2s.c)
- [ ] Protocol implementation (network/protocol.c)
- [ ] ESP-NOW manager (network/esp_now_manager.c)

### TODO 📝
- [ ] Configuration loading (config/session_config.c)
- [ ] Topology generators (config/topology.c)
- [ ] Preset configurations (config/presets.c)
- [ ] Complex math helpers (utils/complex_math.c)
- [ ] CRC32 checksum (utils/crc32.c)
- [ ] Unit tests
- [ ] Integration tests

---

## Building

### Full Clean Build
```bash
idf.py fullclean
idf.py build
```

### Flash Specific Partition
```bash
idf.py app-flash  # Flash app only (faster)
```

### Monitor Serial Output
```bash
idf.py monitor
# Press Ctrl+] to exit
```

### Build + Flash + Monitor
```bash
idf.py flash monitor
```

---

## Debugging

### Enable Debug Logs

In `sdkconfig` or menuconfig:
```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```

In code:
```c
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

ESP_LOGD(TAG, "Debug message: %d", value);
```

### Profile with JTAG

Connect JTAG adapter and use OpenOCD:
```bash
openocd -f board/esp32c3-builtin.cfg
```

### Measure Task CPU Usage

Enable runtime stats in menuconfig:
```
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
```

Query from code:
```c
char stats_buffer[1024];
vTaskGetRunTimeStats(stats_buffer);
ESP_LOGI(TAG, "Task stats:\n%s", stats_buffer);
```

---

## Testing

### Single Node Test

1. Flash firmware
2. Open monitor
3. Check for:
   - Boot message
   - WiFi MAC address
   - Task creation success
   - Audio task running

Expected output:
```
I (123) MODAL_NODE: === ESP32 Modal Resonator Node ===
I (125) MODAL_NODE: Node ID: 0
I (127) MODAL_NODE: Audio task started on core 1
I (129) MODAL_NODE: Control task started on core 0
I (131) MODAL_NODE: Network task started on core 0
```

### Two-Node Coupling Test

1. Flash two nodes (IDs 0 and 1)
2. Power on both
3. Check for discovery messages
4. Manually send poke from node 0 to node 1
5. Verify audio response on node 1

---

## Performance Targets

### Audio Task
- **Sample rate**: 48 kHz
- **Buffer size**: 480 samples (10ms)
- **CPU usage**: <30% (Core 1)
- **Latency**: <10ms (buffer + processing)

### Control Task
- **Rate**: 500 Hz (2ms period)
- **CPU usage**: <20% (Core 0)
- **Jitter**: <100µs

### Network Task
- **Latency**: 2-5ms (ESP-NOW roundtrip)
- **Packet loss**: <1%
- **Throughput**: ~50 KB/s (16 nodes)

---

## Known Issues

1. **ESP32-C3 FPU**: Single-precision only (use `float`, not `double`)
2. **Complex math**: No native library (implement in utils/complex_math.c)
3. **ESP-NOW limits**: Max 20 peers (enough for 16-node network)
4. **I2S DMA**: Requires aligned buffers (use `__attribute__((aligned(4)))`)

---

## Next Steps

1. **Implement core modules** (modal_node.c, audio_synth.c, etc.)
2. **Test single node** audio output
3. **Implement ESP-NOW** communication
4. **Test two-node coupling**
5. **Scale to 16 nodes**

---

## Contributing

See project root README for contribution guidelines.

---

**Status**: Framework complete, implementation in progress

**Target**: Single-node prototype by end of Phase 1
