#include "ledstrip.h"
#include <driver/rmt_tx.h>
#include <ledstrip_encoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static size_t led_count = 0;
static size_t buffer_size = 0;
static uint8_t* ledstrip_data = NULL;

static rmt_channel_handle_t rmt_handle;
static rmt_encoder_handle_t rmt_encoder = NULL;
static uint8_t currentLight;
static const char* TAG = "ledstrip";

void ledstrip_setLedsCount(size_t new_led_count) {
    led_count = new_led_count;
    buffer_size = led_count * 3;
    if (ledstrip_data == NULL) {
        ledstrip_data = malloc(buffer_size);
    } else {
        ledstrip_data = realloc(ledstrip_data, buffer_size);
    }
}

void ledstrip_init() {
    ledstrip_clear(0, 0, 0);
    
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = PIN_LED,
        .mem_block_symbols = 64,
        .resolution_hz = LED_FREQ,
        .trans_queue_depth = 4
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&rmt_config, &rmt_handle));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = LED_FREQ
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &rmt_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(rmt_handle));
}

void ledstrip_setLight(uint8_t light) {
    currentLight = light;
}

void ledstrip_clear(uint8_t r, uint8_t g, uint8_t b) {
    for (size_t i = 0; i < led_count; i++) {
        ledstrip_set(i, r, g, b);
    }
}

void ledstrip_set(size_t i, uint8_t r, uint8_t g, uint8_t b) {
    if (LED_REVERSE) {
        i = led_count - i - 1;
    }

    uint8_t v0 = 0;
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    
    switch (LED_ORDER) {
        case ledstrip_RGB:
            v0 = r;
            v1 = g;
            v2 = b;
            break;
        case ledstrip_GRB:
            v1 = r;
            v0 = g;
            v2 = b;
            break;
        case ledstrip_BGR:
            v2 = r;
            v1 = g;
            v0 = b;
            break;
        case ledstrip_RBG:
            v0 = r;
            v2 = g;
            v1 = b;
            break;
        case ledstrip_GBR:
            v1 = r;
            v2 = g;
            v0 = b;
            break;
        case ledstrip_BRG:
            v2 = r;
            v0 = g;
            v1 = b;
            break;
        default:
            return;
    }

    ledstrip_data[i * 3 + 0] = (v0 * currentLight) / 255;
    ledstrip_data[i * 3 + 1] = (v1 * currentLight) / 255;
    ledstrip_data[i * 3 + 2] = (v2 * currentLight) / 255;
}

void ledstrip_flush() {
    const rmt_transmit_config_t tx_config = {
        .loop_count = 0
    };
    ESP_ERROR_CHECK(rmt_transmit(rmt_handle, rmt_encoder, ledstrip_data, buffer_size, &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(rmt_handle, portMAX_DELAY));
}