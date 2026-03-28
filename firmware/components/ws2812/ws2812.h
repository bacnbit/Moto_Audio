/*
 * ws2812.h — WS2812B status LED driver
 *
 * Single WS2812B-2020 LED (D1) on IO38.
 * Indicates system state via color and pattern.
 *
 * LED status codes (see docs/ARCHITECTURE.md for full table):
 *   Blue solid       — Phone connected
 *   Blue slow pulse  — Phone pairing
 *   Green solid      — Radio BT connected
 *   Green slow pulse — Radio BT pairing
 *   Cyan solid       — Phone + Radio both connected
 *   Amber flash      — Radio RX (ducking active)
 *   Red flash        — PTT TX active
 *   Purple solid     — Expansion BT connected
 *   White slow pulse — Booting
 *   Red solid        — Error / critically low battery
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

typedef enum {
    LED_STATE_BOOTING = 0,
    LED_STATE_PHONE_PAIRING,
    LED_STATE_PHONE_CONNECTED,
    LED_STATE_RADIO_PAIRING,
    LED_STATE_RADIO_CONNECTED,
    LED_STATE_BOTH_CONNECTED,
    LED_STATE_RADIO_RX,         // Ducking active
    LED_STATE_TX,               // Transmitting
    LED_STATE_EXPANSION_CONNECTED,
    LED_STATE_ERROR,
    LED_STATE_OFF,
} led_state_t;

esp_err_t ws2812_init(int gpio_num, uint8_t brightness);
void ws2812_set_state(led_state_t state);
void ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b);
