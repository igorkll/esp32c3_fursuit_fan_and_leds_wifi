#include "main.h"
#include "ledstrip.h"
#include "fan.h"
#include "wifi.h"

static const char* TAG = "esp32c3_fursuit_fan_and_leds_wifi";

typedef struct {
    bool fan;
} Settings;

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
    ledstrip_init();
    ledstrip_flush();

    fan_init();

    wifi_init();
    wifi_attachCallback_post("/set_fan", callback_set_fan, NULL);
    wifi_attachCallback_post("/get_fan", callback_get_fan, NULL);
    
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

