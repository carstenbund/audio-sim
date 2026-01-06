/**
 * @file hub_controller.h
 * @brief Hub/Controller node for network coordination
 *
 * The hub node provides:
 * - MIDI input handling (Channel 1: triggers, Channel 2: drive)
 * - Node discovery and registration
 * - Configuration distribution
 * - MIDI-to-poke translation
 * - Default configuration if none provided
 *
 * Based on Python implementation in src/midi_input.py
 */

#ifndef HUB_CONTROLLER_H
#define HUB_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "modal_node.h"
#include "protocol.h"
#include "esp_now_manager.h"
#include "session_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define MAX_REGISTERED_NODES 16
#define MIDI_BAUD_RATE 31250

// MIDI channel assignments (1-indexed like Python)
#define MIDI_CHANNEL_TRIGGER 1  // Short poke events
#define MIDI_CHANNEL_DRIVE 2    // Sustained pokes

// ============================================================================
// MIDI Note Tracking
// ============================================================================

/**
 * @brief Active MIDI note
 */
typedef struct {
    uint8_t note;           ///< MIDI note number
    uint8_t velocity;       ///< Velocity (0-127)
    float freq_hz;          ///< Frequency in Hz
    uint8_t channel;        ///< MIDI channel (1-16)
    uint8_t target_node;    ///< Target network node
    bool active;            ///< Note is active
} midi_note_t;

/**
 * @brief MIDI input state
 */
typedef struct {
    midi_note_t active_notes[128];  ///< Active notes (indexed by MIDI note)
    uint8_t num_active;             ///< Number of active notes
    bool initialized;               ///< UART initialized
} midi_input_t;

// ============================================================================
// Node Registration
// ============================================================================

/**
 * @brief Registered node information
 */
typedef struct {
    uint8_t node_id;            ///< Node ID
    uint8_t mac_address[6];     ///< MAC address
    bool registered;            ///< Successfully registered
    bool configured;            ///< Configuration sent
    bool running;               ///< Session started
    uint32_t last_heartbeat_ms; ///< Last heartbeat timestamp
} registered_node_t;

/**
 * @brief Hub state machine
 */
typedef enum {
    HUB_STATE_IDLE,         ///< Not initialized
    HUB_STATE_DISCOVERING,  ///< Discovering nodes
    HUB_STATE_CONFIGURING,  ///< Sending configuration
    HUB_STATE_READY,        ///< Ready to start
    HUB_STATE_RUNNING,      ///< Session running
    HUB_STATE_ERROR         ///< Error state
} hub_state_t;

// ============================================================================
// Hub Controller
// ============================================================================

/**
 * @brief Hub controller state
 */
typedef struct {
    uint8_t hub_node_id;                        ///< This hub's node ID
    hub_state_t state;                          ///< Current state

    // Node registry
    registered_node_t nodes[MAX_REGISTERED_NODES];
    uint8_t num_registered;                     ///< Number of registered nodes

    // MIDI input
    midi_input_t midi;

    // Network
    esp_now_manager_t* network;

    // Session configuration
    session_manager_t session;
    bool use_default_config;                    ///< Use defaults if no config provided

    // Statistics
    uint32_t pokes_sent;
    uint32_t discovery_attempts;
} hub_controller_t;

// ============================================================================
// Initialization
// ============================================================================

/**
 * @brief Initialize hub controller
 *
 * @param hub Pointer to hub controller
 * @param hub_node_id Hub node ID (usually 0)
 * @param network Network manager (must be initialized)
 * @param use_defaults Use default configuration if none provided
 */
void hub_controller_init(hub_controller_t* hub,
                        uint8_t hub_node_id,
                        esp_now_manager_t* network,
                        bool use_defaults);

// ============================================================================
// MIDI Input
// ============================================================================

/**
 * @brief Initialize MIDI input (UART-based)
 *
 * Configures UART for MIDI:
 * - 31250 baud
 * - 8N1
 * - GPIO 16 (RX)
 * - 6N138 optocoupler isolation
 *
 * @param hub Pointer to hub controller
 * @return true if successful
 */
bool hub_midi_init(hub_controller_t* hub);

