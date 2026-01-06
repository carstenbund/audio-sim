/**
 * @file esp_now_manager.c
 * @brief ESP-NOW mesh networking manager (complete implementation)
 *
 * Full ESP-NOW implementation for distributed modal network:
 * - Peer discovery and management
 * - Message routing with retries
 * - Statistics tracking
 * - Configuration chunking support
 */

#include "esp_now_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <string.h>

#define TAG "ESP_NOW_MGR"

// ============================================================================
// Static State
// ============================================================================

static esp_now_manager_t* g_manager = NULL;

// Statistics
static uint32_t g_tx_count = 0;
static uint32_t g_rx_count = 0;
static uint32_t g_tx_fail_count = 0;

// ============================================================================
// Static Callbacks
// ============================================================================

static void esp_now_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    if (!g_manager) return;

    g_rx_count++;

    // Parse message
    network_message_t msg;
    if (!protocol_parse_message(data, len, &msg)) {
        ESP_LOGW(TAG, "Failed to parse message (%d bytes)", len);
        return;
    }

    // Update peer statistics
    for (int i = 0; i < g_manager->num_peers; i++) {
        if (mac_equal(g_manager->peers[i].mac_address, recv_info->src_addr)) {
            g_manager->peers[i].packets_received++;
            g_manager->peers[i].last_seen_ms = esp_timer_get_time() / 1000;
            break;
        }
    }

    // Call user callback
    if (g_manager->on_message_received) {
        g_manager->on_message_received(&msg);
    }

    ESP_LOGD(TAG, "RX: type=0x%02X from=%d len=%d",
             msg.header.type, msg.header.source_id, len);
}

static void esp_now_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        g_tx_count++;

        // Update peer statistics
        if (g_manager) {
            for (int i = 0; i < g_manager->num_peers; i++) {
                if (mac_equal(g_manager->peers[i].mac_address, mac_addr)) {
                    g_manager->peers[i].packets_sent++;
                    break;
                }
            }
        }
    } else {
        g_tx_fail_count++;
        ESP_LOGW(TAG, "TX failed to " MACSTR, MAC2STR(mac_addr));

        // Update peer lost packets
        if (g_manager) {
            for (int i = 0; i < g_manager->num_peers; i++) {
                if (mac_equal(g_manager->peers[i].mac_address, mac_addr)) {
                    g_manager->peers[i].packets_lost++;
                    break;
                }
            }
        }
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool esp_now_manager_init(esp_now_manager_t* mgr, uint8_t my_node_id) {
    if (!mgr) return false;

    memset(mgr, 0, sizeof(esp_now_manager_t));
    mgr->my_node_id = my_node_id;
    g_manager = mgr;

    ESP_LOGI(TAG, "Initializing ESP-NOW for node %d", my_node_id);

    // Initialize WiFi (required for ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi set mode failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(err));
        return false;
    }

    // Get MAC address
    esp_wifi_get_mac(WIFI_IF_STA, mgr->my_mac);
    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mgr->my_mac[0], mgr->my_mac[1], mgr->my_mac[2],
             mgr->my_mac[3], mgr->my_mac[4], mgr->my_mac[5]);

    // Initialize ESP-NOW
    err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(err));
        return false;
    }

    // Register callbacks
    esp_now_register_recv_cb(esp_now_recv_cb);
    esp_now_register_send_cb(esp_now_send_cb);

    // Add broadcast peer (for discovery)
    esp_now_peer_info_t broadcast_peer = {0};
    memset(broadcast_peer.peer_addr, 0xFF, 6);
    broadcast_peer.channel = 0;
    broadcast_peer.ifidx = WIFI_IF_STA;
    broadcast_peer.encrypt = false;

    err = esp_now_add_peer(&broadcast_peer);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGE(TAG, "Failed to add broadcast peer: %s", esp_err_to_name(err));
        return false;
    }

    mgr->initialized = true;
    ESP_LOGI(TAG, "ESP-NOW initialized successfully");

    return true;
}

void esp_now_manager_deinit(esp_now_manager_t* mgr) {
    if (!mgr || !mgr->initialized) return;

    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();

    mgr->initialized = false;
    g_manager = NULL;

    ESP_LOGI(TAG, "ESP-NOW deinitialized");
}

// ============================================================================
// Send Functions
// ============================================================================

