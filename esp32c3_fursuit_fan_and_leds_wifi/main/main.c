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
    uint8_t other_colors[MAX_LED_COUNT * 3];
    bool other_colors_overwrite[MAX_LED_COUNT];

    bool separate_colors_enabled;

    uint16_t en_fan_time;
    uint16_t dis_fan_time;
} config_t;

config_t currentConfig;
static nvs_handle_t nvs;

static const config_t defaultConfig = {
    .led_count = DEFAULT_LED_COUNT,
    .led_light = DEFAULT_LIGHT_LEVEL,
    .led_state = true,
    .fan_state = Fan_state_off,
    .separate_colors_enabled = true,

    .en_fan_time = 10,
    .dis_fan_time = 30
};

static void nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));
}

static void wait(float milliseconds) {
    uint32_t ticks = milliseconds / portTICK_PERIOD_MS;
    if (ticks < 1) ticks = 1;
    vTaskDelay(ticks);
}

static int atoi_with_limit(const char* str, int max_length) {
    long result = 0;
    int sign = 1;
    int i = 0;

    if (str[i] == '-' || str[i] == '+') {
        sign = (str[i] == '-') ? -1 : 1;
        i++;
    }

    for (int length = 0; length < max_length && str[i] >= '0' && str[i] <= '9'; i++, length++) {
        result = result * 10 + (str[i] - '0');

        if (result * sign > INT_MAX) {
            return INT_MAX;
        }
        if (result * sign < INT_MIN) {
            return INT_MIN;
        }
    }

    return (int)(result * sign);
}

