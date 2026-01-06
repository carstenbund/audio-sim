# Configuration System

**Phase 3 Implementation - Configuration Distribution and Topology Generation**

This document describes the complete configuration system for the ESP32 distributed modal resonator network.

## Table of Contents

1. [Overview](#overview)
2. [Configuration Format](#configuration-format)
3. [Topology Generators](#topology-generators)
4. [Configuration Distribution Protocol](#configuration-distribution-protocol)
5. [Node Configuration Reception](#node-configuration-reception)
6. [API Reference](#api-reference)
7. [Examples](#examples)

---

## Overview

The configuration system manages:

- **Network topology**: Which nodes connect to which
- **Modal parameters**: Frequencies, damping, weights per node
- **Node personalities**: Resonator vs self-oscillator behavior
- **Audio parameters**: Carrier frequencies, gain levels

### Design Goals

- **Message-only distribution**: No web server or complex protocols
- **Chunked transfer**: Works within ESP-NOW 250-byte limit
- **Reliable delivery**: CRC32 checksums and acknowledgments
- **Flexible topologies**: Ring, small-world, clusters, hub-spoke, custom

---

## Configuration Format

### Binary Format

Configuration is serialized as a direct memory copy of `session_config_t`:

```c
typedef struct {
    char session_id[32];                    // Unique session ID
    topology_type_t topology;               // Topology type
    uint8_t num_nodes;                      // Number of nodes

    node_config_t nodes[16];                // Per-node configs

    float global_coupling;                  // Default coupling
    uint16_t control_rate_hz;               // Control loop rate
    uint32_t max_duration_ms;               // Max session time
    bool auto_restart;                      // Auto-restart flag
} session_config_t;
```

**Size**: ~3.5 KB for 16 nodes (depends on architecture alignment)

### Per-Node Configuration

Each node receives the full configuration and extracts its own settings:

```c
typedef struct {
    uint8_t node_id;                        // Node ID
    node_personality_t personality;         // RESONATOR or SELF_OSCILLATOR

    // Modal parameters
    float omega[4];                         // Angular frequencies
    float gamma[4];                         // Damping coefficients
    float weight[4];                        // Audio weights

    // Network topology
    uint8_t neighbors[MAX_NEIGHBORS];       // Neighbor IDs
    uint8_t num_neighbors;                  // Number of neighbors
    float coupling_strength;                // Coupling coefficient

    // Audio synthesis
    float carrier_freq_hz;                  // Base carrier frequency
    float audio_gain;                       // Output gain
} node_config_t;
```

---

## Topology Generators

### 1. Ring Topology

**Use case**: Basic diffusive coupling, equal connectivity

```c
void topology_generate_ring(session_config_t* config, uint8_t num_nodes);
```

**Properties**:
- Each node connects to left and right neighbors
- Degree: 2
- Symmetric, periodic boundary conditions
- Example: `[0]←→[1]←→[2]←→...←→[N-1]←→[0]`

**Code snippet**:
```c
session_config_t config;
topology_generate_ring(&config, 8);  // 8-node ring

// Node i connects to:
// - (i-1) % N  (left neighbor)
// - (i+1) % N  (right neighbor)
```

### 2. Small-World Topology

**Use case**: Mix of local and long-range connections, Watts-Strogatz model

```c
void topology_generate_small_world(session_config_t* config,
                                   uint8_t num_nodes,
                                   float rewire_prob);
```

**Properties**:
- Starts with ring (degree 2)
- Adds long-range connections (degree 3)
- Good for synchronization studies
- Example: Ring + diagonal connections

**Code snippet**:
```c
session_config_t config;
topology_generate_small_world(&config, 8, 0.3);  // 8 nodes, 30% rewire

// Node i connects to:
// - (i-1) % N, (i+1) % N  (ring neighbors)
// - (i + N/2) % N         (long-range connection)
```

### 3. Cluster Topology

**Use case**: Community detection, modular networks

```c
void topology_generate_clusters(session_config_t* config,
                               uint8_t num_nodes,
                               uint8_t num_clusters);
```

**Properties**:
- Divides nodes into clusters
- Strong intra-cluster connections (ring within cluster)
- Weak inter-cluster bridges
- Example: Two 8-node rings connected by single edge

**Code snippet**:
```c
session_config_t config;
topology_generate_clusters(&config, 16, 2);  // 2 clusters of 8 nodes

// Within cluster: ring topology
// Between clusters: bridge from node 0 in each cluster
```

### 4. Hub-and-Spoke Topology

**Use case**: Centralized coordination, conductor model

```c
void topology_generate_hub_spoke(session_config_t* config,
                                uint8_t num_nodes,
                                uint8_t hub_id);
```

**Properties**:
- Hub node connects to all spokes
- Spokes only connect to hub
- Hub typically has SELF_OSCILLATOR personality
- Spokes have RESONATOR personality

**Code snippet**:
```c
session_config_t config;
topology_generate_hub_spoke(&config, 16, 0);  // Node 0 is hub

// Hub (node 0) connects to: 1, 2, 3, ..., 15
// Spoke (node i>0) connects to: 0
```

---

## Configuration Distribution Protocol

### Message Flow

```
Hub                                    Nodes
  |                                      |
  |--- OFFER (session info) ----------->|
  |<-- JOIN (accept) --------------------|
  |                                      |
  |--- CFG_BEGIN (size, chunks, CRC) -->|
  |--- CFG_CHUNK[0] (200 bytes) ------->|
  |--- CFG_CHUNK[1] (200 bytes) ------->|
  |--- ... ----------------------------->|
  |--- CFG_CHUNK[N-1] ------------------>|
  |--- CFG_END (final CRC) ------------->|
  |                                      |
  |<-- CFG_ACK (status=0: OK) -----------|
  |                                      |
  |--- START (begin session) ----------->|
```

### Message Types

#### MSG_OFFER
Sent by hub after discovery to announce configuration availability.

```c
typedef struct {
    message_header_t header;
    char session_id[32];      // Session identifier
    uint16_t config_size;     // Total size in bytes
    uint8_t num_nodes;        // Expected number of nodes
} msg_offer_t;
```

#### MSG_CFG_BEGIN
Start of configuration transfer.

```c
typedef struct {
    message_header_t header;
    uint16_t total_size;      // Total config size
    uint8_t num_chunks;       // Number of chunks
    uint32_t checksum;        // CRC32 of full config
} msg_cfg_begin_t;
```

#### MSG_CFG_CHUNK
Configuration data chunk (up to 200 bytes).

```c
typedef struct {
    message_header_t header;
    uint8_t chunk_idx;        // Chunk index (0-based)
    uint8_t chunk_size;       // Data size in this chunk
    uint8_t data[200];        // Chunk data
} msg_cfg_chunk_t;
```

#### MSG_CFG_END
End of configuration transfer.

```c
typedef struct {
    message_header_t header;
    uint32_t checksum;        // Final CRC32 (should match CFG_BEGIN)
} msg_cfg_end_t;
```

#### MSG_CFG_ACK
Node acknowledgment.

```c
typedef struct {
    message_header_t header;
    uint8_t status;           // 0=OK, 1=missing chunks, 2=checksum fail
} msg_cfg_ack_t;
```

---

## Node Configuration Reception

### Reception State Machine

```
IDLE --> CFG_BEGIN --> CFG_CHUNK(s) --> CFG_END --> VERIFY --> APPLY --> READY
                            ^                |
                            |                v
                            +--- (retry) ----+
```

### Chunk Reception

Nodes maintain a bitmap to track received chunks:

```c
static struct {
    uint8_t buffer[4096];         // Configuration buffer
    uint16_t total_size;          // Expected size
    uint8_t num_chunks;           // Expected chunks
    uint8_t chunks_received;      // Count
    uint32_t expected_checksum;   // CRC32
    uint8_t chunk_bitmap[32];     // 256-bit bitmap
    bool receiving;               // Currently receiving
} g_config_rx;
```

### Verification Steps

1. **Chunk completeness**: All chunks received?
2. **CRC32 checksum**: Matches expected value?
3. **Size validation**: Matches `sizeof(session_config_t)`?
4. **Content validation**: Node ID exists in config?

### Error Handling

| Error | Status Code | Recovery |
|-------|-------------|----------|
| Missing chunks | 1 | Hub can retransmit |
| Checksum mismatch | 2 | Hub retransmits all |
| Invalid format | 3 | Manual intervention |

---

## API Reference

### Hub Controller

#### `hub_send_default_config()`
```c
bool hub_send_default_config(hub_controller_t* hub);
```
Generates and sends default ring topology configuration.

**Returns**: `true` if successful

**Example**:
```c
hub_controller_t hub;
hub_start_discovery(&hub, 5000);  // Wait 5 seconds for nodes
hub_send_default_config(&hub);    // Send ring topology
hub_start_session(&hub);          // Begin session
```

#### `hub_send_config()`
```c
bool hub_send_config(hub_controller_t* hub, const session_config_t* config);
```
Sends custom configuration to all registered nodes.

**Parameters**:
- `hub`: Hub controller instance
- `config`: Configuration to distribute

**Returns**: `true` if successful

**Example**:
```c
session_manager_t mgr;
session_manager_init(&mgr, HUB_ID);
preset_small_world_8_oscillator(&mgr);  // Load preset
hub_send_config(&hub, &mgr.config);     // Distribute
```

### Session Manager (Nodes)

#### `session_manager_init()`
```c
void session_manager_init(session_manager_t* mgr, uint8_t my_node_id);
```
Initialize session manager.

#### `session_load_config_binary()`
```c
bool session_load_config_binary(session_manager_t* mgr,
                               const uint8_t* data,
                               size_t len);
```
Load configuration from binary blob (used by nodes after chunk assembly).

#### `session_get_my_config()`
```c
const node_config_t* session_get_my_config(const session_manager_t* mgr);
```
Extract this node's configuration from session config.

#### `session_apply_to_node()`
```c
bool session_apply_to_node(const session_manager_t* mgr, modal_node_t* node);
```
Apply configuration to modal node (sets frequencies, damping, personality).

---

## Examples

### Example 1: Hub with Default Ring

```c
void hub_task(void* param) {
    hub_controller_t hub;
    hub_init(&hub, HUB_NODE_ID, &network);

    // Discovery phase
    hub_start_discovery(&hub, 5000);
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Discovered %d nodes", hub_get_num_registered(&hub));

    // Send default configuration
    hub_send_default_config(&hub);  // Ring topology

    // Start session
    hub_start_session(&hub);

    // MIDI input loop...
}
```

### Example 2: Hub with Custom Topology

```c
void hub_task_custom(void* param) {
    hub_controller_t hub;
    hub_init(&hub, HUB_NODE_ID, &network);

    // Discovery
    hub_start_discovery(&hub, 5000);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Create custom configuration
    session_manager_t session;
    session_manager_init(&session, HUB_NODE_ID);

    // Generate small-world topology
    topology_generate_small_world(&session.config, hub_get_num_registered(&hub), 0.3f);

    // Customize mode parameters
    for (int i = 0; i < session.config.num_nodes; i++) {
        node_config_t* node = &session.config.nodes[i];
        node->omega[0] = 2 * M_PI * (440.0f + i * 10.0f);  // Frequency spread
        node->personality = (i == 0) ? PERSONALITY_SELF_OSCILLATOR : PERSONALITY_RESONATOR;
    }

    // Distribute
    hub_send_config(&hub, &session.config);
    hub_start_session(&hub);
}
```

### Example 3: Node Configuration Reception

```c
static void on_network_message_received(const network_message_t* msg) {
    switch (msg->header.type) {
        case MSG_CFG_BEGIN:
            // Reset reception state
            memset(&g_config_rx, 0, sizeof(g_config_rx));
            g_config_rx.total_size = msg->cfg_begin.total_size;
            g_config_rx.num_chunks = msg->cfg_begin.num_chunks;
            g_config_rx.expected_checksum = msg->cfg_begin.checksum;
            g_config_rx.receiving = true;
            ESP_LOGI(TAG, "Receiving config: %d bytes, %d chunks",
                     g_config_rx.total_size, g_config_rx.num_chunks);
            break;

        case MSG_CFG_CHUNK:
            // Assemble chunks (see main.c for full implementation)
            // ...
            break;

        case MSG_CFG_END:
            // Verify and apply
            if (session_load_config_binary(&g_session,
                                          g_config_rx.buffer,
                                          g_config_rx.total_size)) {
                session_apply_to_node(&g_session, &g_node);
                ESP_LOGI(TAG, "Configuration applied");
            }
            break;
    }
}
```

---

## Performance Characteristics

### Timing

| Operation | Duration | Notes |
|-----------|----------|-------|
| CFG_BEGIN send | ~5 ms | Single broadcast |
| Per-chunk send | ~25 ms | 20ms delay + transmission |
| Total config transfer (16 nodes) | ~500 ms | ~18 chunks @ 25ms each |
| CRC32 calculation | ~2 ms | 3.5 KB @ ~1.75 MB/s |
| Config application | <1 ms | Memory copy + apply |

### Network Load

- **Total bytes sent**: ~3.5 KB (full config)
- **Overhead**: ~15% (headers, checksums)
- **Messages**: ~20 (BEGIN + 18 chunks + END)
- **Broadcast**: All nodes receive simultaneously

### Memory Usage

| Component | Size | Location |
|-----------|------|----------|
| Hub config buffer | 4 KB | Stack (temporary) |
| Node RX buffer | 4 KB | Static (global) |
| Chunk bitmap | 32 bytes | Static (global) |
| Session config | 3.5 KB | Static (global) |

---

## Troubleshooting

### Configuration Not Received

**Symptoms**: Node doesn't ACK, stays in IDLE state

**Checks**:
1. Node registered during discovery? Check hub logs
2. ESP-NOW peer added? Check `esp_now_add_peer()` return
3. Buffer overflow? Check config size < 4096 bytes

**Fix**:
```bash
# Increase log verbosity
idf.py menuconfig → Component config → Log output → Debug
```

### Checksum Mismatch

**Symptoms**: `CFG_ACK` status=2

**Causes**:
- Chunk corruption in transmission
- Buffer alignment issues
- Endianness mismatch (cross-architecture)

**Fix**:
1. Verify ESP-NOW signal strength (RSSI > -70 dBm)
2. Reduce chunk send rate (increase delay in `hub_send_config()`)
3. Add per-chunk CRC (future enhancement)

### Missing Chunks

**Symptoms**: `CFG_ACK` status=1, chunk count < total

**Causes**:
- Node missed broadcast (in sleep mode?)
- Hub send too fast (ESP-NOW queue full)
- Node buffer overflow

**Fix**:
```c
// Increase inter-chunk delay in hub_send_config()
vTaskDelay(pdMS_TO_TICKS(50));  // Was 20ms
```

### Node Applies Wrong Configuration

**Symptoms**: Node plays wrong frequencies, connects to wrong neighbors

**Cause**: Node ID mismatch

**Fix**:
```c
// Verify node ID in session_get_my_config()
const node_config_t* config = session_get_my_config(&g_session);
if (!config) {
    ESP_LOGE(TAG, "My node ID (%d) not in config!", g_session.my_node_id);
}
```

---

## Future Enhancements

### JSON Configuration (Planned)

```c
bool session_load_config_json(session_manager_t* mgr, const char* json_str);
```

**Requires**:
- cJSON library integration
- JSON schema validation
- Larger buffers (~8 KB for readable JSON)

**Example JSON**:
```json
{
  "session_id": "ring_16",
  "topology": "ring",
  "num_nodes": 16,
  "global_coupling": 0.3,
  "nodes": [
    {
      "node_id": 0,
      "personality": "resonator",
      "omega": [2764.6, 2777.1, 5529.2, 345.6],
      "gamma": [0.5, 0.6, 1.0, 0.1],
      "neighbors": [15, 1]
    }
  ]
}
```

### Configuration Templates

Predefined templates stored in NVS:
- Small ensemble (4 nodes, ring)
- Medium ensemble (8 nodes, small-world)
- Large ensemble (16 nodes, clusters)

### Dynamic Reconfiguration

Hot-swap topology without session restart:
```c
MSG_CFG_UPDATE  // Partial config change
MSG_TOPOLOGY_CHANGE  // Neighbor list update only
```

---

## References

- `session_config.h` - API definitions
- `session_config.c` - Implementation
- `protocol.h` - Message format
- `hub_controller.c` - Distribution logic
- `main.c:191-302` - Node reception handling

---

**Phase 3 Complete**: Configuration distribution, topology generation, and chunked transfer protocol fully implemented and tested.
