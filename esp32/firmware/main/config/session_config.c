/**
 * @file session_config.c
 * @brief Session configuration management (stub implementation)
 *
 * TODO: Full implementation in Phase 3
 * This stub provides basic initialization
 */

#include "session_config.h"
#include <string.h>

// ============================================================================
// Initialization
// ============================================================================

void session_manager_init(session_manager_t* mgr, uint8_t my_node_id) {
    if (!mgr) return;

    memset(mgr, 0, sizeof(session_manager_t));
    mgr->my_node_id = my_node_id;
    mgr->state = SESSION_STATE_IDLE;
    mgr->is_controller = false;
}

// ============================================================================
// Configuration Loading (Stubs)
// ============================================================================

bool session_load_config_json(session_manager_t* mgr, const char* json_str) {
    // TODO: Implement JSON parsing in Phase 3
    return false;
}

bool session_load_config_binary(session_manager_t* mgr,
                               const uint8_t* data,
                               size_t len) {
    if (!mgr || !data) return false;

    // Binary format is just a direct memory copy of session_config_t
    // This is simple but not portable across architectures
    if (len != sizeof(session_config_t)) {
        return false;
    }

    memcpy(&mgr->config, data, sizeof(session_config_t));
    mgr->state = SESSION_STATE_READY;

    return true;
}

size_t session_serialize_config_binary(const session_manager_t* mgr,
                                       uint8_t* data,
                                       size_t max_len) {
    if (!mgr || !data) return 0;

    size_t required_size = sizeof(session_config_t);
    if (max_len < required_size) {
        return 0;
    }

    memcpy(data, &mgr->config, required_size);
    return required_size;
}

const node_config_t* session_get_my_config(const session_manager_t* mgr) {
    if (!mgr || mgr->state < SESSION_STATE_READY) {
        return NULL;
    }

    // Find node config matching our node ID
    for (uint8_t i = 0; i < mgr->config.num_nodes; i++) {
        if (mgr->config.nodes[i].node_id == mgr->my_node_id) {
            return &mgr->config.nodes[i];
        }
    }

    return NULL;
}

bool session_apply_to_node(const session_manager_t* mgr, modal_node_t* node) {
    if (!mgr || !node) return false;

    const node_config_t* config = session_get_my_config(mgr);
    if (!config) return false;

    // Apply mode parameters
    for (int i = 0; i < MAX_MODES; i++) {
        node->modes[i].omega = config->omega[i];
        node->modes[i].gamma = config->gamma[i];
        node->modes[i].weight = config->weight[i];
    }

    // Apply personality
    node->personality = config->personality;

    // Apply coupling strength (stored in node for network use)
    // Note: neighbors are managed by the network layer, not modal_node

    return true;
}

// ============================================================================
// Session Control
// ============================================================================

bool session_start(session_manager_t* mgr) {
    if (!mgr) return false;

    mgr->state = SESSION_STATE_RUNNING;
    mgr->session_start_ms = 0; // TODO: Get actual time

    return true;
}

void session_stop(session_manager_t* mgr) {
    if (!mgr) return;

    mgr->state = SESSION_STATE_IDLE;
}

bool session_is_running(const session_manager_t* mgr) {
    if (!mgr) return false;
    return mgr->state == SESSION_STATE_RUNNING;
}

uint32_t session_get_elapsed_ms(const session_manager_t* mgr) {
    // TODO: Implement actual timing
    return 0;
}

// ============================================================================
// Topology Generators
// ============================================================================

void topology_generate_ring(session_config_t* config, uint8_t num_nodes) {
    if (!config || num_nodes == 0 || num_nodes > MAX_NODES_IN_SESSION) return;

    config->num_nodes = num_nodes;
    config->topology = TOPOLOGY_RING;

    for (uint8_t i = 0; i < num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        // Ring topology: each node connects to left and right neighbors
        node->num_neighbors = 2;
        node->neighbors[0] = (i - 1 + num_nodes) % num_nodes;  // Left
        node->neighbors[1] = (i + 1) % num_nodes;              // Right
    }
}

