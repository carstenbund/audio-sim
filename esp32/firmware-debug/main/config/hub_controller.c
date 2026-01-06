/**
 * @file hub_controller.c
 * @brief Hub/Controller node implementation
 *
 * Based on Python implementation:
 * - src/midi_input.py → MIDI handling
 * - experiments/audio_sonification.py → MIDI-to-poke translation
 */

#include "hub_controller.h"
#include "session_config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

#define TAG "HUB"

// ============================================================================
// MIDI UART Configuration
// ============================================================================

#define MIDI_UART_NUM UART_NUM_1
#define MIDI_RX_PIN 16
#define MIDI_BUF_SIZE 256

// ============================================================================
// Initialization
// ============================================================================

void hub_controller_init(hub_controller_t* hub,
                        uint8_t hub_node_id,
                        esp_now_manager_t* network,
                        bool use_defaults) {
    memset(hub, 0, sizeof(hub_controller_t));

    hub->hub_node_id = hub_node_id;
    hub->network = network;
    hub->use_default_config = use_defaults;
    hub->state = HUB_STATE_IDLE;

    // Initialize session manager
    session_manager_init(&hub->session, hub_node_id);
    hub->session.is_controller = true;

    ESP_LOGI(TAG, "Hub controller initialized (node_id=%d, use_defaults=%d)",
             hub_node_id, use_defaults);
}

// ============================================================================
// MIDI Input
// ============================================================================