/**
 * @brief Process MIDI input (call from task loop)
 *
 * Reads UART buffer and processes MIDI messages.
 *
 * @param hub Pointer to hub controller
 */
void hub_midi_process(hub_controller_t* hub);

/**
 * @brief Handle MIDI note on
 *
 * @param hub Pointer to hub controller
 * @param note MIDI note number
 * @param velocity Velocity (0-127)
 * @param channel MIDI channel (1-16)
 */
void hub_midi_note_on(hub_controller_t* hub, uint8_t note, uint8_t velocity, uint8_t channel);

/**
 * @brief Handle MIDI note off
 *
 * @param hub Pointer to hub controller
 * @param note MIDI note number
 * @param channel MIDI channel (1-16)
 */
void hub_midi_note_off(hub_controller_t* hub, uint8_t note, uint8_t channel);

// ============================================================================
// Node Discovery & Registration
// ============================================================================

/**
 * @brief Start node discovery
 *
 * Broadcasts HELLO messages and waits for responses.
 *
 * @param hub Pointer to hub controller
 * @param timeout_ms Discovery timeout (milliseconds)
 */
void hub_start_discovery(hub_controller_t* hub, uint32_t timeout_ms);

/**
 * @brief Handle HELLO message from node
 *
 * @param hub Pointer to hub controller
 * @param msg HELLO message
 */
void hub_handle_hello(hub_controller_t* hub, const msg_hello_t* msg);

/**
 * @brief Register a node
 *
 * @param hub Pointer to hub controller
 * @param node_id Node ID to assign
 * @param mac MAC address
 * @return true if registered successfully
 */
bool hub_register_node(hub_controller_t* hub, uint8_t node_id, const uint8_t* mac);

/**
 * @brief Get number of registered nodes
 *
 * @param hub Pointer to hub controller
 * @return Number of registered nodes
 */
uint8_t hub_get_num_registered(const hub_controller_t* hub);

// ============================================================================
// Configuration Distribution
// ============================================================================

/**
 * @brief Send default configuration to all nodes
 *
 * Uses built-in presets (ring topology, 4-mode resonators).
 *
 * @param hub Pointer to hub controller
 * @return true if successful
 */
bool hub_send_default_config(hub_controller_t* hub);

/**
 * @brief Send custom configuration to all nodes
 *
 * @param hub Pointer to hub controller
 * @param config Session configuration
 * @return true if successful
 */
bool hub_send_config(hub_controller_t* hub, const session_config_t* config);

// ============================================================================
// Session Control
// ============================================================================

/**
 * @brief Start session on all nodes
 *
 * @param hub Pointer to hub controller
 * @return true if successful
 */
bool hub_start_session(hub_controller_t* hub);

/**
 * @brief Stop session on all nodes
 *
 * @param hub Pointer to hub controller
 */
void hub_stop_session(hub_controller_t* hub);

// ============================================================================
// MIDI → Poke Translation
// ============================================================================

/**
 * @brief Send poke to target node based on MIDI note
 *
 * Channel 1: Short trigger poke (10ms envelope)
 * Channel 2: Sustained poke (sent every 100ms while note held)
 *
 * @param hub Pointer to hub controller
 * @param note MIDI note
 */
void hub_send_midi_poke(hub_controller_t* hub, const midi_note_t* note);

/**
 * @brief Process active drive notes (channel 2)
 *
 * Sends sustained pokes for all active channel 2 notes.
 * Call periodically (e.g., every 100ms).
 *
 * @param hub Pointer to hub controller
 */
void hub_process_drive_notes(hub_controller_t* hub);

// ============================================================================
// Utility
// ============================================================================

/**
 * @brief Map MIDI note to target node (modulo mapping)
 *
 * @param note MIDI note number
 * @param num_nodes Number of nodes in network
 * @return Target node ID
 */
uint8_t hub_note_to_node(uint8_t note, uint8_t num_nodes);

/**
 * @brief Print hub status
 *
 * @param hub Pointer to hub controller
 */
void hub_print_status(const hub_controller_t* hub);

#ifdef __cplusplus
}
#endif

#endif // HUB_CONTROLLER_H
