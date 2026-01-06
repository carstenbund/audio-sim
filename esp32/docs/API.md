# API Reference

**ESP32 Modal Resonator Network - Message Protocol & Node API**

---

## Message Protocol

### Message Flow Diagram

```
Controller                     Node 0              Node 1              Node N
    |                             |                   |                   |
    |-------- HELLO -------------->|                  |                   |
    |                             |                   |                   |
    |<------- HELLO --------------|                   |                   |
    |<------- HELLO -------------------------------|                   |
    |<------- HELLO ----------------------------------------------------|
    |                             |                   |                   |
    |-------- OFFER -------------->|                  |                   |
    |-------- OFFER ---------------------------->|                   |
    |                             |                   |                   |
    |<------- JOIN ---------------|                   |                   |
    |<------- JOIN ------------------------------|                   |
    |                             |                   |                   |
    |---- CFG_BEGIN -------------->|                  |                   |
    |---- CFG_CHUNK -------------->|                  |                   |
    |---- CFG_CHUNK -------------->|                  |                   |
    |---- CFG_END ---------------->|                  |                   |
    |                             |                   |                   |
    |<------- CFG_ACK -------------|                   |                   |
    |                             |                   |                   |
    |-------- START -------------->| (broadcast)      |                   |
    |                             |                   |                   |
    |                             |---- POKE -------->|                   |
    |                             |<--- POKE ---------|                   |
    |                             |---- POKE ------------------------------>|
```

---

## Message Types

### Discovery Phase

#### HELLO
**Purpose**: Node announces presence
**Direction**: Node → Broadcast
**Frequency**: Every 1-5s during discovery

```c
typedef struct {
    message_header_t header;
    uint8_t mac_address[6];
    uint8_t capabilities;
    char name[16];
} msg_hello_t;
```

**Example**:
```json
{
  "type": "HELLO",
  "source_id": 3,
  "mac": "AA:BB:CC:DD:EE:FF",
  "name": "Node_003"
}
```

---

#### OFFER
**Purpose**: Controller offers to configure nodes
**Direction**: Controller → Broadcast
**Frequency**: Once after discovery

```c
typedef struct {
    message_header_t header;
    char session_id[32];
    uint16_t config_size;
    uint8_t num_nodes;
} msg_offer_t;
```

---

#### JOIN
**Purpose**: Node accepts configuration
**Direction**: Node → Controller
**Frequency**: Once per discovery

```c
typedef struct {
    message_header_t header;
    uint8_t requested_node_id;
    uint8_t mac_address[6];
} msg_join_t;
```

---

### Configuration Phase

#### CFG_BEGIN
**Purpose**: Start configuration transfer
**Direction**: Controller → Node
**Frequency**: Once per session

```c
typedef struct {
    message_header_t header;
    uint16_t total_size;
    uint8_t num_chunks;
    uint32_t checksum;
} msg_cfg_begin_t;
```

---

#### CFG_CHUNK
**Purpose**: Transfer configuration data
**Direction**: Controller → Node
**Frequency**: Multiple (one per chunk)

```c
typedef struct {
    message_header_t header;
    uint8_t chunk_idx;
    uint8_t chunk_size;
    uint8_t data[200];
} msg_cfg_chunk_t;
```

**Notes**:
- Max chunk size: 200 bytes
- Chunks sent sequentially
- Node stores in buffer until CFG_END

---

#### CFG_END
**Purpose**: Finish configuration transfer
**Direction**: Controller → Node
**Frequency**: Once per session

```c
typedef struct {
    message_header_t header;
    uint32_t checksum;
} msg_cfg_end_t;
```

---

#### CFG_ACK / CFG_NACK
**Purpose**: Acknowledge configuration
**Direction**: Node → Controller
**Frequency**: Once per session

```c
typedef struct {
    message_header_t header;
    uint8_t status;  // 0 = OK, else error code
} msg_cfg_ack_t;
```

**Error Codes**:
- `0x00`: Success
- `0x01`: Checksum mismatch
- `0x02`: Buffer overflow
- `0x03`: Invalid JSON
- `0x04`: Unsupported topology

---

### Session Control

#### START
**Purpose**: Begin session (unlock runtime)
**Direction**: Controller → Broadcast
**Frequency**: Once per session

```c
typedef struct {
    message_header_t header;
    uint32_t start_time_ms;
} msg_start_t;
```

**Effect**:
- Nodes enter running mode
- Modal integration starts
- Audio generation begins
- Configuration locked

---

#### STOP
**Purpose**: End session
**Direction**: Controller → Broadcast
**Frequency**: Once per session

```c
typedef struct {
    message_header_t header;
} msg_stop_t;
```

**Effect**:
- Nodes stop modal integration
- Audio fades out
- Network stays active (for next session)

---

### Runtime Events

#### POKE
**Purpose**: Excitation event (energy injection)
**Direction**: Node → Neighbor(s)
**Frequency**: Event-driven (e.g., on MIDI note)

```c
typedef struct {
    message_header_t header;
    float strength;           // [0, 1]
    float phase_hint;         // radians, or -1 for random
    float mode_weights[4];    // per-mode weights
} msg_poke_t;
```

