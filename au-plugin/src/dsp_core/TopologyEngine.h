/**
 * @file TopologyEngine.h
 * @brief Network topology generation and voice coupling
 *
 * Implements various network topologies for voice coupling:
 * - Ring/Chain
 * - Small-world (Watts-Strogatz)
 * - Clustered/Modular
 * - Hub-and-spoke (Star)
 * - Random (Erdős–Rényi)
 * - Complete graph (all-to-all)
 */

#ifndef TOPOLOGY_ENGINE_H
#define TOPOLOGY_ENGINE_H

#include "ModalVoice.h"
#include <cstdint>

/**
 * @brief Topology types
 */
enum class TopologyType {
    Ring,           ///< Each voice connected to 2 neighbors
    SmallWorld,     ///< Small-world network (Watts-Strogatz)
    Clustered,      ///< Modular/clustered structure
    HubSpoke,       ///< Star topology (hub-and-spoke)
    Random,         ///< Random connections (Erdős–Rényi)
    Complete,       ///< All voices connected to all others
    None            ///< No coupling
};

class TopologyEngine {
public:
    /**
     * @brief Constructor
     * @param num_voices Number of voices in the network
     */
    TopologyEngine(uint32_t num_voices);

    /**
     * @brief Destructor
     */
    ~TopologyEngine();

    /**
     * @brief Generate topology
     * @param type Topology type
     * @param coupling_strength Global coupling strength (0.0-1.0)
     */
    void generateTopology(TopologyType type, float coupling_strength);

    /**
     * @brief Update coupling between voices
     * @param voices Array of voice pointers
     * @param num_voices Number of voices
     */
    void updateCoupling(ModalVoice** voices, uint32_t num_voices);

    /**
     * @brief Set coupling strength
     * @param strength Coupling strength (0.0-1.0)
     */
    void setCouplingStrength(float strength) {
        coupling_strength_ = strength;
    }

    /**
     * @brief Get coupling strength
     * @return Current coupling strength
     */
    float getCouplingStrength() const {
        return coupling_strength_;
    }

    /**
     * @brief Get current topology type
     * @return Topology type
     */
    TopologyType getTopologyType() const {
        return topology_type_;
    }

    /**
     * @brief Set topology parameter (e.g., rewiring probability for small-world)
     * @param param Parameter value (0.0-1.0)
     */
    void setTopologyParameter(float param) {
        topology_param_ = param;
    }

    /**
     * @brief Get topology parameter
     * @return Topology parameter
     */
    float getTopologyParameter() const {
        return topology_param_;
    }

private:
    uint32_t num_voices_;           ///< Number of voices
    float** coupling_matrix_;       ///< Coupling matrix [num_voices][num_voices]
    float coupling_strength_;       ///< Global coupling strength
    TopologyType topology_type_;    ///< Current topology type
    float topology_param_;          ///< Topology-specific parameter

    /**
     * @brief Allocate coupling matrix
     */
    void allocateMatrix();

    /**
     * @brief Clear coupling matrix (set all to zero)
     */
    void clearMatrix();

    /**
     * @brief Normalize coupling matrix rows (diffusive coupling)
     */
    void normalizeMatrix();

    /**
     * @brief Generate ring topology
     */
    void generateRing();

    /**
     * @brief Generate small-world topology
     * @param rewire_prob Rewiring probability (0.0-1.0)
     */
    void generateSmallWorld(float rewire_prob);

    /**
     * @brief Generate clustered topology
     * @param cluster_size Size of each cluster
     */
    void generateClustered(uint32_t cluster_size);

    /**
     * @brief Generate hub-spoke topology
     * @param hub_idx Hub voice index
     */
    void generateHubSpoke(uint32_t hub_idx);

    /**
     * @brief Generate random topology
     * @param connection_prob Connection probability (0.0-1.0)
     */
    void generateRandom(float connection_prob);

    /**
     * @brief Generate complete graph topology
     */
    void generateComplete();
};

#endif // TOPOLOGY_ENGINE_H