bool esp_now_send_message(esp_now_manager_t* mgr,
                         uint8_t dest_id,
                         const network_message_t* msg,
                         size_t len) {
    if (!mgr || !mgr->initialized) return false;

    // Find destination MAC address
    uint8_t dest_mac[6];
    if (dest_id == 0xFF) {
        // Broadcast
        memset(dest_mac, 0xFF, 6);
    } else {
        // Find peer by node ID
        bool found = false;
        for (int i = 0; i < mgr->num_peers; i++) {
            if (mgr->peers[i].node_id == dest_id && mgr->peers[i].active) {
                memcpy(dest_mac, mgr->peers[i].mac_address, 6);
                found = true;
                break;
            }
        }

        if (!found) {
            ESP_LOGD(TAG, "Peer %d not found, using broadcast", dest_id);
            memset(dest_mac, 0xFF, 6);
        }
    }

    // Send with retries
    int retries = MAX_SEND_RETRIES;
    esp_err_t err;

    while (retries > 0) {
        err = esp_now_send(dest_mac, (const uint8_t*)msg, len);

        if (err == ESP_OK) {
            ESP_LOGD(TAG, "TX: type=0x%02X to=%d len=%zu",
                     msg->header.type, dest_id, len);
            return true;
        }

        ESP_LOGW(TAG, "TX failed (attempt %d/%d): %s",
                 MAX_SEND_RETRIES - retries + 1, MAX_SEND_RETRIES,
                 esp_err_to_name(err));

        retries--;
        if (retries > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));  // Wait before retry
        }
    }

    ESP_LOGE(TAG, "TX failed to node %d after %d retries", dest_id, MAX_SEND_RETRIES);
    return false;
}

uint8_t esp_now_broadcast_message(esp_now_manager_t* mgr,
                                 const network_message_t* msg,
                                 size_t len) {
    if (!esp_now_send_message(mgr, 0xFF, msg, len)) {
        return 0;
    }
    return 1;  // Broadcast counts as 1 send
}

// ============================================================================
// Callback Registration
// ============================================================================

void esp_now_register_message_callback(esp_now_manager_t* mgr,
                                      void (*callback)(const network_message_t*)) {
    if (mgr) {
        mgr->on_message_received = callback;
    }
}

void esp_now_register_discovery_callback(esp_now_manager_t* mgr,
                                        void (*callback)(uint8_t, const uint8_t*)) {
    if (mgr) {
        mgr->on_peer_discovered = callback;
    }
}

// ============================================================================
// Peer Management
// ============================================================================

bool esp_now_add_peer(esp_now_manager_t* mgr,
                     uint8_t node_id,
                     const uint8_t* mac) {
    if (!mgr || !mgr->initialized) return false;

    // Check if already exists
    for (int i = 0; i < mgr->num_peers; i++) {
        if (mgr->peers[i].node_id == node_id) {
            ESP_LOGD(TAG, "Peer %d already exists, updating MAC", node_id);
            memcpy(mgr->peers[i].mac_address, mac, 6);
            mgr->peers[i].active = true;
            return true;
        }
    }

    // Add new peer
    if (mgr->num_peers >= MAX_PEERS) {
        ESP_LOGE(TAG, "Cannot add peer %d: registry full", node_id);
        return false;
    }

    peer_info_t* peer = &mgr->peers[mgr->num_peers];
    memcpy(peer->mac_address, mac, 6);
    peer->node_id = node_id;
    peer->active = true;
    peer->last_seen_ms = esp_timer_get_time() / 1000;
    peer->packets_sent = 0;
    peer->packets_received = 0;
    peer->packets_lost = 0;
    peer->latency_ms = 0.0f;

    mgr->num_peers++;

    // Add to ESP-NOW peer list
    esp_now_peer_info_t esp_peer = {0};
    memcpy(esp_peer.peer_addr, mac, 6);
    esp_peer.channel = 0;  // Use current channel
    esp_peer.ifidx = WIFI_IF_STA;
    esp_peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&esp_peer);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGW(TAG, "Failed to add ESP-NOW peer: %s", esp_err_to_name(err));
        // Continue anyway - might work via broadcast
    }

    ESP_LOGI(TAG, "Added peer: node_id=%d mac=" MACSTR " (total: %d)",
             node_id, MAC2STR(mac), mgr->num_peers);

    // Call discovery callback
    if (mgr->on_peer_discovered) {
        mgr->on_peer_discovered(node_id, mac);
    }

    return true;
}

bool esp_now_remove_peer(esp_now_manager_t* mgr, uint8_t node_id) {
    if (!mgr) return false;

    for (int i = 0; i < mgr->num_peers; i++) {
        if (mgr->peers[i].node_id == node_id) {
            // Remove from ESP-NOW
            esp_now_del_peer(mgr->peers[i].mac_address);

            // Mark as inactive (don't shift array to preserve indices)
            mgr->peers[i].active = false;

            ESP_LOGI(TAG, "Removed peer: node_id=%d", node_id);
            return true;
        }
    }

    return false;
}

const peer_info_t* esp_now_get_peer(const esp_now_manager_t* mgr,
                                   uint8_t node_id) {
    if (!mgr) return NULL;

    for (int i = 0; i < mgr->num_peers; i++) {
        if (mgr->peers[i].node_id == node_id && mgr->peers[i].active) {
            return &mgr->peers[i];
        }
    }

    return NULL;
}