**Example**:
```json
{
  "type": "POKE",
  "source_id": 2,
  "dest_id": 3,
  "strength": 0.8,
  "phase_hint": 1.57,
  "mode_weights": [1.0, 0.8, 0.3, 0.5]
}
```

**Excitation Model**:
```
a_k ← a_k + strength · mode_weights[k] · e^(i·phase_hint)
```

Applied via short envelope (1-20ms).

---

#### STATE (Optional)
**Purpose**: Broadcast current modal state
**Direction**: Node → Neighbors
**Frequency**: Optional, ~10-100 Hz

```c
typedef struct {
    message_header_t header;
    float mode0_real;
    float mode0_imag;
    float amplitude;
} msg_state_t;
```

**Notes**:
- **Not required** for core operation
- Useful for monitoring/debugging
- Can be used for weak coupling (optional design)

---

## Configuration Blob Format

### JSON Schema

```json
{
  "session_id": "string",
  "topology": "ring|small_world|clusters|hub_spoke",
  "num_nodes": 16,
  "global_coupling": 0.3,
  "control_rate_hz": 500,
  "nodes": [
    {
      "node_id": 0,
      "personality": "resonator|self_oscillator",
      "modes": [
        {"omega": 2765.0, "gamma": 0.5, "weight": 1.0},
        {"omega": 2777.0, "gamma": 0.6, "weight": 0.8},
        {"omega": 5530.0, "gamma": 1.0, "weight": 0.3},
        {"omega": 345.0,  "gamma": 0.1, "weight": 0.5}
      ],
      "neighbors": [1, 15],
      "coupling_strength": 0.3,
      "carrier_freq_hz": 440.0,
      "audio_gain": 0.7
    }
  ]
}
```

---

### Binary Format (Compact)

For bandwidth efficiency, configuration can use binary format:

```
Header (8 bytes):
  - session_id (4 bytes, hash)
  - topology (1 byte)
  - num_nodes (1 byte)
  - control_rate_hz (2 bytes)

Per-Node Block (40 bytes):
  - node_id (1 byte)
  - personality (1 byte)
  - modes[4] × 3 floats = 48 bytes (omega, gamma, weight)
  - neighbors (8 bytes, bitmask)
  - coupling_strength (4 bytes)
  - carrier_freq_hz (4 bytes)
  - audio_gain (4 bytes)

Total: 8 + (40 × num_nodes) bytes
Example: 16 nodes = 648 bytes (fits in 4 chunks)
```

---

## Node API (C)

### Initialization

```c
void modal_node_init(modal_node_t* node, uint8_t node_id, node_personality_t personality);
```

### Configuration

```c
void modal_node_set_mode(modal_node_t* node, uint8_t mode_idx,
                         float omega, float gamma, float weight);
void modal_node_set_neighbors(modal_node_t* node, uint8_t* neighbor_ids, uint8_t num_neighbors);
```

### Runtime

```c
void modal_node_step(modal_node_t* node);  // Call at control rate
void modal_node_apply_poke(modal_node_t* node, const poke_event_t* poke);
float modal_node_get_amplitude(const modal_node_t* node);
```

### Audio

```c
void audio_synth_init(audio_synth_t* synth, const modal_node_t* node, float carrier_freq_hz);
int16_t* audio_synth_generate_buffer(audio_synth_t* synth);
```

### Network

```c
bool esp_now_manager_init(esp_now_manager_t* mgr, uint8_t my_node_id);
bool esp_now_send_message(esp_now_manager_t* mgr, uint8_t dest_id,
                         const network_message_t* msg, size_t len);
```

---

## Example Usage

### Sending a Poke

```c
// Create poke message
network_message_t msg;
float weights[4] = {1.0, 0.8, 0.3, 0.5};

protocol_create_poke(&msg, my_node_id, neighbor_id,
                     0.8f,      // strength
                     -1.0f,     // random phase
                     weights);

// Send via ESP-NOW
esp_now_send_message(&network_mgr, neighbor_id, &msg, sizeof(msg_poke_t));
```

### Receiving a Poke

```c
void on_network_message(const network_message_t* msg) {
    if (msg->header.type == MSG_POKE) {
        poke_event_t poke = {
            .source_node_id = msg->poke.header.source_id,
            .strength = msg->poke.strength,
            .phase_hint = msg->poke.phase_hint
        };
        memcpy(poke.mode_weights, msg->poke.mode_weights, sizeof(poke.mode_weights));

        modal_node_apply_poke(&node, &poke);
    }
}
```

---

## Performance Notes

### Packet Sizes
- HELLO: 24 bytes
- POKE: 28 bytes
- STATE: 20 bytes
- CFG_CHUNK: 208 bytes (max)

### Latency Budget
- ESP-NOW TX: ~0.2 ms
- Network propagation: 1-3 ms
- ESP-NOW RX: ~0.2 ms
- **Total roundtrip: 2-5 ms**

### Bandwidth (16-node ring)
- Poke rate: ~10-100 Hz per node
- Total traffic: 16 nodes × 100 Hz × 28 bytes = **44.8 KB/s**
- ESP-NOW capacity: ~250 KB/s
- **Utilization: ~18%** ✓

---

## Next Steps

1. Implement configuration chunking
2. Add CRC32 validation
3. Test 16-node network latency
4. Optimize packet sizes
5. Add OTA update protocol

---

**End of API Reference**