static void load_config() {
    ESP_LOGI(TAG, "loading config...");
    FILE* file = fopen("/storage/config", "rb");
    if (file) {
        fread(&currentConfig, 1, sizeof(config_t), file);
        fclose(file);
        ESP_LOGI(TAG, "config loaded!");
    } else {
        memset(&currentConfig, 0, sizeof(config_t));
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

static void deadlock() {
    while (true) wait(1000);
}

static int32_t old_en_fan_time = -1;
static int32_t old_dis_fan_time = -1;

static TaskHandle_t fan_task_handle = NULL;

static void fan_task(void *pvParameters) {
    while (true) {
        fan_setState(true);
        wait(currentConfig.en_fan_time * 1000);
        fan_setState(false);
        wait(currentConfig.dis_fan_time * 1000);
    }
}

static void disable_fan_timer() {
    ESP_LOGI(TAG, "disable_fan_timer");

    old_en_fan_time = -1;
    old_dis_fan_time = -1;

    if (fan_task_handle) {
        vTaskDelete(fan_task_handle);
        fan_task_handle = NULL;
    }
}

static void enable_fan_timer() {
    if (fan_task_handle) disable_fan_timer();
    ESP_LOGI(TAG, "enable_fan_timer");

    xTaskCreate(
        fan_task,
        NULL,
        4096,
        NULL,
        1,
        &fan_task_handle
    );
}

static void update_fan_state() {
    ESP_LOGI(TAG, "update fan state");

    switch (currentConfig.fan_state) {
        case Fan_state_off:
            ESP_LOGI(TAG, "update fan: off");
            disable_fan_timer();
            fan_setState(false);
            break;

        case Fan_state_on:
            ESP_LOGI(TAG, "update fan: on");
            disable_fan_timer();
            fan_setState(true);
            break;

        case Fan_state_timer:
            ESP_LOGI(TAG, "update fan: timer");
            if (currentConfig.en_fan_time != old_en_fan_time ||
                currentConfig.dis_fan_time != old_dis_fan_time) {
                enable_fan_timer();

                old_en_fan_time = currentConfig.en_fan_time;
                old_dis_fan_time = currentConfig.dis_fan_time;
            }
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

        if (currentConfig.separate_colors_enabled) {
            for (size_t i = 0; i < currentConfig.led_count; i++) {
                if (currentConfig.other_colors_overwrite[i]) {
                    size_t ii = i * 3;
                    ledstrip_set(i, currentConfig.other_colors[ii], currentConfig.other_colors[ii + 1], currentConfig.other_colors[ii + 2]);
                }
            }
        }
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
    save_config();
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

static void callback_set_led_count_and_save(void* user_ctx, char* content, size_t len) {
    currentConfig.led_count = content[0];
    ESP_LOGI(TAG, "set led count: %i, %i", currentConfig.led_count, MAX_LED_COUNT);

    if (currentConfig.led_count > MAX_LED_COUNT) currentConfig.led_count = MAX_LED_COUNT;

    for (size_t i = currentConfig.led_count; i < MAX_LED_COUNT; i++) {
        currentConfig.other_colors_overwrite[i] = false;
        
        size_t ii = i * 3;
        currentConfig.other_colors[ii] = 0;
        currentConfig.other_colors[ii+1] = 0;
        currentConfig.other_colors[ii+2] = 0;
    }

    save_config();
    update_ledstrip_state();
}

static void* callback_get_led_count(void* user_ctx, char* content, size_t len, size_t* retsize) {
    ESP_LOGI(TAG, "get led count: %i, %i", currentConfig.led_count, MAX_LED_COUNT);
    *retsize = 2;
    uint8_t* ret = malloc(2);
    ret[0] = currentConfig.led_count;
    ret[1] = MAX_LED_COUNT;
    return ret;
}

static void callback_set_separate_enabled(void* user_ctx, char* content, size_t len) {
    currentConfig.separate_colors_enabled = content[0];
    ESP_LOGI(TAG, "set separate enabled: %i", currentConfig.separate_colors_enabled);
    save_config();
    update_ledstrip_state();
}

static void* callback_get_separate_enabled(void* user_ctx, char* content, size_t len, size_t* retsize) {
    ESP_LOGI(TAG, "get separate enabled: %i", currentConfig.separate_colors_enabled);
    *retsize = 1;
    uint8_t* ret = malloc(1);
    ret[0] = currentConfig.separate_colors_enabled;
    return ret;
}

static void callback_set_en_fan_and_save(void* user_ctx, char* content, size_t len) {
    currentConfig.en_fan_time = atoi_with_limit(content, len);
    ESP_LOGI(TAG, "set en fan: %i", currentConfig.en_fan_time);
    save_config();
    update_fan_state();
}

static void callback_set_dis_fan_and_save(void* user_ctx, char* content, size_t len) {
    currentConfig.dis_fan_time = atoi_with_limit(content, len);
    ESP_LOGI(TAG, "set dis fan: %i", currentConfig.dis_fan_time);
    save_config();
    update_fan_state();
}

static void* callback_get_en_fan(void* user_ctx, char* content, size_t len, size_t* retsize) {
    ESP_LOGI(TAG, "get en fan: %i", currentConfig.en_fan_time);
    char* ret = malloc(17);
    itoa(currentConfig.en_fan_time, ret, 10);
    *retsize = strlen(ret);
    return ret;
}

static void* callback_get_dis_fan(void* user_ctx, char* content, size_t len, size_t* retsize) {
    ESP_LOGI(TAG, "get dis fan: %i", currentConfig.dis_fan_time);
    char* ret = malloc(17);
    itoa(currentConfig.dis_fan_time, ret, 10);
    *retsize = strlen(ret);
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

    wifi_attachCallback_post("/set_led_count_and_save", callback_set_led_count_and_save, NULL);
    wifi_attachCallback_get_binary("/get_led_count", callback_get_led_count, NULL);

    wifi_attachCallback_post("/set_separate_enabled", callback_set_separate_enabled, NULL);
    wifi_attachCallback_get_binary("/get_separate_enabled", callback_get_separate_enabled, NULL);

    wifi_attachCallback_post("/set_en_fan_and_save", callback_set_en_fan_and_save, NULL);
    wifi_attachCallback_post("/set_dis_fan_and_save", callback_set_dis_fan_and_save, NULL);
    wifi_attachCallback_get("/get_en_fan", callback_get_en_fan, NULL);
    wifi_attachCallback_get("/get_dis_fan", callback_get_dis_fan, NULL);
    
}

