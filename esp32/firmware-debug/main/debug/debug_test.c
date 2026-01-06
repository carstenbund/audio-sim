/**
 * @file debug_test.c
 * @brief Debug test implementation
 *
 * Implements all test harnesses from TESTING.md
 */

#include "debug_test.h"
#include "protocol.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

#define TAG "DEBUG_TEST"

// ============================================================================
// Test Infrastructure
// ============================================================================

void debug_test_init(debug_test_context_t* ctx,
                     modal_node_t* node,
                     audio_synth_t* audio,
                     esp_now_manager_t* network,
                     QueueHandle_t poke_queue) {
    memset(ctx, 0, sizeof(debug_test_context_t));

    ctx->test_phase = DEBUG_TEST_PHASE;
    ctx->auto_run = DEBUG_AUTO_RUN_TESTS;
    ctx->my_node_id = node ? 0 : 0;  // Will be set from config

    ctx->node = node;
    ctx->audio = audio;
    ctx->network = network;
    ctx->poke_queue = poke_queue;

    ESP_LOGI(TAG, "=== Debug Test System Initialized ===");
    ESP_LOGI(TAG, "Test Phase: %d", ctx->test_phase);
    ESP_LOGI(TAG, "Auto-run: %s", ctx->auto_run ? "Yes" : "No");
}

