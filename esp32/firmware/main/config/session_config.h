/**
 * @file session_config.h
 * @brief Session configuration and management
 *
 * Configuration approach:
 * - Message-only control plane (no web server)
 * - Fixed topology per session
 * - JSON or compact binary format
 * - Distributed via ESP-NOW
 *
 * Session lifecycle:
 * 1. Discovery (HELLO/OFFER/JOIN)
 * 2. Configuration (CFG_BEGIN/CHUNK/END)
 * 3. Lock (START)
 * 4. Run (POKE events)
 * 5. Stop (STOP)
 */

#ifndef SESSION_CONFIG_H
#define SESSION_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "modal_node.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define MAX_SESSION_ID_LEN 32
#define MAX_TOPOLOGY_NAME_LEN 16
#define MAX_NODES_IN_SESSION 16

// ============================================================================
// Topology Types
// ============================================================================

typedef enum {
    TOPOLOGY_RING,              ///< Ring (degree 2)
    TOPOLOGY_SMALL_WORLD,       ///< Ring + sparse long links
    TOPOLOGY_CLUSTERS,          ///< Two clusters with bridge
    TOPOLOGY_HUB_SPOKE,         ///< Hub-and-spokes (conductor)
    TOPOLOGY_CUSTOM             ///< Custom adjacency matrix
} topology_type_t;

// ============================================================================
// Configuration Structures
// ============================================================================

/**
 * @brief Single node configuration
 */
typedef struct {
    uint8_t node_id;                    ///< Node ID
    node_personality_t personality;      ///< Resonator or self-oscillator

    // Mode parameters
    float omega[MAX_MODES];             ///< Angular frequencies
    float gamma[MAX_MODES];             ///< Damping coefficients
    float weight[MAX_MODES];            ///< Audio weights

    // Network
    uint8_t neighbors[MAX_NEIGHBORS];   ///< Neighbor IDs
    uint8_t num_neighbors;              ///< Number of neighbors
    float coupling_strength;            ///< Coupling coefficient

    // Audio
    float carrier_freq_hz;              ///< Base frequency
    float audio_gain;                   ///< Output gain
} node_config_t;

/**
 * @brief Session configuration (full network)
 */
typedef struct {
    char session_id[MAX_SESSION_ID_LEN]; ///< Unique session ID
    topology_type_t topology;            ///< Network topology
    uint8_t num_nodes;                   ///< Number of nodes

    node_config_t nodes[MAX_NODES_IN_SESSION]; ///< Per-node configs

    // Global parameters
    float global_coupling;              ///< Default coupling strength
    uint16_t control_rate_hz;           ///< Control loop rate

    // Session limits
    uint32_t max_duration_ms;           ///< Max session time (0 = unlimited)
    bool auto_restart;                  ///< Restart on completion
} session_config_t;

/**
 * @brief Session state machine
 */
typedef enum {
    SESSION_STATE_IDLE,         ///< Not configured
    SESSION_STATE_DISCOVERING,  ///< Waiting for nodes
    SESSION_STATE_CONFIGURING,  ///< Receiving config
    SESSION_STATE_READY,        ///< Configured, not running
    SESSION_STATE_RUNNING,      ///< Active session
    SESSION_STATE_ERROR         ///< Error state
} session_state_t;

/**
 * @brief Session manager
 */
typedef struct {
    session_config_t config;    ///< Current configuration
    session_state_t state;      ///< Current state
    uint8_t my_node_id;         ///< This node's ID
    uint32_t session_start_ms;  ///< Session start time
    bool is_controller;         ///< This node is controller
} session_manager_t;

// ============================================================================
// Configuration API
// ============================================================================

/**
 * @brief Initialize session manager
 *
 * @param mgr Pointer to session manager
 * @param my_node_id This node's ID (0xFF = auto-assign)
 */
void session_manager_init(session_manager_t* mgr, uint8_t my_node_id);

/**
 * @brief Load configuration from JSON string
 *
 * @param mgr Pointer to session manager
 * @param json_str JSON configuration string
 * @return true if successful, false on error
 */
bool session_load_config_json(session_manager_t* mgr, const char* json_str);

/**
 * @brief Load configuration from binary blob
 *
 * @param mgr Pointer to session manager
 * @param data Binary configuration data
 * @param len Data length
 * @return true if successful, false on error
 */
bool session_load_config_binary(session_manager_t* mgr,
                               const uint8_t* data,
                               size_t len);

/**
 * @brief Get configuration for this node
 *
 * @param mgr Pointer to session manager
 * @return Pointer to node configuration, or NULL if not found
 */
const node_config_t* session_get_my_config(const session_manager_t* mgr);

/**
 * @brief Apply configuration to modal node
 *
 * @param mgr Pointer to session manager
 * @param node Pointer to modal node
 * @return true if successful, false on error
 */
bool session_apply_to_node(const session_manager_t* mgr, modal_node_t* node);

/**
 * @brief Start session
 *
 * @param mgr Pointer to session manager
 * @return true if successful, false on error
 */
bool session_start(session_manager_t* mgr);

/**
 * @brief Stop session
 *
 * @param mgr Pointer to session manager
 */
void session_stop(session_manager_t* mgr);

/**
 * @brief Check if session is running
 *
 * @param mgr Pointer to session manager
 * @return true if running, false otherwise
 */
bool session_is_running(const session_manager_t* mgr);

/**
 * @brief Get elapsed session time
 *
 * @param mgr Pointer to session manager
 * @return Elapsed time in milliseconds
 */
uint32_t session_get_elapsed_ms(const session_manager_t* mgr);

// ============================================================================
// Topology Generators
// ============================================================================

/**
 * @brief Generate ring topology
 *
 * @param config Pointer to session config
 * @param num_nodes Number of nodes
 */
void topology_generate_ring(session_config_t* config, uint8_t num_nodes);

/**
 * @brief Generate small-world topology
 *
 * @param config Pointer to session config
 * @param num_nodes Number of nodes
 * @param rewire_prob Rewiring probability [0,1]
 */
void topology_generate_small_world(session_config_t* config,
                                   uint8_t num_nodes,
                                   float rewire_prob);

/**
 * @brief Generate cluster topology
 *
 * @param config Pointer to session config
 * @param num_nodes Number of nodes
 * @param num_clusters Number of clusters
 */
void topology_generate_clusters(session_config_t* config,
                               uint8_t num_nodes,
                               uint8_t num_clusters);

/**
 * @brief Generate hub-and-spoke topology
 *
 * @param config Pointer to session config
 * @param num_nodes Number of nodes
 * @param hub_id Hub node ID
 */
void topology_generate_hub_spoke(session_config_t* config,
                                uint8_t num_nodes,
                                uint8_t hub_id);

// ============================================================================
// Preset Configurations
// ============================================================================

/**
 * @brief Load preset: 16-node ring resonator
 *
 * @param mgr Pointer to session manager
 */
void preset_ring_16_resonator(session_manager_t* mgr);

/**
 * @brief Load preset: 8-node small-world oscillator
 *
 * @param mgr Pointer to session manager
 */
void preset_small_world_8_oscillator(session_manager_t* mgr);

/**
 * @brief Load preset: 16-node cluster network
 *
 * @param mgr Pointer to session manager
 */
void preset_clusters_16(session_manager_t* mgr);

#ifdef __cplusplus
}
#endif

#endif // SESSION_CONFIG_H
