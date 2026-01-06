/**
 * @file main.c
 * @brief Main entry point for ESP32 modal resonator node
 *
 * Architecture:
 * - Audio task: 48kHz synthesis (Core 1, high priority)
 * - Control task: 200-1000Hz modal integration (Core 0, medium priority)
 * - Network task: ESP-NOW event handling (Core 0, low priority)
 *
 * Task priorities:
 * - Audio: 10 (highest)
 * - Control: 5 (medium)
 * - Network: 3 (low)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "core/modal_node.h"
#include "audio/audio_synth.h"
#include "network/protocol.h"
#include "network/esp_now_manager.h"
#include "config/session_config.h"

// ============================================================================
// Constants
// ============================================================================

#define TAG "MODAL_NODE"

#define AUDIO_TASK_PRIORITY 10
#define CONTROL_TASK_PRIORITY 5
#define NETWORK_TASK_PRIORITY 3

#define AUDIO_TASK_STACK_SIZE 8192
#define CONTROL_TASK_STACK_SIZE 4096
#define NETWORK_TASK_STACK_SIZE 4096

// Pin to Core assignments
#define AUDIO_TASK_CORE 1
#define CONTROL_TASK_CORE 0
#define NETWORK_TASK_CORE 0

// ============================================================================
// Global State
// ============================================================================

static modal_node_t g_node;
static audio_synth_t g_audio;
static esp_now_manager_t g_network;
static session_manager_t g_session;

static QueueHandle_t g_poke_queue;  // Queue for poke events

// ============================================================================
// Configuration (TODO: load from NVS or external config)
// ============================================================================

#define MY_NODE_ID 0  // Change per node
#define DEFAULT_CARRIER_FREQ 440.0f

// ============================================================================
// Task Implementations
// ============================================================================

/**
 * @brief Control task: Modal integration at control rate
 *
 * This task runs the modal resonator dynamics at 200-1000 Hz.
 * It's independent of network and audio tasks.
 */
static void control_task(void* pvParameters) {
    ESP_LOGI(TAG, "Control task started on core %d", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000 / CONTROL_RATE_HZ);

    while (1) {
        // Check for poke events
        poke_event_t poke;
        if (xQueueReceive(g_poke_queue, &poke, 0) == pdTRUE) {
            modal_node_apply_poke(&g_node, &poke);
            ESP_LOGD(TAG, "Applied poke from node %d (strength=%.2f)",
                     poke.source_node_id, poke.strength);
        }

        // Simulate one timestep
        if (g_node.running) {
            modal_node_step(&g_node);
        }

        // Wait until next control period
        vTaskDelayUntil(&last_wake, period);
    }
}

/**
 * @brief Network callback: Handle received messages
 */
