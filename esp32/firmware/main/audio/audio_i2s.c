/**
 * @file audio_i2s.c
 * @brief I2S audio output driver for PCM5102A DAC
 *
 * Configures ESP32 I2S peripheral for:
 * - 48kHz sample rate
 * - 16-bit samples
 * - Mono output (left channel)
 * - DMA buffers for low-latency
 *
 * Hardware connections:
 * - GPIO 25: BCK (bit clock)
 * - GPIO 26: WS (word select / LRCK)
 * - GPIO 27: DIN (data in)
 */

#include "audio_synth.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ============================================================================
// Configuration
// ============================================================================

#define TAG "AUDIO_I2S"

// I2S peripheral
#define I2S_NUM I2S_NUM_0

// GPIO pins for I2S
#define I2S_BCK_PIN     25  // Bit clock
#define I2S_WS_PIN      26  // Word select (LRCK)
#define I2S_DATA_PIN    27  // Data out

// Buffer configuration
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     480  // 10ms @ 48kHz

// ============================================================================
// I2S Initialization
// ============================================================================

void audio_i2s_init(void) {
    ESP_LOGI(TAG, "Initializing I2S driver");

    // I2S configuration
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono on left channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,  // Use PLL (more accurate timing)
        .tx_desc_auto_clear = true,  // Auto-clear on underrun
        .fixed_mclk = 0
    };

    // I2S pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    // Install and configure I2S driver
    esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(err));
        return;
    }

    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(err));
        return;
    }

    // Set clock (ensure exact 48kHz)
    i2s_set_clk(I2S_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    ESP_LOGI(TAG, "I2S driver initialized successfully");
    ESP_LOGI(TAG, "  Sample rate: %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "  Bits per sample: 16");
    ESP_LOGI(TAG, "  Channel: Mono (left)");
    ESP_LOGI(TAG, "  DMA buffers: %d x %d samples", DMA_BUF_COUNT, DMA_BUF_LEN);
}

// ============================================================================
// I2S Write
// ============================================================================

size_t audio_i2s_write(const int16_t* buffer, size_t size) {
    size_t bytes_written = 0;

    esp_err_t err = i2s_write(I2S_NUM, buffer, size, &bytes_written, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "I2S write error: %s", esp_err_to_name(err));
        return 0;
    }

    return bytes_written;
}

// ============================================================================
// Audio Task
// ============================================================================

void audio_task(void* pvParameters) {
    audio_synth_t* synth = (audio_synth_t*)pvParameters;

    ESP_LOGI(TAG, "Audio task started on core %d", xPortGetCoreID());
    ESP_LOGI(TAG, "Generating 48kHz audio");

    // Wait a bit for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    while (1) {
        // Generate one buffer of audio (480 samples = 10ms)
        int16_t* audio_buffer = audio_synth_generate_buffer(synth);

        // Write to I2S (this blocks until DMA buffer is available)
        size_t bytes_written = audio_i2s_write(audio_buffer, AUDIO_BUFFER_SIZE * sizeof(int16_t));

        if (bytes_written == 0) {
            ESP_LOGW(TAG, "I2S write failed, retrying...");
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // No explicit delay needed - i2s_write blocks until buffer ready
        // This gives us precise 48kHz timing
    }
}