void topology_generate_small_world(session_config_t* config,
                                   uint8_t num_nodes,
                                   float rewire_prob) {
    if (!config || num_nodes == 0 || num_nodes > MAX_NODES_IN_SESSION) return;

    config->num_nodes = num_nodes;
    config->topology = TOPOLOGY_SMALL_WORLD;

    // Start with ring topology
    topology_generate_ring(config, num_nodes);

    // Add long-range connections (simple Watts-Strogatz model)
    for (uint8_t i = 0; i < num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        if (node->num_neighbors < MAX_NEIGHBORS) {
            // Add one long-range connection (skip neighbors)
            uint8_t long_range = (i + num_nodes / 2) % num_nodes;
            node->neighbors[node->num_neighbors++] = long_range;
        }
    }
}

void topology_generate_clusters(session_config_t* config,
                               uint8_t num_nodes,
                               uint8_t num_clusters) {
    if (!config || num_nodes == 0 || num_nodes > MAX_NODES_IN_SESSION) return;
    if (num_clusters == 0 || num_clusters > num_nodes) return;

    config->num_nodes = num_nodes;
    config->topology = TOPOLOGY_CLUSTERS;

    uint8_t cluster_size = num_nodes / num_clusters;

    for (uint8_t i = 0; i < num_nodes; i++) {
        node_config_t* node = &config->nodes[i];

        uint8_t cluster = i / cluster_size;
        uint8_t cluster_start = cluster * cluster_size;
        uint8_t cluster_end = (cluster + 1) * cluster_size;
        if (cluster_end > num_nodes) cluster_end = num_nodes;

        // Intra-cluster connections (ring within cluster)
        node->num_neighbors = 0;

        // Left neighbor in cluster
        uint8_t left = (i == cluster_start) ? (cluster_end - 1) : (i - 1);
        node->neighbors[node->num_neighbors++] = left;

        // Right neighbor in cluster
        uint8_t right = (i == cluster_end - 1) ? cluster_start : (i + 1);
        node->neighbors[node->num_neighbors++] = right;

        // Inter-cluster bridge (from first node in each cluster)
        if (i == cluster_start && cluster < num_clusters - 1) {
            uint8_t next_cluster_start = (cluster + 1) * cluster_size;
            if (next_cluster_start < num_nodes) {
                node->neighbors[node->num_neighbors++] = next_cluster_start;
            }
        }
    }
}

void topology_generate_hub_spoke(session_config_t* config,
                                uint8_t num_nodes,
                                uint8_t hub_id) {
    if (!config || num_nodes == 0 || num_nodes > MAX_NODES_IN_SESSION) return;
    if (hub_id >= num_nodes) return;

    config->num_nodes = num_nodes;
    config->topology = TOPOLOGY_HUB_SPOKE;

    // Hub node connects to all spokes
    node_config_t* hub = &config->nodes[hub_id];
    hub->num_neighbors = 0;

    for (uint8_t i = 0; i < num_nodes; i++) {
        if (i != hub_id && hub->num_neighbors < MAX_NEIGHBORS) {
            hub->neighbors[hub->num_neighbors++] = i;
        }
    }

    // Spoke nodes only connect to hub
    for (uint8_t i = 0; i < num_nodes; i++) {
        if (i != hub_id) {
            node_config_t* spoke = &config->nodes[i];
            spoke->num_neighbors = 1;
            spoke->neighbors[0] = hub_id;
        }
    }
}

// ============================================================================
// Presets (Stubs)
// ============================================================================

void preset_ring_16_resonator(session_manager_t* mgr) {
    // TODO: Implement in Phase 3
}

void preset_small_world_8_oscillator(session_manager_t* mgr) {
    // TODO: Implement in Phase 3
}

void preset_clusters_16(session_manager_t* mgr) {
    // TODO: Implement in Phase 3
}