bool hub_midi_init(hub_controller_t* hub) {
    ESP_LOGI(TAG, "Initializing MIDI input (UART%d, RX=GPIO%d)", MIDI_UART_NUM, MIDI_RX_PIN);

    uart_config_t uart_config = {
        .baud_rate = MIDI_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver
    esp_err_t err = uart_driver_install(MIDI_UART_NUM, MIDI_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_param_config(MIDI_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_set_pin(MIDI_UART_NUM, UART_PIN_NO_CHANGE, MIDI_RX_PIN,
                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        return false;
    }

    hub->midi.initialized = true;
    ESP_LOGI(TAG, "MIDI input initialized at 31250 baud");

    return true;
}

void hub_midi_process(hub_controller_t* hub) {
    if (!hub->midi.initialized) return;

    uint8_t data[3];
    int len = uart_read_bytes(MIDI_UART_NUM, data, sizeof(data), 0);

    if (len < 3) return;  // MIDI messages are 3 bytes

    // Parse MIDI message
    uint8_t status = data[0];
    uint8_t cmd = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1;  // Convert to 1-indexed

    if (cmd == 0x90) {  // Note On
        uint8_t note = data[1];
        uint8_t velocity = data[2];

        if (velocity > 0) {
            hub_midi_note_on(hub, note, velocity, channel);
        } else {
            hub_midi_note_off(hub, note, channel);
        }
    } else if (cmd == 0x80) {  // Note Off
        uint8_t note = data[1];
        hub_midi_note_off(hub, note, channel);
    }
}

void hub_midi_note_on(hub_controller_t* hub, uint8_t note, uint8_t velocity, uint8_t channel) {
    // Create MIDI note
    midi_note_t* midi_note = &hub->midi.active_notes[note];
    midi_note->note = note;
    midi_note->velocity = velocity;
    midi_note->freq_hz = midi_to_freq(note);
    midi_note->channel = channel;
    midi_note->target_node = hub_note_to_node(note, hub->num_registered);
    midi_note->active = true;

    hub->midi.num_active++;

    ESP_LOGI(TAG, "[MIDI] Note ON: ch=%d note=%d vel=%d freq=%.1f Hz → node %d",
             channel, note, velocity, midi_note->freq_hz, midi_note->target_node);

    // Send immediate poke (both channels trigger immediately)
    hub_send_midi_poke(hub, midi_note);
}

void hub_midi_note_off(hub_controller_t* hub, uint8_t note, uint8_t channel) {
    if (!hub->midi.active_notes[note].active) return;

    hub->midi.active_notes[note].active = false;
    hub->midi.num_active--;

    ESP_LOGI(TAG, "[MIDI] Note OFF: ch=%d note=%d", channel, note);
}

// ============================================================================
// Node Discovery & Registration
// ============================================================================

void hub_start_discovery(hub_controller_t* hub, uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Starting node discovery (timeout=%u ms)", (unsigned)timeout_ms);

    hub->state = HUB_STATE_DISCOVERING;
    hub->discovery_attempts++;

    // Send HELLO broadcast
    network_message_t msg;
    protocol_create_hello(&msg, hub->hub_node_id, "Hub");
    esp_now_broadcast_message(hub->network, &msg, sizeof(msg_hello_t));

    // Discovery continues in background via hub_handle_hello()
}

void hub_handle_hello(hub_controller_t* hub, const msg_hello_t* msg) {
    ESP_LOGI(TAG, "Received HELLO from node %d (%s)",
             msg->header.source_id, msg->name);

    // Auto-assign node ID based on order received
    uint8_t node_id = hub->num_registered;

    if (hub_register_node(hub, node_id, msg->mac_address)) {
        // Send OFFER
        network_message_t offer;
        offer.offer.header.type = MSG_OFFER;
        offer.offer.header.source_id = hub->hub_node_id;
        offer.offer.header.dest_id = node_id;
        strncpy(offer.offer.session_id, "default_session", sizeof(offer.offer.session_id));
        offer.offer.config_size = 0;  // Will send in config phase
        offer.offer.num_nodes = hub->num_registered;

        esp_now_send_message(hub->network, node_id, &offer, sizeof(msg_offer_t));

        ESP_LOGI(TAG, "Sent OFFER to node %d", node_id);
    }
}

bool hub_register_node(hub_controller_t* hub, uint8_t node_id, const uint8_t* mac) {
    if (hub->num_registered >= MAX_REGISTERED_NODES) {
        ESP_LOGE(TAG, "Cannot register node %d: registry full", node_id);
        return false;
    }

    registered_node_t* node = &hub->nodes[hub->num_registered];
    node->node_id = node_id;
    memcpy(node->mac_address, mac, 6);
    node->registered = true;
    node->configured = false;
    node->running = false;
    node->last_heartbeat_ms = esp_timer_get_time() / 1000;

    hub->num_registered++;

    ESP_LOGI(TAG, "Registered node %d (total: %d)", node_id, hub->num_registered);

    return true;
}

uint8_t hub_get_num_registered(const hub_controller_t* hub) {
    return hub->num_registered;
}

// ============================================================================
// Configuration Distribution
// ============================================================================

bool hub_send_default_config(hub_controller_t* hub) {
    ESP_LOGI(TAG, "Sending default configuration to %d nodes", hub->num_registered);

    // Generate default ring topology
    session_manager_t session_mgr;
    session_manager_init(&session_mgr, hub->hub_node_id);

    // Use preset ring configuration
    preset_ring_16_resonator(&session_mgr);

    // Update to match actual number of registered nodes
    if (hub->num_registered < 16) {
        session_mgr.config.num_nodes = hub->num_registered;
        topology_generate_ring(&session_mgr.config, hub->num_registered);
    }

    // Send this configuration to all nodes
    return hub_send_config(hub, &session_mgr.config);
}

bool hub_send_config(hub_controller_t* hub, const session_config_t* config) {
    if (!hub || !config) return false;

    ESP_LOGI(TAG, "Sending configuration to %d nodes", hub->num_registered);

    // Serialize configuration to binary
    uint8_t config_buffer[4096];  // Max config size
    session_manager_t temp_mgr;
    memcpy(&temp_mgr.config, config, sizeof(session_config_t));

    size_t config_size = session_serialize_config_binary(&temp_mgr,
                                                         config_buffer,
                                                         sizeof(config_buffer));

    if (config_size == 0) {
        ESP_LOGE(TAG, "Failed to serialize configuration");
        return false;
    }

    ESP_LOGI(TAG, "Configuration size: %d bytes", config_size);

    // Calculate number of chunks (200 bytes per chunk)
    const uint8_t chunk_size = 200;
    uint8_t num_chunks = (config_size + chunk_size - 1) / chunk_size;

    ESP_LOGI(TAG, "Splitting into %d chunks", num_chunks);

    // Calculate checksum
    uint32_t checksum = protocol_crc32(config_buffer, config_size);

    // Send CFG_BEGIN to all nodes
    network_message_t msg;
    protocol_create_cfg_begin(&msg, hub->hub_node_id, config_size, num_chunks, checksum);
    esp_now_broadcast_message(&hub->network, &msg, sizeof(msg_cfg_begin_t));

    ESP_LOGI(TAG, "Sent CFG_BEGIN (size=%d, chunks=%d, crc=0x%08X)",
             config_size, num_chunks, checksum);

    // Small delay between messages
    vTaskDelay(pdMS_TO_TICKS(50));

    // Send chunks
    for (uint8_t i = 0; i < num_chunks; i++) {
        size_t offset = i * chunk_size;
        size_t this_chunk_size = (offset + chunk_size > config_size) ?
                                 (config_size - offset) : chunk_size;

        protocol_create_cfg_chunk(&msg, hub->hub_node_id, i,
                                 config_buffer + offset, this_chunk_size);

        size_t msg_size = sizeof(message_header_t) + 2 + this_chunk_size;
        esp_now_broadcast_message(&hub->network, &msg, msg_size);

        ESP_LOGD(TAG, "Sent chunk %d/%d (%d bytes)", i + 1, num_chunks, this_chunk_size);

        // Small delay to avoid overwhelming nodes
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Send CFG_END
    protocol_create_cfg_end(&msg, hub->hub_node_id, checksum);
    esp_now_broadcast_message(&hub->network, &msg, sizeof(msg_cfg_end_t));

    ESP_LOGI(TAG, "Sent CFG_END");

    // Wait for acknowledgments (simplified - just wait a bit)
    vTaskDelay(pdMS_TO_TICKS(200));

    // Mark all nodes as configured
    for (int i = 0; i < hub->num_registered; i++) {
        hub->nodes[i].configured = true;
    }

    hub->state = HUB_STATE_READY;
    ESP_LOGI(TAG, "Configuration distribution complete");

    return true;
}

// ============================================================================
// Session Control
// ============================================================================

bool hub_start_session(hub_controller_t* hub) {
    if (hub->state != HUB_STATE_READY) {
        ESP_LOGW(TAG, "Cannot start session: not ready (state=%d)", hub->state);
        return false;
    }

    ESP_LOGI(TAG, "Starting session on %d nodes", hub->num_registered);

    // Send START message to all nodes
    network_message_t msg;
    uint32_t start_time = esp_timer_get_time() / 1000;
    protocol_create_start(&msg, hub->hub_node_id, start_time);

    for (int i = 0; i < hub->num_registered; i++) {
        esp_now_send_message(hub->network, hub->nodes[i].node_id,
                           &msg, sizeof(msg_start_t));
        hub->nodes[i].running = true;
    }

    hub->state = HUB_STATE_RUNNING;
    ESP_LOGI(TAG, "Session started");

    return true;
}

void hub_stop_session(hub_controller_t* hub) {
    if (hub->state != HUB_STATE_RUNNING) return;

    ESP_LOGI(TAG, "Stopping session");

    // Send STOP message
    network_message_t msg;
    protocol_create_stop(&msg, hub->hub_node_id);

    for (int i = 0; i < hub->num_registered; i++) {
        esp_now_send_message(hub->network, hub->nodes[i].node_id,
                           &msg, sizeof(msg_stop_t));
        hub->nodes[i].running = false;
    }

    hub->state = HUB_STATE_READY;
    ESP_LOGI(TAG, "Session stopped");
}

// ============================================================================
// MIDI → Poke Translation
// ============================================================================

void hub_send_midi_poke(hub_controller_t* hub, const midi_note_t* note) {
    if (hub->state != HUB_STATE_RUNNING) {
        ESP_LOGD(TAG, "Skipping poke: session not running");
        return;
    }

    // Create poke message
    network_message_t msg;
    float strength = note->velocity / 127.0f;
    float phase_hint = -1.0f;  // Random phase

    // Mode weights (match Python's default mapping)
    float mode_weights[4] = {1.0f, 0.8f, 0.3f, 0.5f};

    protocol_create_poke(&msg, hub->hub_node_id, note->target_node,
                        strength, phase_hint, mode_weights);

    // Send poke
    bool sent = esp_now_send_message(hub->network, note->target_node,
                                     &msg, sizeof(msg_poke_t));

    if (sent) {
        hub->pokes_sent++;
        ESP_LOGD(TAG, "Sent poke to node %d (strength=%.2f)", note->target_node, strength);
    } else {
        ESP_LOGW(TAG, "Failed to send poke to node %d", note->target_node);
    }
}

void hub_process_drive_notes(hub_controller_t* hub) {
    // Send sustained pokes for all active channel 2 notes
    for (int i = 0; i < 128; i++) {
        midi_note_t* note = &hub->midi.active_notes[i];
        if (note->active && note->channel == MIDI_CHANNEL_DRIVE) {
            hub_send_midi_poke(hub, note);
        }
    }
}

// ============================================================================
// Utility
// ============================================================================

uint8_t hub_note_to_node(uint8_t note, uint8_t num_nodes) {
    if (num_nodes == 0) return 0;
    return note % num_nodes;
}

void hub_print_status(const hub_controller_t* hub) {
    ESP_LOGI(TAG, "=== Hub Status ===");
    ESP_LOGI(TAG, "State: %d", hub->state);
    ESP_LOGI(TAG, "Registered nodes: %d", hub->num_registered);
    ESP_LOGI(TAG, "Active MIDI notes: %d", hub->midi.num_active);
    ESP_LOGI(TAG, "Pokes sent: %u", (unsigned)hub->pokes_sent);
    ESP_LOGI(TAG, "Discovery attempts: %u", (unsigned)hub->discovery_attempts);

    for (int i = 0; i < hub->num_registered; i++) {
        registered_node_t* node = (registered_node_t*)&hub->nodes[i];
        ESP_LOGI(TAG, "  Node %d: registered=%d configured=%d running=%d",
                 node->node_id, node->registered, node->configured, node->running);
    }
}
