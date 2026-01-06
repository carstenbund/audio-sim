/**
 * @file hub_main.c
 * @brief Hub/Controller node main entry point
 *
 * To build hub firmware instead of regular node firmware:
 *   idf.py menuconfig → Component config → Modal Node → Enable Hub Mode
 *
 * Or compile with: -DCONFIG_MODAL_HUB_MODE=1
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "network/protocol.h"
#include "network/esp_now_manager.h"
#include "config/hub_controller.h"

// ============================================================================
// Constants
// ============================================================================

#define TAG "HUB_MAIN"

#define HUB_NODE_ID 0

#define MIDI_TASK_PRIORITY 4
#define DISCOVERY_TASK_PRIORITY 3
#define HEARTBEAT_TASK_PRIORITY 2

#define MIDI_TASK_STACK_SIZE 4096
#define DISCOVERY_TASK_STACK_SIZE 4096
#define HEARTBEAT_TASK_STACK_SIZE 2048

// Pin to Core assignments
#define MIDI_TASK_CORE 0
#define DISCOVERY_TASK_CORE 0
#define HEARTBEAT_TASK_CORE 0

// ============================================================================
// Global State
// ============================================================================

static hub_controller_t g_hub;
static esp_now_manager_t g_network;

// ============================================================================
// Network Message Handler
// ============================================================================

static void on_network_message_received(const network_message_t* msg) {
    ESP_LOGD(TAG, "Received message type 0x%02X from node %d",
             msg->header.type, msg->header.source_id);

    switch (msg->header.type) {
        case MSG_HELLO:
            hub_handle_hello(&g_hub, &msg->hello);
            break;

        case MSG_HEARTBEAT:
            // Update last heartbeat time for node
            for (int i = 0; i < g_hub.num_registered; i++) {
                if (g_hub.nodes[i].node_id == msg->header.source_id) {
                    g_hub.nodes[i].last_heartbeat_ms = msg->heartbeat.uptime_ms;
                    break;
                }
            }
            break;

        case MSG_CFG_ACK:
            ESP_LOGI(TAG, "Node %d acknowledged configuration", msg->header.source_id);
            break;

        case MSG_CFG_NACK:
            ESP_LOGW(TAG, "Node %d rejected configuration", msg->header.source_id);
            break;

        default:
            ESP_LOGD(TAG, "Unhandled message type: 0x%02X", msg->header.type);
            break;
    }
}

// ============================================================================
// MIDI Task
// ============================================================================

static void midi_task(void* pvParameters) {
    ESP_LOGI(TAG, "MIDI task started on core %d", xPortGetCoreID());

    // Wait for network to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize MIDI
    if (!hub_midi_init(&g_hub)) {
        ESP_LOGE(TAG, "Failed to initialize MIDI");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "MIDI input ready");
    ESP_LOGI(TAG, "  Channel 1: Trigger notes (short pokes)");
    ESP_LOGI(TAG, "  Channel 2: Drive notes (sustained pokes)");

    TickType_t last_drive_time = xTaskGetTickCount();

    while (1) {
        // Process MIDI input
        hub_midi_process(&g_hub);

        // Process drive notes (channel 2) every 100ms
        TickType_t now = xTaskGetTickCount();
        if ((now - last_drive_time) >= pdMS_TO_TICKS(100)) {
            hub_process_drive_notes(&g_hub);
            last_drive_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(1));  // 1ms MIDI polling
    }
}

// ============================================================================
// Discovery & Configuration Task
// ============================================================================

static void discovery_task(void* pvParameters) {
    ESP_LOGI(TAG, "Discovery task started on core %d", xPortGetCoreID());

    // Wait for network to initialize
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Starting node discovery...");

    // Start discovery (5 second timeout)
    hub_start_discovery(&g_hub, 5000);

    // Wait for nodes to respond
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "Discovery complete: %d nodes found", hub_get_num_registered(&g_hub));

    // Send configuration (use defaults if specified)
    if (g_hub.use_default_config) {
        hub_send_default_config(&g_hub);
    } else {
        // TODO: Load custom config
        ESP_LOGW(TAG, "Custom config not implemented, using defaults");
        hub_send_default_config(&g_hub);
    }

    // Wait a bit for config to propagate
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Start session
    hub_start_session(&g_hub);

    ESP_LOGI(TAG, "Session started - ready for MIDI input");

    // Print status
    hub_print_status(&g_hub);

    // Task complete
    vTaskDelete(NULL);
}

// ============================================================================
// Heartbeat Task
// ============================================================================

static void heartbeat_task(void* pvParameters) {
    ESP_LOGI(TAG, "Heartbeat task started on core %d", xPortGetCoreID());

    while (1) {
        // Send heartbeat every 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));

        network_message_t heartbeat;
        uint32_t uptime_ms = esp_log_timestamp();
        protocol_create_heartbeat(&heartbeat, g_hub.hub_node_id, uptime_ms, 0);

        esp_now_broadcast_message(&g_network, &heartbeat, sizeof(msg_heartbeat_t));

        // Check for stale nodes (no heartbeat in 10s)
        for (int i = 0; i < g_hub.num_registered; i++) {
            if (g_hub.nodes[i].running) {
                uint32_t elapsed = uptime_ms - g_hub.nodes[i].last_heartbeat_ms;
                if (elapsed > 10000) {
                    ESP_LOGW(TAG, "Node %d heartbeat timeout (%u ms)",
                             g_hub.nodes[i].node_id, (unsigned)elapsed);
                }
            }
        }

        // Print status periodically
        if ((uptime_ms / 1000) % 30 == 0) {  // Every 30 seconds
            hub_print_status(&g_hub);
        }
    }
}

// ============================================================================
// Initialization
// ============================================================================

static void system_init(void) {
    ESP_LOGI(TAG, "Initializing hub controller node");

    // Initialize NVS (for WiFi/ESP-NOW)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize ESP-NOW
    if (!esp_now_manager_init(&g_network, HUB_NODE_ID)) {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW");
        abort();
    }

    // Register network callback
    esp_now_register_message_callback(&g_network, on_network_message_received);

    // Initialize hub controller
    hub_controller_init(&g_hub, HUB_NODE_ID, &g_network, true);  // Use defaults

    ESP_LOGI(TAG, "System initialization complete");
}

// ============================================================================
// Main Entry Point
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 Modal Network Hub ===");
    ESP_LOGI(TAG, "Hub Node ID: %d", HUB_NODE_ID);
    ESP_LOGI(TAG, "Firmware Version: 1.0-hub");

    // Initialize system
    system_init();

    // Start tasks
    ESP_LOGI(TAG, "Starting FreeRTOS tasks");

    // Discovery & configuration task (runs once then exits)
    xTaskCreatePinnedToCore(
        discovery_task,
        "discovery",
        DISCOVERY_TASK_STACK_SIZE,
        NULL,
        DISCOVERY_TASK_PRIORITY,
        NULL,
        DISCOVERY_TASK_CORE
    );

    // MIDI input task (runs continuously)
    xTaskCreatePinnedToCore(
        midi_task,
        "midi",
        MIDI_TASK_STACK_SIZE,
        NULL,
        MIDI_TASK_PRIORITY,
        NULL,
        MIDI_TASK_CORE
    );

    // Heartbeat task (runs continuously)
    xTaskCreatePinnedToCore(
        heartbeat_task,
        "heartbeat",
        HEARTBEAT_TASK_STACK_SIZE,
        NULL,
        HEARTBEAT_TASK_PRIORITY,
        NULL,
        HEARTBEAT_TASK_CORE
    );

    ESP_LOGI(TAG, "All tasks started successfully");
    ESP_LOGI(TAG, "Hub ready for operation");
}
