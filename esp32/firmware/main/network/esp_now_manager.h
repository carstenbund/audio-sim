/**
 * @file esp_now_manager.h
 * @brief ESP-NOW mesh networking for modal resonator network
 *
 * Features:
 * - Auto-discovery via broadcast
 * - Peer management (up to 20 peers)
 * - Message routing
 * - Latency monitoring
 * - Packet loss detection
 *
 * Design:
 * - Event-based, not continuous sync
 * - Small packets (<250 bytes)
 * - Fire-and-forget or ACK modes
 */

#ifndef ESP_NOW_MANAGER_H
#define ESP_NOW_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_now.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define MAX_PEERS 20               // ESP-NOW limit
#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MAX_SEND_RETRIES 3

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Peer information
 */
typedef struct {
    uint8_t mac_address[6];     ///< MAC address
    uint8_t node_id;            ///< Assigned node ID
    bool active;                ///< Peer is active
    uint32_t last_seen_ms;      ///< Last message timestamp
    uint32_t packets_sent;      ///< TX packet count
    uint32_t packets_received;  ///< RX packet count
    uint32_t packets_lost;      ///< Lost packet count
    float latency_ms;           ///< Estimated latency
} peer_info_t;

/**
 * @brief ESP-NOW manager state
 */
typedef struct {
    bool initialized;
    uint8_t my_node_id;
    uint8_t my_mac[6];

    peer_info_t peers[MAX_PEERS];
    uint8_t num_peers;

    uint16_t tx_sequence;       ///< TX sequence counter

    // Callbacks
    void (*on_message_received)(const network_message_t* msg);
    void (*on_peer_discovered)(uint8_t node_id, const uint8_t* mac);
    void (*on_peer_lost)(uint8_t node_id);
} esp_now_manager_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize ESP-NOW manager
 *
 * Sets up ESP-NOW, registers callbacks, and starts discovery.
 *
 * @param mgr Pointer to manager structure
 * @param my_node_id This node's ID
 * @return true if successful, false on error
 */
bool esp_now_manager_init(esp_now_manager_t* mgr, uint8_t my_node_id);

/**
 * @brief Deinitialize ESP-NOW manager
 *
 * @param mgr Pointer to manager structure
 */
void esp_now_manager_deinit(esp_now_manager_t* mgr);

/**
 * @brief Send message to specific peer
 *
 * @param mgr Pointer to manager structure
 * @param dest_id Destination node ID (0xFF = broadcast)
 * @param msg Message to send
 * @param len Message length
 * @return true if sent successfully, false on error
 */
bool esp_now_send_message(esp_now_manager_t* mgr,
                         uint8_t dest_id,
                         const network_message_t* msg,
                         size_t len);

/**
 * @brief Broadcast message to all peers
 *
 * @param mgr Pointer to manager structure
 * @param msg Message to send
 * @param len Message length
 * @return Number of successful sends
 */
uint8_t esp_now_broadcast_message(esp_now_manager_t* mgr,
                                 const network_message_t* msg,
                                 size_t len);

/**
 * @brief Register message callback
 *
 * @param mgr Pointer to manager structure
 * @param callback Callback function
 */
void esp_now_register_message_callback(esp_now_manager_t* mgr,
                                      void (*callback)(const network_message_t*));

/**
 * @brief Register peer discovery callback
 *
 * @param mgr Pointer to manager structure
 * @param callback Callback function
 */
void esp_now_register_discovery_callback(esp_now_manager_t* mgr,
                                        void (*callback)(uint8_t, const uint8_t*));

// ============================================================================
// Peer Management
// ============================================================================

/**
 * @brief Add peer by MAC address
 *
 * @param mgr Pointer to manager structure
 * @param node_id Node ID to assign
 * @param mac MAC address
 * @return true if added successfully, false on error
 */
bool esp_now_add_peer(esp_now_manager_t* mgr,
                     uint8_t node_id,
                     const uint8_t* mac);

/**
 * @brief Remove peer
 *
 * @param mgr Pointer to manager structure
 * @param node_id Node ID to remove
 * @return true if removed successfully, false on error
 */
bool esp_now_remove_peer(esp_now_manager_t* mgr, uint8_t node_id);

/**
 * @brief Get peer info by node ID
 *
 * @param mgr Pointer to manager structure
 * @param node_id Node ID
 * @return Pointer to peer info, or NULL if not found
 */
const peer_info_t* esp_now_get_peer(const esp_now_manager_t* mgr,
                                   uint8_t node_id);

/**
 * @brief Get all active peers
 *
 * @param mgr Pointer to manager structure
 * @param peers Output array of peer info
 * @param max_peers Maximum peers to return
 * @return Number of active peers
 */
uint8_t esp_now_get_all_peers(const esp_now_manager_t* mgr,
                             peer_info_t* peers,
                             uint8_t max_peers);

/**
 * @brief Check if peer is active
 *
 * @param mgr Pointer to manager structure
 * @param node_id Node ID
 * @return true if active, false otherwise
 */
bool esp_now_is_peer_active(const esp_now_manager_t* mgr, uint8_t node_id);

// ============================================================================
// Discovery
// ============================================================================

/**
 * @brief Start discovery (send HELLO broadcast)
 *
 * @param mgr Pointer to manager structure
 */
void esp_now_start_discovery(esp_now_manager_t* mgr);

/**
 * @brief Stop discovery
 *
 * @param mgr Pointer to manager structure
 */
void esp_now_stop_discovery(esp_now_manager_t* mgr);

/**
 * @brief Handle received HELLO message
 *
 * @param mgr Pointer to manager structure
 * @param msg HELLO message
 */
void esp_now_handle_hello(esp_now_manager_t* mgr, const msg_hello_t* msg);

// ============================================================================
// Statistics & Monitoring
// ============================================================================

/**
 * @brief Get network statistics
 *
 * @param mgr Pointer to manager structure
 * @param total_tx Output: Total packets sent
 * @param total_rx Output: Total packets received
 * @param total_lost Output: Total packets lost
 */
void esp_now_get_stats(const esp_now_manager_t* mgr,
                      uint32_t* total_tx,
                      uint32_t* total_rx,
                      uint32_t* total_lost);

/**
 * @brief Get average network latency
 *
 * @param mgr Pointer to manager structure
 * @return Average latency in milliseconds
 */
float esp_now_get_avg_latency(const esp_now_manager_t* mgr);

/**
 * @brief Check for stale peers (no recent activity)
 *
 * Marks peers as inactive if no message received within timeout.
 *
 * @param mgr Pointer to manager structure
 * @param timeout_ms Timeout in milliseconds
 * @return Number of stale peers found
 */
uint8_t esp_now_check_stale_peers(esp_now_manager_t* mgr, uint32_t timeout_ms);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Convert MAC address to string
 *
 * @param mac MAC address
 * @param str Output string buffer (min 18 bytes)
 */
void mac_to_string(const uint8_t* mac, char* str);

/**
 * @brief Compare MAC addresses
 *
 * @param mac1 First MAC address
 * @param mac2 Second MAC address
 * @return true if equal, false otherwise
 */
bool mac_equal(const uint8_t* mac1, const uint8_t* mac2);

/**
 * @brief Get my MAC address
 *
 * @param mac Output MAC address buffer (6 bytes)
 */
void esp_now_get_my_mac(uint8_t* mac);

#ifdef __cplusplus
}
#endif

#endif // ESP_NOW_MANAGER_H
