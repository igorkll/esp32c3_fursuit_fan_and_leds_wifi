#include "main.h"
#include "ledstrip.h"
#include "fan.h"
#include "wifi.h"
#include "filesystem.h"

static const char* TAG = "esp32c3_fursuit_fan_and_leds_wifi";

typedef enum {
    Fan_state_off,
    Fan_state_on,
    Fan_state_timer
} Fan_state;

typedef struct {
    uint8_t led_count;
    uint8_t led_light;
    bool led_state;
    Fan_state fan_state;

    uint8_t primary_color[3];
} config_t;

config_t currentConfig;
static nvs_handle_t nvs;

static const config_t defaultConfig = {
    .led_count = DEFAULT_LED_COUNT,
    .led_light = DEFAULT_LIGHT_COUNT,
    .led_state = true,
    .fan_state = Fan_state_off
};

static void nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));
}

static void load_config() {
    ESP_LOGI(TAG, "loading config...");
    FILE* file = fopen("/storage/config", "rb");
    if (file) {
        fread(&currentConfig, 1, sizeof(config_t), file);
        fclose(file);
        ESP_LOGI(TAG, "config loaded!");
    } else {
        memcpy(&currentConfig, &defaultConfig, sizeof(config_t));
        ESP_LOGI(TAG, "loaded default config!");
    }
}

static void save_config() {
    ESP_LOGI(TAG, "saving config...");
    FILE* file = fopen("/storage/config", "wb");
    if (file) {
        fwrite(&currentConfig, 1, sizeof(config_t), file);
        fclose(file);
        ESP_LOGI(TAG, "config saved!");
    } else {
        ESP_LOGE(TAG, "failed to save config");
    }
}

static void wait(float milliseconds) {
    uint32_t ticks = milliseconds / portTICK_PERIOD_MS;
    if (ticks < 1) ticks = 1;
    vTaskDelay(ticks);
}

static void deadlock() {
    while (true) wait(1000);
}

static void update_fan_state() {
    switch (currentConfig.fan_state) {
        case Fan_state_off:
            fan_setState(false);
            break;

        case Fan_state_on:
            fan_setState(true);
            break;

        case Fan_state_timer:
            fan_setState(true);
            break;
    }
}

static int16_t old_led_count = -1;

static void update_ledstrip_state() {
    if (currentConfig.led_count != old_led_count) {
        ledstrip_setLedsCount(currentConfig.led_count);
        if (old_led_count < 0) ledstrip_init();
        old_led_count = currentConfig.led_count;
    }

    if (currentConfig.led_state) {
        ledstrip_setLight(currentConfig.led_light);
        ledstrip_clear(currentConfig.primary_color[0], currentConfig.primary_color[1], currentConfig.primary_color[2]);
    } else {
        ledstrip_setLight(0);
        ledstrip_clear(0, 0, 0);
    }

    ledstrip_flush();
}

static void callback_set_fan(void* user_ctx, char* content, size_t len) {
    currentConfig.fan_state = content[0];
    save_config();
    update_fan_state();
}


static void* callback_get_fan(void* user_ctx, char* content, size_t len, size_t* retsize) {
    *retsize = 1;
    uint8_t* ret = malloc(1);
    ret[0] = currentConfig.fan_state;
    return ret;
}

static void callback_set_enabled(void* user_ctx, char* content, size_t len) {
    currentConfig.led_state = content[0];
    ESP_LOGI(TAG, "set enabled: %i", currentConfig.led_state);
    save_config();
    update_ledstrip_state();
}

static void* callback_get_enabled(void* user_ctx, char* content, size_t len, size_t* retsize) {
    ESP_LOGI(TAG, "get enabled: %i", currentConfig.led_state);
    *retsize = 1;
    uint8_t* ret = malloc(1);
    ret[0] = currentConfig.led_state;
    return ret;
}

static void callback_set_light(void* user_ctx, char* content, size_t len) {
    currentConfig.led_light = content[0];
    ESP_LOGI(TAG, "set light: %i", currentConfig.led_light);
    update_ledstrip_state();
}

static void callback_set_light_and_save(void* user_ctx, char* content, size_t len) {
    currentConfig.led_light = content[0];
    ESP_LOGI(TAG, "set light: %i", currentConfig.led_light);
    save_config();
    update_ledstrip_state();
}

static void* callback_get_light(void* user_ctx, char* content, size_t len, size_t* retsize) {
    ESP_LOGI(TAG, "get light: %i", currentConfig.led_light);
    *retsize = 1;
    uint8_t* ret = malloc(1);
    ret[0] = currentConfig.led_light;
    return ret;
}

static void set_primary_color(void* user_ctx, char* content, size_t len) {
    for (size_t i = 0; i < 3; i++) {
        currentConfig.primary_color[i] = content[i];
    }
    update_ledstrip_state();
}

static void* callback_get_primary_color(void* user_ctx, char* content, size_t len, size_t* retsize) {
    *retsize = 3;
    uint8_t* ret = malloc(3);
    for (size_t i = 0; i < 3; i++) {
        ret[i] = currentConfig.primary_color[i];
    }
    return ret;
}

void app_main() {
    if (!filesystem_init()) {
        ESP_LOGE(TAG, "failed to init filesystem");
        deadlock();
    }
    
    nvs_init();
    load_config();

    update_ledstrip_state();

    fan_init();
    update_fan_state();

    wifi_init();
    
    wifi_attachCallback_post("/set_enabled", callback_set_enabled, NULL);
    wifi_attachCallback_get_binary("/get_enabled", callback_get_enabled, NULL);

    wifi_attachCallback_post("/set_light", callback_set_light, NULL);
    wifi_attachCallback_post("/set_light_and_save", callback_set_light_and_save, NULL);
    wifi_attachCallback_get_binary("/get_light", callback_get_light, NULL);

    wifi_attachCallback_post("/set_primary_color", set_primary_color, NULL);
    wifi_attachCallback_get_binary("/get_primary_color", callback_get_primary_color, NULL);

    wifi_attachCallback_post("/set_fan", callback_set_fan, NULL);
    wifi_attachCallback_get_binary("/get_fan", callback_get_fan, NULL);
}

