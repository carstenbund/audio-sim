/**
 * @file protocol.h
 * @brief ESP-NOW message protocol for distributed modal network
 *
 * Design principles:
 * - Event-based, not state-sync
 * - Small packet sizes (<250 bytes)
 * - No continuous sync required
 * - Message-only control plane
 *
 * Message types:
 * - Discovery: HELLO, OFFER, JOIN
 * - Configuration: CFG_BEGIN, CFG_CHUNK, CFG_END, CFG_ACK
 * - Runtime: POKE, START, STOP
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define MAX_PACKET_SIZE 250        // ESP-NOW max payload
#define MAX_CONFIG_SIZE 2048       // Max configuration blob size
#define PROTOCOL_VERSION 1

// ============================================================================
// Message Types
// ============================================================================

typedef enum {
    // Discovery phase
    MSG_HELLO = 0x01,       ///< Node announces presence
    MSG_OFFER = 0x02,       ///< Controller offers configuration
    MSG_JOIN = 0x03,        ///< Node accepts configuration

    // Configuration phase
    MSG_CFG_BEGIN = 0x10,   ///< Start configuration transfer
    MSG_CFG_CHUNK = 0x11,   ///< Configuration data chunk
    MSG_CFG_END = 0x12,     ///< End configuration transfer
    MSG_CFG_ACK = 0x13,     ///< Configuration acknowledged
    MSG_CFG_NACK = 0x14,    ///< Configuration rejected

    // Session control
    MSG_START = 0x20,       ///< Start session
    MSG_STOP = 0x21,        ///< Stop session
    MSG_RESET = 0x22,       ///< Reset node state

    // Runtime events
    MSG_POKE = 0x30,        ///< Excitation event
    MSG_STATE = 0x31,       ///< State broadcast (optional)
    MSG_HEARTBEAT = 0x32,   ///< Keep-alive

    // Debug/monitoring
    MSG_DEBUG = 0xF0,       ///< Debug message
    MSG_ERROR = 0xF1        ///< Error report
} message_type_t;

// ============================================================================
// Message Structures
// ============================================================================

/**
 * @brief Message header (all messages)
 */
typedef struct __attribute__((packed)) {
    uint8_t version;        ///< Protocol version
    uint8_t type;           ///< Message type (message_type_t)
    uint8_t source_id;      ///< Sender node ID
    uint8_t dest_id;        ///< Destination (0xFF = broadcast)
    uint16_t sequence;      ///< Sequence number
    uint16_t timestamp_ms;  ///< Timestamp (ms since boot)
} message_header_t;

/**
 * @brief HELLO message (discovery)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint8_t mac_address[6]; ///< Node MAC address
    uint8_t capabilities;   ///< Capability flags
    char name[16];          ///< Human-readable name
} msg_hello_t;

/**
 * @brief OFFER message (controller → nodes)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    char session_id[32];    ///< Session identifier
    uint16_t config_size;   ///< Configuration size (bytes)
    uint8_t num_nodes;      ///< Expected number of nodes
} msg_offer_t;

/**
 * @brief JOIN message (node → controller)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint8_t requested_node_id; ///< Requested node ID (0xFF = auto)
    uint8_t mac_address[6];    ///< Node MAC address
} msg_join_t;

/**
 * @brief CFG_BEGIN message (start config transfer)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint16_t total_size;    ///< Total config size (bytes)
    uint8_t num_chunks;     ///< Number of chunks
    uint32_t checksum;      ///< CRC32 checksum
} msg_cfg_begin_t;

/**
 * @brief CFG_CHUNK message (config data)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint8_t chunk_idx;          ///< Chunk index
    uint8_t chunk_size;         ///< Data size in this chunk
    uint8_t data[200];          ///< Configuration data
} msg_cfg_chunk_t;

/**
 * @brief CFG_END message (finish config transfer)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint32_t checksum;      ///< Final checksum
} msg_cfg_end_t;

/**
 * @brief CFG_ACK message (acknowledgment)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint8_t status;         ///< 0 = OK, else error code
} msg_cfg_ack_t;

/**
 * @brief START message (begin session)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint32_t start_time_ms; ///< Synchronized start time
} msg_start_t;

/**
 * @brief STOP message (end session)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
} msg_stop_t;

/**
 * @brief POKE message (excitation event)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    float strength;             ///< Excitation strength [0,1]
    float phase_hint;           ///< Phase hint (radians, or -1)
    float mode_weights[4];      ///< Per-mode weights
} msg_poke_t;

/**
 * @brief STATE message (optional state broadcast)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    float mode0_real;       ///< Re(a_0)
    float mode0_imag;       ///< Im(a_0)
    float amplitude;        ///< |a_0|
} msg_state_t;

/**
 * @brief HEARTBEAT message (keep-alive)
 */
typedef struct __attribute__((packed)) {
    message_header_t header;
    uint32_t uptime_ms;     ///< Node uptime
    uint8_t cpu_usage;      ///< CPU usage %
} msg_heartbeat_t;

/**
 * @brief Generic message union
 */
