/**
 * @file debug_test.h
 * @brief Debug test infrastructure for ESP32 modal resonator
 *
 * Provides test harnesses, simulators, and debug utilities for
 * incremental testing according to TESTING.md phases.
 *
 * Usage:
 * - Enable tests via menuconfig: Component config → Debug Tests
 * - Set test phase: DEBUG_TEST_PHASE (1-5)
 * - Enable auto-run: DEBUG_AUTO_RUN_TESTS
 */

#ifndef DEBUG_TEST_H
#define DEBUG_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "modal_node.h"
#include "audio_synth.h"
#include "esp_now_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

// Test phases (set via menuconfig or define here)
#ifndef DEBUG_TEST_PHASE
#define DEBUG_TEST_PHASE 1  // Default: Phase 1 (single node)
#endif

#ifndef DEBUG_AUTO_RUN_TESTS
#define DEBUG_AUTO_RUN_TESTS 1  // Auto-run tests on boot
#endif

#ifndef DEBUG_VERBOSE_LOGGING
#define DEBUG_VERBOSE_LOGGING 1  // Enable verbose debug logs
#endif

// ============================================================================
// Debug Logging Macros
// ============================================================================

#if DEBUG_VERBOSE_LOGGING
#define DEBUG_LOG_STATE(tag, msg, ...) \
    ESP_LOGI(tag, "[STATE] " msg, ##__VA_ARGS__)
#define DEBUG_LOG_MODAL(tag, node) \
    ESP_LOGI(tag, "[MODAL] |a0|=%.3f |a1|=%.3f |a2|=%.3f |a3|=%.3f", \
             cabsf((node)->modes[0].a), cabsf((node)->modes[1].a), \
             cabsf((node)->modes[2].a), cabsf((node)->modes[3].a))
#define DEBUG_LOG_NETWORK(tag, msg, ...) \
    ESP_LOGI(tag, "[NET] " msg, ##__VA_ARGS__)
#define DEBUG_LOG_AUDIO(tag, msg, ...) \
    ESP_LOGI(tag, "[AUDIO] " msg, ##__VA_ARGS__)
#else
#define DEBUG_LOG_STATE(tag, msg, ...)
#define DEBUG_LOG_MODAL(tag, node)
#define DEBUG_LOG_NETWORK(tag, msg, ...)
#define DEBUG_LOG_AUDIO(tag, msg, ...)
#endif

// ============================================================================
// Test Context
// ============================================================================

typedef struct {
    // Test configuration
    uint8_t test_phase;           ///< Current test phase (1-5)
    bool auto_run;                ///< Auto-run tests on boot
    uint8_t my_node_id;           ///< This node's ID

    // Test state
    uint32_t test_start_time_ms;  ///< Test start timestamp
    uint32_t test_count;          ///< Number of tests run
    uint32_t test_pass;           ///< Number of tests passed
    uint32_t test_fail;           ///< Number of tests failed

    // References
    modal_node_t* node;           ///< Modal node under test
    audio_synth_t* audio;         ///< Audio synth under test
    esp_now_manager_t* network;   ///< Network manager under test
    QueueHandle_t poke_queue;     ///< Poke event queue
} debug_test_context_t;

// ============================================================================
// Test Infrastructure
// ============================================================================

/**
 * @brief Initialize debug test system
 *
 * @param ctx Test context
 * @param node Modal node reference
 * @param audio Audio synth reference
 * @param network Network manager reference
 * @param poke_queue Poke event queue
 */
void debug_test_init(debug_test_context_t* ctx,
                     modal_node_t* node,
                     audio_synth_t* audio,
                     esp_now_manager_t* network,
                     QueueHandle_t poke_queue);

/**
 * @brief Run tests for current phase
 *
 * @param ctx Test context
 */
void debug_test_run_phase(debug_test_context_t* ctx);

/**
 * @brief Print test summary
 *
 * @param ctx Test context
 */
void debug_test_print_summary(debug_test_context_t* ctx);

// ============================================================================
// Phase 1: Single Node Tests
// ============================================================================

/**
 * @brief Test 1.1: Task scheduling verification
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_1_1_task_scheduling(debug_test_context_t* ctx);

/**
 * @brief Test 1.2: Modal state monitoring
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_1_2_modal_state(debug_test_context_t* ctx);

/**
 * @brief Test 1.3: Self-test poke injection
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_1_3_self_poke(debug_test_context_t* ctx);

// ============================================================================
// Phase 2: Two Node Tests
// ============================================================================

/**
 * @brief Test 2.1: Peer discovery
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_2_1_peer_discovery(debug_test_context_t* ctx);

/**
 * @brief Test 2.2: Manual poke transmission
 *
 * @param ctx Test context
 * @param target_node_id Target node ID
 * @return true if passed
 */
bool debug_test_2_2_poke_transmission(debug_test_context_t* ctx, uint8_t target_node_id);

/**
 * @brief Test 2.3: Bidirectional messaging
 *
 * @param ctx Test context
 * @param target_node_id Target node ID
 * @return true if passed
 */
bool debug_test_2_3_bidirectional(debug_test_context_t* ctx, uint8_t target_node_id);

/**
 * @brief Test 2.4: Network statistics
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_2_4_network_stats(debug_test_context_t* ctx);

// ============================================================================
// Phase 3: Audio Tests
// ============================================================================

/**
 * @brief Test 3.1: Channel isolation
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_3_1_channel_isolation(debug_test_context_t* ctx);

/**
 * @brief Test 3.2: Beating test (440Hz + 442Hz)
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_3_2_beating(debug_test_context_t* ctx);

/**
 * @brief Test 3.3: Modal decay timing
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_3_3_decay_timing(debug_test_context_t* ctx);

/**
 * @brief Test 3.4: Self-oscillator mode
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_3_4_self_oscillator(debug_test_context_t* ctx);

/**
 * @brief Test 3.5: Multi-mode response
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_3_5_multi_mode(debug_test_context_t* ctx);

// ============================================================================
// Phase 4: Distributed Audio Tests
// ============================================================================

/**
 * @brief Test 4.1: Network-driven audio
 *
 * @param ctx Test context
 * @param target_node_id Target node ID
 * @return true if passed
 */
bool debug_test_4_1_network_audio(debug_test_context_t* ctx, uint8_t target_node_id);

/**
 * @brief Test 4.2: Configuration distribution
 *
 * @param ctx Test context
 * @return true if passed
 */
bool debug_test_4_2_config_distribution(debug_test_context_t* ctx);

// ============================================================================
// Test Simulators
// ============================================================================

/**
 * @brief MIDI simulator - generates MIDI events without hardware
 *
 * @param note MIDI note number (0-127)
 * @param velocity Note velocity (0-127)
 * @param channel MIDI channel (0-15)
 * @param note_on true for Note On, false for Note Off
 */
void debug_midi_simulator_send(uint8_t note, uint8_t velocity,
                               uint8_t channel, bool note_on);

/**
 * @brief MIDI simulator task - plays test sequences
 *
 * @param params Task parameters (debug_test_context_t*)
 */
void debug_midi_simulator_task(void* params);

/**
 * @brief Hub simulator - simple hub without MIDI hardware
 *
 * Sends periodic pokes to registered nodes for testing.
 *
 * @param params Task parameters (debug_test_context_t*)
 */
void debug_hub_simulator_task(void* params);

/**
 * @brief Network traffic generator - sends test pokes
 *
 * @param ctx Test context
 * @param target_node Target node ID
 * @param count Number of pokes to send
 * @param interval_ms Interval between pokes
 */
void debug_network_traffic_generator(debug_test_context_t* ctx,
                                     uint8_t target_node,
                                     uint32_t count,
                                     uint32_t interval_ms);

// ============================================================================
// Debug Utilities
// ============================================================================

/**
 * @brief Monitor modal state continuously
 *
 * @param ctx Test context
 * @param duration_ms Duration to monitor (ms)
 * @param interval_ms Logging interval (ms)
 */
void debug_monitor_modal_state(debug_test_context_t* ctx,
                               uint32_t duration_ms,
                               uint32_t interval_ms);

/**
 * @brief Monitor network statistics continuously
 *
 * @param ctx Test context
 * @param duration_ms Duration to monitor (ms)
 * @param interval_ms Logging interval (ms)
 */
void debug_monitor_network_stats(debug_test_context_t* ctx,
                                 uint32_t duration_ms,
                                 uint32_t interval_ms);

/**
 * @brief Inject manual poke event
 *
 * @param ctx Test context
 * @param strength Poke strength [0,1]
 * @param mode_weights Mode weights [4]
 */
void debug_inject_poke(debug_test_context_t* ctx,
                       float strength,
                       const float mode_weights[4]);

/**
 * @brief Run performance benchmark
 *
 * @param ctx Test context
 */
void debug_run_performance_benchmark(debug_test_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_TEST_H
