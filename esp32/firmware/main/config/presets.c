/**
 * @file presets.c
 * @brief Default configuration presets
 *
 * Provides built-in configurations for common network topologies.
 */

#include "session_config.h"
#include "modal_node.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Default Mode Parameters
// ============================================================================

/**
 * @brief Get default 4-mode configuration
 *
 * Mode mapping:
 * - Mode 0: 440 Hz carrier (A4)
 * - Mode 1: 442 Hz detuning (slight beating)
 * - Mode 2: 880 Hz brightness (octave up)
 * - Mode 3: 55 Hz sub-bass (2 octaves down)
 */
static void get_default_modes(float omega[4], float gamma[4], float weight[4]) {
    // Mode 0: Carrier (A4 = 440 Hz)
    omega[0] = 2.0f * M_PI * 440.0f;
    gamma[0] = 0.5f;
    weight[0] = 1.0f;

    // Mode 1: Detuned carrier (442 Hz, creates 2 Hz beating)
    omega[1] = 2.0f * M_PI * 442.0f;
    gamma[1] = 0.6f;
    weight[1] = 0.8f;

    // Mode 2: Brightness (octave up)
    omega[2] = 2.0f * M_PI * 880.0f;
    gamma[2] = 1.0f;
    weight[2] = 0.3f;

    // Mode 3: Sub-bass (2 octaves down)
    omega[3] = 2.0f * M_PI * 55.0f;
    gamma[3] = 0.1f;
    weight[3] = 0.5f;
}

// ============================================================================
// Ring Topology (16 nodes)
// ============================================================================

void preset_ring_16_resonator(session_manager_t* mgr) {
    session_config_t* config = &mgr->config;

    strncpy(config->session_id, "ring_16_resonator", MAX_SESSION_ID_LEN);
    config->topology = TOPOLOGY_RING;
    config->num_nodes = 16;
    config->global_coupling = 0.3f;
    config->control_rate_hz = 500;
    config->max_duration_ms = 0;  // Unlimited
    config->auto_restart = false;

    // Default modes
    float omega[4], gamma[4], weight[4];
    get_default_modes(omega, gamma, weight);

    // Configure each node
    for (uint8_t i = 0; i < config->num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        node->node_id = i;
        node->personality = PERSONALITY_RESONATOR;

        // Copy mode parameters
        memcpy(node->omega, omega, sizeof(omega));
        memcpy(node->gamma, gamma, sizeof(gamma));
        memcpy(node->weight, weight, sizeof(weight));

        // Ring topology: each node connects to left and right neighbors
        node->num_neighbors = 2;
        node->neighbors[0] = (i - 1 + config->num_nodes) % config->num_nodes;  // Left
        node->neighbors[1] = (i + 1) % config->num_nodes;                     // Right

        node->coupling_strength = config->global_coupling;
        node->carrier_freq_hz = 440.0f;
        node->audio_gain = 0.7f;
    }
}

// ============================================================================
// Small-World Topology (8 nodes)
// ============================================================================

void preset_small_world_8_oscillator(session_manager_t* mgr) {
    session_config_t* config = &mgr->config;

    strncpy(config->session_id, "small_world_8", MAX_SESSION_ID_LEN);
    config->topology = TOPOLOGY_SMALL_WORLD;
    config->num_nodes = 8;
    config->global_coupling = 0.4f;
    config->control_rate_hz = 500;

    // Default modes
    float omega[4], gamma[4], weight[4];
    get_default_modes(omega, gamma, weight);

    // Configure each node
    for (uint8_t i = 0; i < config->num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        node->node_id = i;
        node->personality = PERSONALITY_SELF_OSCILLATOR;  // Drones

        memcpy(node->omega, omega, sizeof(omega));
        memcpy(node->gamma, gamma, sizeof(gamma));
        memcpy(node->weight, weight, sizeof(weight));

        // Ring + long-range connections
        node->num_neighbors = 3;
        node->neighbors[0] = (i - 1 + config->num_nodes) % config->num_nodes;  // Left
        node->neighbors[1] = (i + 1) % config->num_nodes;                     // Right
        node->neighbors[2] = (i + 4) % config->num_nodes;                     // Long-range

        node->coupling_strength = config->global_coupling;
        node->carrier_freq_hz = 440.0f + i * 10.0f;  // Slight frequency variation
        node->audio_gain = 0.6f;
    }
}