static void on_network_message_received(const network_message_t* msg) {
    ESP_LOGD(TAG, "Received message type 0x%02X from node %d",
             msg->header.type, msg->header.source_id);

    switch (msg->header.type) {
        case MSG_POKE: {
            // Forward poke to control task
            poke_event_t poke = {
                .source_node_id = msg->poke.header.source_id,
                .strength = msg->poke.strength,
                .phase_hint = msg->poke.phase_hint
            };
            memcpy(poke.mode_weights, msg->poke.mode_weights, sizeof(poke.mode_weights));

            if (xQueueSend(g_poke_queue, &poke, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Poke queue full, dropped event");
            }
            break;
        }

        case MSG_START:
            ESP_LOGI(TAG, "Session starting");
            session_start(&g_session);
            modal_node_start(&g_node);
            break;

        case MSG_STOP:
            ESP_LOGI(TAG, "Session stopping");
            session_stop(&g_session);
            modal_node_stop(&g_node);
            break;

        case MSG_RESET:
            ESP_LOGI(TAG, "Resetting node state");
            modal_node_reset(&g_node);
            break;

        case MSG_CFG_BEGIN:
        case MSG_CFG_CHUNK:
        case MSG_CFG_END:
            // TODO: Handle configuration messages
            ESP_LOGW(TAG, "Configuration messages not yet implemented");
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", msg->header.type);
            break;
    }
}

/**
 * @brief Network task: ESP-NOW message handling
 */
static void network_task(void* pvParameters) {
    ESP_LOGI(TAG, "Network task started on core %d", xPortGetCoreID());

    // Initialize ESP-NOW manager
    if (!esp_now_manager_init(&g_network, MY_NODE_ID)) {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW");
        vTaskDelete(NULL);
        return;
    }

    // Register message callback
    esp_now_register_message_callback(&g_network, on_network_message_received);

    // Start discovery
    esp_now_start_discovery(&g_network);
    ESP_LOGI(TAG, "ESP-NOW discovery started");

    // Heartbeat loop
    while (1) {
        // Send heartbeat every 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));

        network_message_t heartbeat;
        heartbeat.heartbeat.header.type = MSG_HEARTBEAT;
        heartbeat.heartbeat.header.source_id = MY_NODE_ID;
        heartbeat.heartbeat.header.dest_id = 0xFF; // Broadcast
        heartbeat.heartbeat.uptime_ms = esp_log_timestamp();
        heartbeat.heartbeat.cpu_usage = 0; // TODO: Calculate actual CPU usage

        esp_now_broadcast_message(&g_network, &heartbeat, sizeof(msg_heartbeat_t));

        // Check for stale peers
        esp_now_check_stale_peers(&g_network, 10000); // 10s timeout
    }
}

// ============================================================================
// Initialization
// ============================================================================

/**
 * @brief Initialize all subsystems
 */
static void system_init(void) {
    ESP_LOGI(TAG, "Initializing modal resonator node %d", MY_NODE_ID);

    // Initialize NVS (for WiFi/ESP-NOW)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create poke event queue
    g_poke_queue = xQueueCreate(16, sizeof(poke_event_t));
    if (g_poke_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create poke queue");
        abort();
    }

    // Initialize modal node
    modal_node_init(&g_node, MY_NODE_ID, PERSONALITY_RESONATOR);

    // Configure 4 modes (default preset)
    modal_node_set_mode(&g_node, 0, freq_to_omega(440.0f), 0.5f, 1.0f);  // Mode 0: carrier
    modal_node_set_mode(&g_node, 1, freq_to_omega(442.0f), 0.6f, 0.8f);  // Mode 1: detune
    modal_node_set_mode(&g_node, 2, freq_to_omega(880.0f), 1.0f, 0.3f);  // Mode 2: timbre
    modal_node_set_mode(&g_node, 3, freq_to_omega(55.0f), 0.1f, 0.5f);   // Mode 3: sub

    g_node.carrier_freq_hz = DEFAULT_CARRIER_FREQ;
    g_node.audio_gain = 0.7f;

    // Initialize audio synthesis
    audio_synth_init(&g_audio, &g_node, DEFAULT_CARRIER_FREQ);
    audio_i2s_init();

    // Initialize session manager
    session_manager_init(&g_session, MY_NODE_ID);

    // Load default preset (TODO: load from config)
    // preset_ring_16_resonator(&g_session);

    ESP_LOGI(TAG, "System initialization complete");
}

// ============================================================================
// Main Entry Point
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 Modal Resonator Node ===");
    ESP_LOGI(TAG, "Node ID: %d", MY_NODE_ID);
    ESP_LOGI(TAG, "Firmware Version: 1.0");

    // Initialize system
    system_init();

    // Start tasks
    ESP_LOGI(TAG, "Starting FreeRTOS tasks");

    // Audio task (Core 1, highest priority)
    xTaskCreatePinnedToCore(
        audio_task,
        "audio",
        AUDIO_TASK_STACK_SIZE,
        &g_audio,
        AUDIO_TASK_PRIORITY,
        NULL,
        AUDIO_TASK_CORE
    );

    // Control task (Core 0, medium priority)
    xTaskCreatePinnedToCore(
        control_task,
        "control",
        CONTROL_TASK_STACK_SIZE,
        NULL,
        CONTROL_TASK_PRIORITY,
        NULL,
        CONTROL_TASK_CORE
    );

    // Network task (Core 0, low priority)
    xTaskCreatePinnedToCore(
        network_task,
        "network",
        NETWORK_TASK_STACK_SIZE,
        NULL,
        NETWORK_TASK_PRIORITY,
        NULL,
        NETWORK_TASK_CORE
    );

    ESP_LOGI(TAG, "All tasks started successfully");
    ESP_LOGI(TAG, "Node ready for operation");
}