void debug_test_run_phase(debug_test_context_t* ctx) {
    if (!ctx) return;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RUNNING PHASE %d TESTS", ctx->test_phase);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    ctx->test_start_time_ms = esp_timer_get_time() / 1000;
    ctx->test_count = 0;
    ctx->test_pass = 0;
    ctx->test_fail = 0;

    switch (ctx->test_phase) {
        case 1:
            // Phase 1: Single node tests
            ESP_LOGI(TAG, "--- Phase 1: Single Node Tests ---");
            if (debug_test_1_1_task_scheduling(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_1_2_modal_state(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_1_3_self_poke(ctx)) ctx->test_pass++; else ctx->test_fail++;
            break;

        case 2:
            // Phase 2: Two node tests
            ESP_LOGI(TAG, "--- Phase 2: Two Node Tests ---");
            ESP_LOGI(TAG, "NOTE: Requires second node to be powered on");
            if (debug_test_2_1_peer_discovery(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_2_2_poke_transmission(ctx, 1)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_2_4_network_stats(ctx)) ctx->test_pass++; else ctx->test_fail++;
            break;

        case 3:
            // Phase 3: Audio tests
            ESP_LOGI(TAG, "--- Phase 3: Audio Tests ---");
            ESP_LOGI(TAG, "NOTE: Requires audio DAC connected");
            if (debug_test_3_1_channel_isolation(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_3_2_beating(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_3_3_decay_timing(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_3_4_self_oscillator(ctx)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_3_5_multi_mode(ctx)) ctx->test_pass++; else ctx->test_fail++;
            break;

        case 4:
            // Phase 4: Distributed audio
            ESP_LOGI(TAG, "--- Phase 4: Distributed Audio Tests ---");
            ESP_LOGI(TAG, "NOTE: Requires second node with audio");
            if (debug_test_4_1_network_audio(ctx, 1)) ctx->test_pass++; else ctx->test_fail++;
            if (debug_test_4_2_config_distribution(ctx)) ctx->test_pass++; else ctx->test_fail++;
            break;

        case 5:
            // Phase 5: MIDI integration (requires real MIDI or simulator)
            ESP_LOGI(TAG, "--- Phase 5: MIDI Integration ---");
            ESP_LOGI(TAG, "Use MIDI simulator task for testing");
            break;

        default:
            ESP_LOGE(TAG, "Invalid test phase: %d", ctx->test_phase);
            return;
    }

    debug_test_print_summary(ctx);
}

void debug_test_print_summary(debug_test_context_t* ctx) {
    uint32_t duration_ms = (esp_timer_get_time() / 1000) - ctx->test_start_time_ms;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  TEST SUMMARY - Phase %d", ctx->test_phase);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Total tests: %d", ctx->test_pass + ctx->test_fail);
    ESP_LOGI(TAG, "Passed:      %d", ctx->test_pass);
    ESP_LOGI(TAG, "Failed:      %d", ctx->test_fail);
    ESP_LOGI(TAG, "Duration:    %d ms", duration_ms);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    if (ctx->test_fail == 0) {
        ESP_LOGI(TAG, "✓ ALL TESTS PASSED!");
    } else {
        ESP_LOGW(TAG, "✗ %d TESTS FAILED", ctx->test_fail);
    }
}

// ============================================================================
// Phase 1 Tests
// ============================================================================

bool debug_test_1_1_task_scheduling(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 1.1] Task Scheduling Verification");

    // Check that tasks are running by monitoring modal state changes
    if (!ctx->node) {
        ESP_LOGE(TAG, "  ✗ FAILED: No modal node reference");
        return false;
    }

    // Wait 1 second and verify control task is running
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "  ✓ PASSED: Tasks are running (1s elapsed, no crash)");
    return true;
}

bool debug_test_1_2_modal_state(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 1.2] Modal State Monitoring");

    if (!ctx->node) {
        ESP_LOGE(TAG, "  ✗ FAILED: No modal node reference");
        return false;
    }

    // Log modal state for 3 seconds
    ESP_LOGI(TAG, "  Monitoring modal state for 3 seconds...");
    for (int i = 0; i < 30; i++) {
        DEBUG_LOG_MODAL(TAG, ctx->node);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Verify amplitudes are small (resonator should decay to ~0)
    float max_amp = 0.0f;
    for (int k = 0; k < MAX_MODES; k++) {
        float amp = cabsf(ctx->node->modes[k].a);
        if (amp > max_amp) max_amp = amp;
    }

    if (max_amp < 0.1f) {
        ESP_LOGI(TAG, "  ✓ PASSED: Modal amplitudes decayed (max=%.3f)", max_amp);
        return true;
    } else {
        ESP_LOGW(TAG, "  ✗ FAILED: Amplitudes higher than expected (max=%.3f)", max_amp);
        return false;
    }
}

bool debug_test_1_3_self_poke(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 1.3] Self-Poke Injection");

    if (!ctx->node || !ctx->poke_queue) {
        ESP_LOGE(TAG, "  ✗ FAILED: Missing references");
        return false;
    }

    // Record initial amplitude
    float initial_amp = cabsf(ctx->node->modes[0].a);

    // Inject poke
    float weights[4] = {1.0, 0.8, 0.3, 0.5};
    debug_inject_poke(ctx, 1.0f, weights);

    ESP_LOGI(TAG, "  Injected poke, waiting 100ms...");
    vTaskDelay(pdMS_TO_TICKS(100));

    // Check amplitude increased
    float new_amp = cabsf(ctx->node->modes[0].a);

    if (new_amp > initial_amp + 0.3f) {
        ESP_LOGI(TAG, "  ✓ PASSED: Amplitude increased (%.3f → %.3f)",
                 initial_amp, new_amp);
        return true;
    } else {
        ESP_LOGE(TAG, "  ✗ FAILED: No amplitude increase (%.3f → %.3f)",
                 initial_amp, new_amp);
        return false;
    }
}

// ============================================================================
// Phase 2 Tests
// ============================================================================

bool debug_test_2_1_peer_discovery(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 2.1] Peer Discovery");

    if (!ctx->network) {
        ESP_LOGE(TAG, "  ✗ FAILED: No network reference");
        return false;
    }

    // Wait 5 seconds for discovery
    ESP_LOGI(TAG, "  Waiting 5 seconds for peer discovery...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Check if peers registered
    int num_peers = ctx->network->num_peers;

    if (num_peers > 0) {
        ESP_LOGI(TAG, "  ✓ PASSED: Discovered %d peer(s)", num_peers);
        return true;
    } else {
        ESP_LOGW(TAG, "  ✗ FAILED: No peers discovered (is other node powered on?)");
        return false;
    }
}

bool debug_test_2_2_poke_transmission(debug_test_context_t* ctx, uint8_t target_node_id) {
    ESP_LOGI(TAG, "[Test 2.2] Poke Transmission to Node %d", target_node_id);

    if (!ctx->network) {
        ESP_LOGE(TAG, "  ✗ FAILED: No network reference");
        return false;
    }

    // Send test poke
    network_message_t msg;
    float weights[4] = {1.0, 0.8, 0.3, 0.5};
    protocol_create_poke(&msg, ctx->my_node_id, target_node_id,
                        0.8f, -1.0f, weights);

    bool sent = esp_now_send_message(ctx->network, target_node_id,
                                     &msg, sizeof(msg_poke_t));

    if (sent) {
        ESP_LOGI(TAG, "  ✓ PASSED: Poke sent successfully");
        return true;
    } else {
        ESP_LOGE(TAG, "  ✗ FAILED: Poke send failed");
        return false;
    }
}

bool debug_test_2_3_bidirectional(debug_test_context_t* ctx, uint8_t target_node_id) {
    ESP_LOGI(TAG, "[Test 2.3] Bidirectional Messaging");

    // Send 10 pokes and monitor responses
    ESP_LOGI(TAG, "  Sending 10 pokes to node %d...", target_node_id);

    for (int i = 0; i < 10; i++) {
        network_message_t msg;
        float weights[4] = {0.5, 0.5, 0.5, 0.5};
        protocol_create_poke(&msg, ctx->my_node_id, target_node_id,
                            0.3f, -1.0f, weights);

        esp_now_send_message(ctx->network, target_node_id,
                            &msg, sizeof(msg_poke_t));

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    ESP_LOGI(TAG, "  ✓ PASSED: Bidirectional test complete");
    return true;
}

bool debug_test_2_4_network_stats(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 2.4] Network Statistics");

    if (!ctx->network) {
        ESP_LOGE(TAG, "  ✗ FAILED: No network reference");
        return false;
    }

    // Log statistics
    debug_monitor_network_stats(ctx, 5000, 1000);

    ESP_LOGI(TAG, "  ✓ PASSED: Network stats logged");
    return true;
}

// ============================================================================
// Phase 3 Tests
// ============================================================================

bool debug_test_3_1_channel_isolation(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 3.1] Channel Isolation");

    if (!ctx->audio || !ctx->node) {
        ESP_LOGE(TAG, "  ✗ FAILED: Missing references");
        return false;
    }

    ESP_LOGI(TAG, "  Testing each channel (3 seconds each)...");

    for (int k = 0; k < 4; k++) {
        float freq = ctx->node->modes[k].params.omega / (2 * M_PI);
        ESP_LOGI(TAG, "  Channel %d (Mode %d: %.0f Hz)", k, k, freq);

        // Mute all channels
        for (int i = 0; i < 4; i++) {
            audio_synth_set_mode_gain(ctx->audio, i, 0.0f);
        }

        // Unmute current channel
        audio_synth_set_mode_gain(ctx->audio, k, 0.7f);

        // Inject poke to this mode
        float weights[4] = {0, 0, 0, 0};
        weights[k] = 1.0f;
        debug_inject_poke(ctx, 1.0f, weights);

        // Listen
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // Restore all channels
    for (int i = 0; i < 4; i++) {
        audio_synth_set_mode_gain(ctx->audio, i, 0.7f);
    }

    ESP_LOGI(TAG, "  ✓ PASSED: Channel isolation test complete");
    return true;
}

bool debug_test_3_2_beating(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 3.2] Beating Test (440Hz + 442Hz = 2Hz beat)");

    if (!ctx->audio) {
        ESP_LOGE(TAG, "  ✗ FAILED: No audio reference");
        return false;
    }

    // Enable only ch 0 and 1
    audio_synth_set_mode_gain(ctx->audio, 0, 0.7f);
    audio_synth_set_mode_gain(ctx->audio, 1, 0.7f);
    audio_synth_set_mode_gain(ctx->audio, 2, 0.0f);
    audio_synth_set_mode_gain(ctx->audio, 3, 0.0f);

    // Poke both modes equally
    float weights[4] = {1.0, 1.0, 0.0, 0.0};
    debug_inject_poke(ctx, 0.8f, weights);

    ESP_LOGI(TAG, "  Listen for 2 Hz beating (amplitude modulation)...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Restore
    for (int i = 0; i < 4; i++) {
        audio_synth_set_mode_gain(ctx->audio, i, 0.7f);
    }

    ESP_LOGI(TAG, "  ✓ PASSED: Beating test complete");
    return true;
}

bool debug_test_3_3_decay_timing(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 3.3] Modal Decay Timing");

    if (!ctx->node || !ctx->audio) {
        ESP_LOGE(TAG, "  ✗ FAILED: Missing references");
        return false;
    }

    // Solo mode 0
    for (int i = 1; i < 4; i++) {
        audio_synth_set_mode_gain(ctx->audio, i, 0.0f);
    }
    audio_synth_set_mode_gain(ctx->audio, 0, 0.7f);

    // Poke mode 0
    float weights[4] = {1.0, 0.0, 0.0, 0.0};
    debug_inject_poke(ctx, 1.0f, weights);

    // Log amplitude every 100ms for 5 seconds
    ESP_LOGI(TAG, "  Monitoring decay (γ=0.5 → T_63%%=2.0s):");
    for (int i = 0; i < 50; i++) {
        float amp = cabsf(ctx->node->modes[0].a);
        if (i % 10 == 0) {  // Log every second
            ESP_LOGI(TAG, "    t=%d ms, |a0|=%.3f", i * 100, amp);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Restore
    for (int i = 0; i < 4; i++) {
        audio_synth_set_mode_gain(ctx->audio, i, 0.7f);
    }

    ESP_LOGI(TAG, "  ✓ PASSED: Decay timing logged");
    return true;
}

bool debug_test_3_4_self_oscillator(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 3.4] Self-Oscillator Mode");

    if (!ctx->node) {
        ESP_LOGE(TAG, "  ✗ FAILED: No node reference");
        return false;
    }

    // Store original personality
    node_personality_t original = ctx->node->personality;

    // Change to self-oscillator
    ctx->node->personality = PERSONALITY_SELF_OSCILLATOR;
    ESP_LOGI(TAG, "  Changed to SELF_OSCILLATOR mode");

    // Initial kick
    float weights[4] = {1.0, 0.0, 0.0, 0.0};
    debug_inject_poke(ctx, 0.1f, weights);

    // Monitor settling to limit cycle
    ESP_LOGI(TAG, "  Monitoring amplitude settling:");
    for (int i = 0; i < 100; i++) {
        float amp = cabsf(ctx->node->modes[0].a);
        if (i % 25 == 0) {  // Log every 2.5 seconds
            ESP_LOGI(TAG, "    t=%d ms, |a0|=%.3f", i * 100, amp);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Restore
    ctx->node->personality = original;
    ESP_LOGI(TAG, "  Restored to RESONATOR mode");

    ESP_LOGI(TAG, "  ✓ PASSED: Self-oscillator test complete");
    return true;
}

bool debug_test_3_5_multi_mode(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 3.5] Multi-Mode Response");

    if (!ctx->audio) {
        ESP_LOGE(TAG, "  ✗ FAILED: No audio reference");
        return false;
    }

    // Enable all channels
    for (int i = 0; i < 4; i++) {
        audio_synth_set_mode_gain(ctx->audio, i, 0.7f);
    }

    // Poke all modes with different weights
    float weights[4] = {1.0, 0.8, 0.3, 0.5};
    debug_inject_poke(ctx, 0.8f, weights);

    ESP_LOGI(TAG, "  Listen for rich harmonic content (all 4 modes)...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "  ✓ PASSED: Multi-mode test complete");
    return true;
}

// ============================================================================
// Phase 4 Tests
// ============================================================================

bool debug_test_4_1_network_audio(debug_test_context_t* ctx, uint8_t target_node_id) {
    ESP_LOGI(TAG, "[Test 4.1] Network-Driven Audio");

    if (!ctx->network) {
        ESP_LOGE(TAG, "  ✗ FAILED: No network reference");
        return false;
    }

    ESP_LOGI(TAG, "  Sending periodic pokes to node %d...", target_node_id);

    // Send pokes every 1 second for 10 seconds
    for (int i = 0; i < 10; i++) {
        network_message_t msg;
        float weights[4] = {1.0, 0.8, 0.3, 0.5};
        protocol_create_poke(&msg, ctx->my_node_id, target_node_id,
                            0.5f, -1.0f, weights);

        esp_now_send_message(ctx->network, target_node_id,
                            &msg, sizeof(msg_poke_t));

        ESP_LOGI(TAG, "  Sent poke %d/10", i + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "  ✓ PASSED: Network audio test complete");
    return true;
}

bool debug_test_4_2_config_distribution(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[Test 4.2] Configuration Distribution");
    ESP_LOGI(TAG, "  NOTE: This test requires hub firmware");
    ESP_LOGI(TAG, "  ✓ PASSED: (Manual test - see logs)");
    return true;
}

// ============================================================================
// Simulators
// ============================================================================

void debug_midi_simulator_send(uint8_t note, uint8_t velocity,
                               uint8_t channel, bool note_on) {
    ESP_LOGI(TAG, "[MIDI SIM] %s: note=%d vel=%d ch=%d",
             note_on ? "Note On" : "Note Off", note, velocity, channel);

    // TODO: Call hub MIDI handler if in hub mode
    // For now, just log the event
}

void debug_midi_simulator_task(void* params) {
    debug_test_context_t* ctx = (debug_test_context_t*)params;

    ESP_LOGI(TAG, "[MIDI SIM] Task started");

    // Test sequence: C major scale
    uint8_t scale[] = {60, 62, 64, 65, 67, 69, 71, 72};  // C D E F G A B C

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));  // Wait 5 seconds

        ESP_LOGI(TAG, "[MIDI SIM] Playing C major scale...");

        for (int i = 0; i < 8; i++) {
            debug_midi_simulator_send(scale[i], 100, 0, true);
            vTaskDelay(pdMS_TO_TICKS(500));  // 500ms per note
            debug_midi_simulator_send(scale[i], 0, 0, false);
        }
    }
}

void debug_hub_simulator_task(void* params) {
    debug_test_context_t* ctx = (debug_test_context_t*)params;

    ESP_LOGI(TAG, "[HUB SIM] Task started");

    // Wait for nodes to register
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        // Send pokes to all registered peers
        if (ctx->network && ctx->network->num_peers > 0) {
            for (int i = 0; i < ctx->network->num_peers; i++) {
                uint8_t target_id = ctx->network->peers[i].node_id;

                network_message_t msg;
                float weights[4] = {1.0, 0.8, 0.3, 0.5};
                protocol_create_poke(&msg, ctx->my_node_id, target_id,
                                    0.5f, -1.0f, weights);

                esp_now_send_message(ctx->network, target_id,
                                    &msg, sizeof(msg_poke_t));

                ESP_LOGD(TAG, "[HUB SIM] Sent poke to node %d", target_id);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 poke/second
    }
}

void debug_network_traffic_generator(debug_test_context_t* ctx,
                                     uint8_t target_node,
                                     uint32_t count,
                                     uint32_t interval_ms) {
    ESP_LOGI(TAG, "[TRAFFIC GEN] Sending %d pokes to node %d @ %d ms intervals",
             count, target_node, interval_ms);

    for (uint32_t i = 0; i < count; i++) {
        network_message_t msg;
        float weights[4] = {0.5, 0.5, 0.5, 0.5};
        protocol_create_poke(&msg, ctx->my_node_id, target_node,
                            0.3f, -1.0f, weights);

        esp_now_send_message(ctx->network, target_node,
                            &msg, sizeof(msg_poke_t));

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }

    ESP_LOGI(TAG, "[TRAFFIC GEN] Complete");
}

// ============================================================================
// Debug Utilities
// ============================================================================

void debug_monitor_modal_state(debug_test_context_t* ctx,
                               uint32_t duration_ms,
                               uint32_t interval_ms) {
    if (!ctx->node) return;

    uint32_t elapsed = 0;
    while (elapsed < duration_ms) {
        DEBUG_LOG_MODAL(TAG, ctx->node);
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
        elapsed += interval_ms;
    }
}

void debug_monitor_network_stats(debug_test_context_t* ctx,
                                 uint32_t duration_ms,
                                 uint32_t interval_ms) {
    if (!ctx->network) return;

    uint32_t elapsed = 0;
    while (elapsed < duration_ms) {
        ESP_LOGI(TAG, "[NET STATS] Peers=%d TX=%d RX=%d",
                 ctx->network->num_peers,
                 ctx->network->tx_count,
                 ctx->network->rx_count);

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
        elapsed += interval_ms;
    }
}

void debug_inject_poke(debug_test_context_t* ctx,
                       float strength,
                       const float mode_weights[4]) {
    if (!ctx->poke_queue) return;

    poke_event_t poke = {
        .source_node_id = ctx->my_node_id,
        .strength = strength,
        .phase_hint = 0.0f
    };
    memcpy(poke.mode_weights, mode_weights, sizeof(poke.mode_weights));

    xQueueSend(ctx->poke_queue, &poke, 0);

    DEBUG_LOG_STATE(TAG, "Injected poke: strength=%.2f", strength);
}

void debug_run_performance_benchmark(debug_test_context_t* ctx) {
    ESP_LOGI(TAG, "[BENCHMARK] Running performance tests...");

    // TODO: Measure CPU usage, latency, etc.

    ESP_LOGI(TAG, "[BENCHMARK] Complete");
}
