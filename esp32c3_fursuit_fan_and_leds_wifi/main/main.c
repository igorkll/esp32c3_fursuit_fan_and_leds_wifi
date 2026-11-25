#include "main.h"
#include "ledstrip.h"

static const char* TAG = "esp32c3_fursuit_fan_and_leds_wifi";

static void fan_init() {
    gpio_set_level(PIN_FAN, FAN_DISABLED_LEVEL);
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_FAN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);
}

static void fan_enable() {
    gpio_set_level(PIN_FAN, FAN_ENABLED_LEVEL);
}

static void fan_disable() {
    gpio_set_level(PIN_FAN, FAN_DISABLED_LEVEL);
}

void app_main() {
    fan_init();

    ledstrip_init();
    ledstrip_flush();
    
    while (true) {
        fan_enable();
        vTaskDelay(pdMS_TO_TICKS(1000));

        fan_disable();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