// ============================================================================
// Cluster Topology (16 nodes, 2 clusters)
// ============================================================================

void preset_clusters_16(session_manager_t* mgr) {
    session_config_t* config = &mgr->config;

    strncpy(config->session_id, "clusters_16", MAX_SESSION_ID_LEN);
    config->topology = TOPOLOGY_CLUSTERS;
    config->num_nodes = 16;
    config->global_coupling = 0.25f;
    config->control_rate_hz = 500;

    // Default modes
    float omega[4], gamma[4], weight[4];
    get_default_modes(omega, gamma, weight);

    // 2 clusters of 8 nodes each
    for (uint8_t i = 0; i < config->num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        node->node_id = i;
        node->personality = PERSONALITY_RESONATOR;

        memcpy(node->omega, omega, sizeof(omega));
        memcpy(node->gamma, gamma, sizeof(gamma));
        memcpy(node->weight, weight, sizeof(weight));

        // Determine cluster (0 or 1)
        uint8_t cluster = i / 8;
        uint8_t cluster_start = cluster * 8;

        // Intra-cluster connections (strong)
        node->num_neighbors = 3;
        node->neighbors[0] = cluster_start + ((i - 1) % 8);  // Left in cluster
        node->neighbors[1] = cluster_start + ((i + 1) % 8);  // Right in cluster

        // Inter-cluster bridge (weak, only from node 3 and 11)
        if (i == 3 || i == 11) {
            node->neighbors[2] = (i + 8) % 16;  // Bridge to other cluster
            node->coupling_strength = config->global_coupling * 0.5f;  // Weaker bridge
        } else {
            node->neighbors[2] = cluster_start + ((i + 4) % 8);  // Diagonal in cluster
            node->coupling_strength = config->global_coupling;
        }

        node->carrier_freq_hz = 440.0f + cluster * 220.0f;  // Cluster frequency offset
        node->audio_gain = 0.7f;
    }
}

// ============================================================================
// Hub-Spoke Topology (16 nodes, hub = node 0)
// ============================================================================

static void preset_hub_spoke_16(session_manager_t* mgr) {
    session_config_t* config = &mgr->config;

    strncpy(config->session_id, "hub_spoke_16", MAX_SESSION_ID_LEN);
    config->topology = TOPOLOGY_HUB_SPOKE;
    config->num_nodes = 16;
    config->global_coupling = 0.3f;
    config->control_rate_hz = 500;

    // Default modes
    float omega[4], gamma[4], weight[4];
    get_default_modes(omega, gamma, weight);

    // Hub node (node 0)
    node_config_t* hub = &config->nodes[0];
    hub->node_id = 0;
    hub->personality = PERSONALITY_SELF_OSCILLATOR;  // Hub is always active
    memcpy(hub->omega, omega, sizeof(omega));
    memcpy(hub->gamma, gamma, sizeof(gamma));
    memcpy(hub->weight, weight, sizeof(weight));

    // Hub connects to all spokes
    hub->num_neighbors = config->num_nodes - 1;
    for (uint8_t i = 0; i < hub->num_neighbors; i++) {
        hub->neighbors[i] = i + 1;
    }
    hub->coupling_strength = config->global_coupling;
    hub->carrier_freq_hz = 440.0f;
    hub->audio_gain = 0.5f;  // Lower gain for hub

    // Spoke nodes (nodes 1-15)
    for (uint8_t i = 1; i < config->num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        node->node_id = i;
        node->personality = PERSONALITY_RESONATOR;
        memcpy(node->omega, omega, sizeof(omega));
        memcpy(node->gamma, gamma, sizeof(gamma));
        memcpy(node->weight, weight, sizeof(weight));

        // Each spoke only connects to hub
        node->num_neighbors = 1;
        node->neighbors[0] = 0;

        node->coupling_strength = config->global_coupling;
        node->carrier_freq_hz = 440.0f + i * 20.0f;  // Frequency spread
        node->audio_gain = 0.7f;
    }
}
