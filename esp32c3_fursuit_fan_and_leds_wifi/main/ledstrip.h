#pragma once
#include "main.h"
#include "ledstrip_consts.h"

void ledstrip_init();
void ledstrip_setLight(uint8_t light);
void ledstrip_clear(uint8_t r, uint8_t g, uint8_t b);
void ledstrip_set(size_t i, uint8_t r, uint8_t g, uint8_t b);
void ledstrip_flush();