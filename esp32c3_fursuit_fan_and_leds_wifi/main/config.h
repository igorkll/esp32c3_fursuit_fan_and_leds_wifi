#pragma once
#include "ledstrip_consts.h"

#define PIN_FAN 3
#define PIN_LED 4

#define FAN_ENABLED_LEVEL 0
#define FAN_DISABLED_LEVEL 1

#define DEFAULT_LED_COUNT 16
#define DEFAULT_LIGHT_LEVEL 64
#define MAX_LED_COUNT 32
#define LED_REVERSE false
#define LED_ORDER ledstrip_GRB
#define LED_FREQ 10000000

#define WIFI_NAME "fursuit"
#define WIFI_PASSWORD "00000000"

#define BOOT_BUTTON_GPIO 9

#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_PASSWD_LEN 64

#define APPLY_WIFI_SETTINGS_DELAY 1000
