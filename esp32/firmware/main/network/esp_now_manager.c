/**
 * @file esp_now_manager.c
 * @brief ESP-NOW mesh networking manager (stub implementation)
 *
 * TODO: Full implementation in Phase 2
 * This stub provides basic initialization and placeholder functions
 */

#include "esp_now_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include <string.h>

#define TAG "ESP_NOW_MGR"

// ============================================================================
// Static Callbacks
// ============================================================================

static esp_now_manager_t* g_manager = NULL;

static void esp_now_recv_cb(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (!g_manager || !g_manager->on_message_received) return;

    // Parse message
    network_message_t msg;
    if (protocol_parse_message(data, len, &msg)) {
        g_manager->on_message_received(&msg);
    }
}

static void esp_now_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
    // TODO: Track send status for statistics
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
}

// ============================================================================
// Send Functions
// ============================================================================

bool esp_now_send_message(esp_now_manager_t* mgr,
                         uint8_t dest_id,
                         const network_message_t* msg,
                         size_t len) {
    if (!mgr || !mgr->initialized) return false;

    // Find peer MAC by node ID
    uint8_t dest_mac[6];
    if (dest_id == 0xFF) {
        // Broadcast
        memset(dest_mac, 0xFF, 6);
    } else {
        // Find peer
        bool found = false;
        for (int i = 0; i < mgr->num_peers; i++) {
            if (mgr->peers[i].node_id == dest_id && mgr->peers[i].active) {
                memcpy(dest_mac, mgr->peers[i].mac_address, 6);
                found = true;
                break;
            }
        }
        if (!found) {
            ESP_LOGW(TAG, "Peer %d not found", dest_id);
            return false;
        }
    }

    // Send via ESP-NOW
    esp_err_t err = esp_now_send(dest_mac, (const uint8_t*)msg, len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ESP-NOW send failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

uint8_t esp_now_broadcast_message(esp_now_manager_t* mgr,
                                 const network_message_t* msg,
                                 size_t len) {
    return esp_now_send_message(mgr, 0xFF, msg, len) ? 1 : 0;
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
// Peer Management (Stubs)
// ============================================================================

bool esp_now_add_peer(esp_now_manager_t* mgr,
                     uint8_t node_id,
                     const uint8_t* mac) {
    // TODO: Implement in Phase 2
    return true;
}

bool esp_now_remove_peer(esp_now_manager_t* mgr, uint8_t node_id) {
    // TODO: Implement in Phase 2
    return true;
}

const peer_info_t* esp_now_get_peer(const esp_now_manager_t* mgr,
                                   uint8_t node_id) {
    // TODO: Implement in Phase 2
    return NULL;
}

// ============================================================================
// Discovery (Stubs)
// ============================================================================

void esp_now_start_discovery(esp_now_manager_t* mgr) {
    ESP_LOGI(TAG, "Discovery started (stub)");
    // TODO: Implement in Phase 2
}

void esp_now_stop_discovery(esp_now_manager_t* mgr) {
    ESP_LOGI(TAG, "Discovery stopped (stub)");
    // TODO: Implement in Phase 2
}

// ============================================================================
// Statistics (Stubs)
// ============================================================================

void esp_now_get_stats(const esp_now_manager_t* mgr,
                      uint32_t* total_tx,
                      uint32_t* total_rx,
                      uint32_t* total_lost) {
    if (total_tx) *total_tx = 0;
    if (total_rx) *total_rx = 0;
    if (total_lost) *total_lost = 0;
}

float esp_now_get_avg_latency(const esp_now_manager_t* mgr) {
    return 0.0f;
}

uint8_t esp_now_check_stale_peers(esp_now_manager_t* mgr, uint32_t timeout_ms) {
    return 0;
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
