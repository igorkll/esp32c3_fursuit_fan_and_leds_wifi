#pragma once

#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <esp_system.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

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

    char wifi_ssid[WIFI_MAX_SSID_LEN];
    char wifi_passwd[WIFI_MAX_PASSWD_LEN];
} config_t;

extern config_t currentConfig;
