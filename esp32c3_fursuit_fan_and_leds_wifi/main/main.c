#include "main.h"

static const char* TAG = "esp32c3_fursuit_fan_and_leds_wifi";

void app_main() {
    gpio_config_t io_conf = {
        .pin_bit_mask = PIN_FAN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);
    
    while (true) {
        gpio_set_level(PIN_FAN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(PIN_FAN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

