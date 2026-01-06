# Development Tools

**Utilities for developing and testing the ESP32 modal resonator network**

---

## Tools Overview

### configurator/
Configuration blob generator and validator
- Generate JSON configs from presets
- Validate topology definitions
- Convert JSON to binary format
- Checksum calculation

### network_bench.py
Network latency and throughput testing
- Measure ESP-NOW roundtrip latency
- Test packet loss rates
- Stress test with high packet rates

### midi_sender.py
MIDI test input generator
- Send test MIDI notes to controller
- Pattern generators (scales, arpeggios)
- Timing validation

### firmware_flasher.sh
Multi-node firmware flashing script
- Flash multiple ESP32s in batch
- Auto-detect USB ports
- Set node IDs automatically

---

## Usage Examples

### Generate Configuration

```bash
cd configurator
python generate_config.py \
  --topology ring \
  --num-nodes 16 \
  --output config_ring_16.json
```

### Test Network Latency

```bash
python network_bench.py \
  --nodes 16 \
  --topology ring \
  --duration 60
```

Output:
```
Network Benchmark Results:
- Avg latency: 3.2ms
- Max latency: 7.8ms
- Packet loss: 0.02%
- Throughput: 45 KB/s
```

### Send Test MIDI

```bash
python midi_sender.py \
  --port /dev/ttyUSB0 \
  --pattern arpeggio \
  --tempo 120
```

### Flash Multiple Nodes

```bash
./firmware_flasher.sh --start-id 0 --count 8
```

---

## Tool Details

### configurator/generate_config.py

Generates session configuration JSON files.

**Arguments**:
- `--topology`: ring, small_world, clusters, hub_spoke
- `--num-nodes`: Number of nodes (1-16)
- `--coupling`: Global coupling strength [0,1]
- `--output`: Output JSON file

**Example**:
```bash
python generate_config.py \
  --topology small_world \
  --num-nodes 12 \
  --coupling 0.3 \
  --rewire-prob 0.1 \
  --output config.json
```

---

### network_bench.py

ESP-NOW network performance testing.

**Arguments**:
- `--nodes`: Number of nodes to test
- `--topology`: Network topology
- `--duration`: Test duration (seconds)
- `--packet-rate`: Packets per second

**Metrics**:
- Roundtrip latency (min/avg/max)
- Packet loss rate
- Throughput (bytes/sec)
- Jitter (latency variation)

---

### midi_sender.py

MIDI input generator for testing.

**Patterns**:
- `arpeggio`: C major arpeggio
- `scale`: Chromatic scale
- `random`: Random notes
- `chord`: Chord progression

**Example**:
```bash
# Send C major scale at 120 BPM
python midi_sender.py --pattern scale --key C --tempo 120
```

---

## Coming Soon

- `monitor.py`: Real-time node monitoring dashboard
- `recorder.py`: Record and playback poke events
- `analyzer.py`: Network topology visualizer
- `calibrator.py`: Audio latency calibration tool

---

**End of Tools Guide**
