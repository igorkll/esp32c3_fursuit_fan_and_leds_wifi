#include "main.h"
#include "ledstrip.h"
#include "fan.h"
#include "wifi.h"
#include "filesystem.h"

static const char* TAG = "esp32c3_fursuit_fan_and_leds_wifi";

typedef struct {
    bool fan_state;
} config_t;

config_t currentConfig;
static nvs_handle_t nvs;

static const config_t defaultConfig = {
    .fan_state = true
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
        fwrite(&currentConfig, 1, sizeof(device_config), file);
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

static void callback_set_fan(void* user_ctx, char* content, size_t len) {
    currentConfig.fan_state = content[0];
    save_config();
}


static void* callback_get_fan(void* user_ctx, char* content, size_t len, size_t* retsize) {
    *retsize = 1;
    uint8_t* ret = malloc(1);
    ret[0] = currentConfig->fan_state;
    return ret;
}

void app_main() {
    if (!filesystem_init()) {
        ESP_LOGE(TAG, "failed to init filesystem");
        deadlock();
    }
    
    nvs_init();
    load_config();

    ledstrip_init();
    ledstrip_flush();

    fan_init();
    fan_setState(currentConfig.fan_state);

    wifi_init();
    wifi_attachCallback_post("/set_fan", callback_set_fan, NULL);
    wifi_attachCallback_get_binary("/get_fan", callback_get_fan, NULL);
    
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