typedef union {
    message_header_t header;
    msg_hello_t hello;
    msg_offer_t offer;
    msg_join_t join;
    msg_cfg_begin_t cfg_begin;
    msg_cfg_chunk_t cfg_chunk;
    msg_cfg_end_t cfg_end;
    msg_cfg_ack_t cfg_ack;
    msg_start_t start;
    msg_stop_t stop;
    msg_poke_t poke;
    msg_state_t state;
    msg_heartbeat_t heartbeat;
    uint8_t raw[MAX_PACKET_SIZE];
} network_message_t;

// ============================================================================
// Protocol API
// ============================================================================

/**
 * @brief Initialize message header
 *
 * @param header Pointer to header
 * @param type Message type
 * @param source_id Source node ID
 * @param dest_id Destination node ID (0xFF = broadcast)
 */
void protocol_init_header(message_header_t* header,
                         message_type_t type,
                         uint8_t source_id,
                         uint8_t dest_id);

/**
 * @brief Create HELLO message
 *
 * @param msg Pointer to message buffer
 * @param node_id Node ID
 * @param name Node name
 * @return Message size (bytes)
 */
size_t protocol_create_hello(network_message_t* msg,
                             uint8_t node_id,
                             const char* name);

/**
 * @brief Create POKE message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param dest_id Destination node ID
 * @param strength Excitation strength
 * @param phase_hint Phase hint (or -1)
 * @param mode_weights Mode weights array
 * @return Message size (bytes)
 */
size_t protocol_create_poke(network_message_t* msg,
                           uint8_t source_id,
                           uint8_t dest_id,
                           float strength,
                           float phase_hint,
                           const float* mode_weights);

/**
 * @brief Create START message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param start_time_ms Start time in ms
 * @return Message size (bytes)
 */
size_t protocol_create_start(network_message_t* msg,
                             uint8_t source_id,
                             uint32_t start_time_ms);

/**
 * @brief Create STOP message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @return Message size (bytes)
 */
size_t protocol_create_stop(network_message_t* msg, uint8_t source_id);

/**
 * @brief Create HEARTBEAT message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param uptime_ms Uptime in milliseconds
 * @param cpu_usage CPU usage percentage
 * @return Message size (bytes)
 */
size_t protocol_create_heartbeat(network_message_t* msg,
                                 uint8_t source_id,
                                 uint32_t uptime_ms,
                                 uint8_t cpu_usage);

/**
 * @brief Create OFFER message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param session_id Session identifier
 * @param config_size Configuration size in bytes
 * @param num_nodes Expected number of nodes
 * @return Message size (bytes)
 */
size_t protocol_create_offer(network_message_t* msg,
                             uint8_t source_id,
                             const char* session_id,
                             uint16_t config_size,
                             uint8_t num_nodes);

/**
 * @brief Create JOIN message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param requested_node_id Requested node ID (0xFF = auto)
 * @param mac_address Node MAC address
 * @return Message size (bytes)
 */
size_t protocol_create_join(network_message_t* msg,
                            uint8_t source_id,
                            uint8_t requested_node_id,
                            const uint8_t* mac_address);

/**
 * @brief Create CFG_BEGIN message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param total_size Total configuration size
 * @param num_chunks Number of chunks
 * @param checksum CRC32 checksum
 * @return Message size (bytes)
 */
size_t protocol_create_cfg_begin(network_message_t* msg,
                                 uint8_t source_id,
                                 uint16_t total_size,
                                 uint8_t num_chunks,
                                 uint32_t checksum);

/**
 * @brief Create CFG_CHUNK message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param chunk_idx Chunk index
 * @param data Chunk data
 * @param data_size Data size
 * @return Message size (bytes)
 */
size_t protocol_create_cfg_chunk(network_message_t* msg,
                                 uint8_t source_id,
                                 uint8_t chunk_idx,
                                 const uint8_t* data,
                                 uint8_t data_size);

/**
 * @brief Create CFG_END message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param checksum CRC32 checksum
 * @return Message size (bytes)
 */
size_t protocol_create_cfg_end(network_message_t* msg,
                               uint8_t source_id,
                               uint32_t checksum);

/**
 * @brief Create CFG_ACK message
 *
 * @param msg Pointer to message buffer
 * @param source_id Source node ID
 * @param dest_id Destination node ID
 * @param status Status code (0 = OK)
 * @return Message size (bytes)
 */
size_t protocol_create_cfg_ack(network_message_t* msg,
                               uint8_t source_id,
                               uint8_t dest_id,
                               uint8_t status);

/**
 * @brief Parse received message
 *
 * @param data Raw message data
 * @param len Data length
 * @param msg Output parsed message
 * @return true if valid, false otherwise
 */
bool protocol_parse_message(const uint8_t* data,
                           size_t len,
                           network_message_t* msg);

/**
 * @brief Validate message checksum
 *
 * @param msg Message to validate
 * @return true if valid, false otherwise
 */
bool protocol_validate_checksum(const network_message_t* msg);

/**
 * @brief Calculate CRC32 checksum
 *
 * @param data Data buffer
 * @param len Data length
 * @return CRC32 checksum
 */
uint32_t protocol_crc32(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_H
