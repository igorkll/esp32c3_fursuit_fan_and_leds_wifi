#include "fan.h"

void fan_init() {
    gpio_set_level(PIN_FAN, FAN_DISABLED_LEVEL);
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_FAN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);
}

void fan_setState(bool state) {
    gpio_set_level(PIN_FAN, state ? FAN_ENABLED_LEVEL : FAN_DISABLED_LEVEL);
}