uint8_t esp_now_get_all_peers(const esp_now_manager_t* mgr,
                             peer_info_t* peers,
                             uint8_t max_peers) {
    if (!mgr || !peers) return 0;

    uint8_t count = 0;
    for (int i = 0; i < mgr->num_peers && count < max_peers; i++) {
        if (mgr->peers[i].active) {
            memcpy(&peers[count], &mgr->peers[i], sizeof(peer_info_t));
            count++;
        }
    }

    return count;
}

bool esp_now_is_peer_active(const esp_now_manager_t* mgr, uint8_t node_id) {
    return esp_now_get_peer(mgr, node_id) != NULL;
}

// ============================================================================
// Discovery
// ============================================================================

void esp_now_start_discovery(esp_now_manager_t* mgr) {
    if (!mgr || !mgr->initialized) return;

    ESP_LOGI(TAG, "Discovery started");

    // Discovery is handled by application layer (hub sends HELLO broadcast)
    // This function is just a marker for clarity
}

void esp_now_stop_discovery(esp_now_manager_t* mgr) {
    if (!mgr) return;

    ESP_LOGI(TAG, "Discovery stopped");
}

void esp_now_handle_hello(esp_now_manager_t* mgr, const msg_hello_t* msg) {
    if (!mgr) return;

    // Extract node info from HELLO
    uint8_t node_id = msg->header.source_id;
    const uint8_t* mac = msg->mac_address;

    ESP_LOGI(TAG, "HELLO from node %d (%s)", node_id, msg->name);

    // Add as peer
    esp_now_add_peer(mgr, node_id, mac);
}

// ============================================================================
// Statistics & Monitoring
// ============================================================================

void esp_now_get_stats(const esp_now_manager_t* mgr,
                      uint32_t* total_tx,
                      uint32_t* total_rx,
                      uint32_t* total_lost) {
    if (total_tx) *total_tx = g_tx_count;
    if (total_rx) *total_rx = g_rx_count;
    if (total_lost) *total_lost = g_tx_fail_count;
}

float esp_now_get_avg_latency(const esp_now_manager_t* mgr) {
    if (!mgr || mgr->num_peers == 0) return 0.0f;

    float sum = 0.0f;
    int count = 0;

    for (int i = 0; i < mgr->num_peers; i++) {
        if (mgr->peers[i].active && mgr->peers[i].latency_ms > 0.0f) {
            sum += mgr->peers[i].latency_ms;
            count++;
        }
    }

    return (count > 0) ? (sum / count) : 0.0f;
}

uint8_t esp_now_check_stale_peers(esp_now_manager_t* mgr, uint32_t timeout_ms) {
    if (!mgr) return 0;

    uint32_t now = esp_timer_get_time() / 1000;
    uint8_t stale_count = 0;

    for (int i = 0; i < mgr->num_peers; i++) {
        if (mgr->peers[i].active) {
            uint32_t elapsed = now - mgr->peers[i].last_seen_ms;
            if (elapsed > timeout_ms) {
                ESP_LOGW(TAG, "Peer %d is stale (%u ms since last seen)",
                         mgr->peers[i].node_id, (unsigned)elapsed);
                stale_count++;

                // Optionally mark as inactive
                // mgr->peers[i].active = false;
            }
        }
    }

    return stale_count;
}

// ============================================================================
// Utilities
// ============================================================================

void mac_to_string(const uint8_t* mac, char* str) {
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool mac_equal(const uint8_t* mac1, const uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

void esp_now_get_my_mac(uint8_t* mac) {
    esp_wifi_get_mac(WIFI_IF_STA, mac);
}

// ============================================================================
// Debug
// ============================================================================

void esp_now_print_stats(const esp_now_manager_t* mgr) {
    ESP_LOGI(TAG, "=== ESP-NOW Statistics ===");
    ESP_LOGI(TAG, "TX: %u packets", (unsigned)g_tx_count);
    ESP_LOGI(TAG, "RX: %u packets", (unsigned)g_rx_count);
    ESP_LOGI(TAG, "TX failed: %u packets", (unsigned)g_tx_fail_count);
    ESP_LOGI(TAG, "Peers: %d active", mgr ? mgr->num_peers : 0);

    if (mgr) {
        for (int i = 0; i < mgr->num_peers; i++) {
            if (mgr->peers[i].active) {
                ESP_LOGI(TAG, "  Peer %d: TX=%u RX=%u lost=%u",
                         mgr->peers[i].node_id,
                         (unsigned)mgr->peers[i].packets_sent,
                         (unsigned)mgr->peers[i].packets_received,
                         (unsigned)mgr->peers[i].packets_lost);
            }
        }
    }
}
