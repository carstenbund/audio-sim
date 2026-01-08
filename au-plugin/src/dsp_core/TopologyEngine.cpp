/**
 * @file TopologyEngine.cpp
 * @brief Network topology generation and coupling implementation
 */

#include "TopologyEngine.h"
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <complex>
#include <algorithm>

TopologyEngine::TopologyEngine(uint32_t num_voices)
    : num_voices_(num_voices)
    , coupling_strength_(0.3f)
    , topology_type_(TopologyType::None)
    , topology_param_(0.1f)
{
    allocateMatrix();
}

TopologyEngine::~TopologyEngine() {
    if (coupling_matrix_) {
        for (uint32_t i = 0; i < num_voices_; i++) {
            delete[] coupling_matrix_[i];
        }
        delete[] coupling_matrix_;
    }
}

void TopologyEngine::allocateMatrix() {
    coupling_matrix_ = new float*[num_voices_];
    for (uint32_t i = 0; i < num_voices_; i++) {
        coupling_matrix_[i] = new float[num_voices_];
        memset(coupling_matrix_[i], 0, num_voices_ * sizeof(float));
    }
}

void TopologyEngine::clearMatrix() {
    for (uint32_t i = 0; i < num_voices_; i++) {
        memset(coupling_matrix_[i], 0, num_voices_ * sizeof(float));
    }
}

void TopologyEngine::normalizeMatrix() {
    // Normalize each row so that sum of connections = 1.0 (diffusive coupling)
    for (uint32_t i = 0; i < num_voices_; i++) {
        float sum = 0.0f;
        for (uint32_t j = 0; j < num_voices_; j++) {
            sum += coupling_matrix_[i][j];
        }

        if (sum > 0.0f) {
            for (uint32_t j = 0; j < num_voices_; j++) {
                coupling_matrix_[i][j] /= sum;
            }
        }
    }
}

void TopologyEngine::generateTopology(TopologyType type, float coupling_strength) {
    topology_type_ = type;
    coupling_strength_ = coupling_strength;

    clearMatrix();

    switch (type) {
        case TopologyType::Ring:
            generateRing();
            break;

        case TopologyType::SmallWorld:
            generateSmallWorld(topology_param_);
            break;

        case TopologyType::Clustered:
            generateClustered(4); // Default cluster size of 4
            break;

        case TopologyType::HubSpoke:
            generateHubSpoke(0); // Voice 0 as hub
            break;

        case TopologyType::Random:
            generateRandom(topology_param_);
            break;

        case TopologyType::Complete:
            generateComplete();
            break;

        case TopologyType::None:
        default:
            // No coupling - matrix stays zero
            break;
    }

    normalizeMatrix();
}

void TopologyEngine::updateCoupling(ModalVoice** voices, uint32_t num_voices) {
    if (!voices || num_voices != num_voices_) return;

    // Apply coupling for each voice
    for (uint32_t i = 0; i < num_voices; i++) {
        if (!voices[i]->isActive()) continue;

        // Calculate coupling inputs for this voice
        float coupling_inputs[MAX_MODES] = {0.0f};

        for (uint32_t j = 0; j < num_voices; j++) {
            if (i == j || !voices[j]->isActive()) continue;

            float coupling_weight = coupling_matrix_[i][j];
            if (coupling_weight > 0.0f) {
                // Get mode 0 amplitude from neighbor voice
                std::complex<float> neighbor_amp = voices[j]->getMode0Amplitude();

                // Diffusive coupling: (neighbor - self) * weight
                std::complex<float> self_amp = voices[i]->getMode0Amplitude();
                std::complex<float> diff = neighbor_amp - self_amp;

                // Apply to mode 0 (can extend to all modes)
                coupling_inputs[0] += std::abs(diff) * coupling_weight * coupling_strength_;
            }
        }

        // Apply coupling inputs to voice
        voices[i]->applyCoupling(coupling_inputs);
    }
}

void TopologyEngine::generateRing() {
    // Connect each voice to its two neighbors in a ring
    for (uint32_t i = 0; i < num_voices_; i++) {
        uint32_t left = (i - 1 + num_voices_) % num_voices_;
        uint32_t right = (i + 1) % num_voices_;

        coupling_matrix_[i][left] = 1.0f;
        coupling_matrix_[i][right] = 1.0f;
    }
}

void TopologyEngine::generateSmallWorld(float rewire_prob) {
    // Start with ring topology
    generateRing();

    // Rewire each edge with probability rewire_prob
    for (uint32_t i = 0; i < num_voices_; i++) {
        for (uint32_t j = i + 1; j < num_voices_; j++) {
            if (coupling_matrix_[i][j] > 0.0f) {
                // Edge exists - rewire with probability rewire_prob
                float rand_val = static_cast<float>(rand()) / RAND_MAX;
                if (rand_val < rewire_prob) {
                    // Remove old edge
                    coupling_matrix_[i][j] = 0.0f;
                    coupling_matrix_[j][i] = 0.0f;

                    // Add random edge
                    uint32_t new_target = rand() % num_voices_;
                    if (new_target != i) {
                        coupling_matrix_[i][new_target] = 1.0f;
                        coupling_matrix_[new_target][i] = 1.0f;
                    }
                }
            }
        }
    }
}

void TopologyEngine::generateClustered(uint32_t cluster_size) {
    // Create clusters of fully connected voices
    uint32_t num_clusters = (num_voices_ + cluster_size - 1) / cluster_size;

    for (uint32_t cluster_idx = 0; cluster_idx < num_clusters; cluster_idx++) {
        uint32_t cluster_start = cluster_idx * cluster_size;
        uint32_t cluster_end = std::min(cluster_start + cluster_size, num_voices_);

        // Fully connect voices within cluster
        for (uint32_t i = cluster_start; i < cluster_end; i++) {
            for (uint32_t j = cluster_start; j < cluster_end; j++) {
                if (i != j) {
                    coupling_matrix_[i][j] = 1.0f;
                }
            }
        }

        // Connect to next cluster (sparse inter-cluster connections)
        if (cluster_idx < num_clusters - 1) {
            uint32_t next_cluster_start = (cluster_idx + 1) * cluster_size;
            if (next_cluster_start < num_voices_) {
                coupling_matrix_[cluster_start][next_cluster_start] = 0.5f;
                coupling_matrix_[next_cluster_start][cluster_start] = 0.5f;
            }
        }
    }
}

void TopologyEngine::generateHubSpoke(uint32_t hub_idx) {
    // Hub connects to all other voices
    if (hub_idx >= num_voices_) hub_idx = 0;

    for (uint32_t i = 0; i < num_voices_; i++) {
        if (i != hub_idx) {
            coupling_matrix_[hub_idx][i] = 1.0f;
            coupling_matrix_[i][hub_idx] = 1.0f;
        }
    }
}

void TopologyEngine::generateRandom(float connection_prob) {
    // Add edges with probability connection_prob
    for (uint32_t i = 0; i < num_voices_; i++) {
        for (uint32_t j = i + 1; j < num_voices_; j++) {
            float rand_val = static_cast<float>(rand()) / RAND_MAX;
            if (rand_val < connection_prob) {
                coupling_matrix_[i][j] = 1.0f;
                coupling_matrix_[j][i] = 1.0f;
            }
        }
    }
}

void TopologyEngine::generateComplete() {
    // Connect all voices to all other voices
    for (uint32_t i = 0; i < num_voices_; i++) {
        for (uint32_t j = 0; j < num_voices_; j++) {
            if (i != j) {
                coupling_matrix_[i][j] = 1.0f;
            }
        }
    }
}
