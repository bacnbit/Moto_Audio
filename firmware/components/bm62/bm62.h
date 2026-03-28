/*
 * bm62.h — Microchip BM62 Bluetooth module driver
 *
 * Controls two BM62 modules via independent UART ports:
 *   BM62-A (U2): BT Radio #2 — pairs to UV-PRO or any HFP radio
 *   BM62-B (U3): BT Radio #3 — expansion (BS-PTT, intercom, 2nd phone)
 *
 * The BM62 is controlled via ASCII AT-style commands at 115200 8N1.
 * Audio flows via shared I2S bus — BM62 is I2S slave, ESP32 is master.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// ─── Module identifiers ───────────────────────────────────────────────────────
typedef enum {
    BM62_MODULE_A = 0,  // U2 — radio HFP
    BM62_MODULE_B = 1,  // U3 — expansion
} bm62_module_t;

// ─── Connection state ─────────────────────────────────────────────────────────
typedef enum {
    BM62_STATE_IDLE = 0,
    BM62_STATE_PAIRING,
    BM62_STATE_CONNECTED,
    BM62_STATE_HFP_CALL,        // HFP audio channel open (radio RX/TX)
} bm62_state_t;

// ─── Profile flags ────────────────────────────────────────────────────────────
#define BM62_PROFILE_HFP    BIT0
#define BM62_PROFILE_A2DP   BIT1
#define BM62_PROFILE_AVRCP  BIT2

// ─── Event callback ───────────────────────────────────────────────────────────
typedef enum {
    BM62_EVT_CONNECTED,
    BM62_EVT_DISCONNECTED,
    BM62_EVT_HFP_AUDIO_OPEN,    // Radio RX started — trigger duck
    BM62_EVT_HFP_AUDIO_CLOSE,   // Radio RX ended — start silence timer
    BM62_EVT_PAIR_REQUEST,      // Remote device requesting pairing
} bm62_event_t;

typedef void (*bm62_event_cb_t)(bm62_module_t module, bm62_event_t event,
                                 void *event_data, void *user_data);

// ─── Config ───────────────────────────────────────────────────────────────────
typedef struct {
    bm62_module_t   module;
    uart_port_t     uart_port;
    int             uart_tx_pin;
    int             uart_rx_pin;
    int             rst_pin;        // Active low reset
    int             status_pin;     // BT_STATUS output from BM62
    bm62_event_cb_t event_cb;
    void           *user_data;
} bm62_config_t;

// ─── API ──────────────────────────────────────────────────────────────────────

/**
 * Initialize a BM62 module.
 * Resets the module, starts UART RX task, configures for HFP.
 */
esp_err_t bm62_init(const bm62_config_t *config);

/**
 * Enter Bluetooth pairing mode.
 * Module will be discoverable for 60 seconds.
 */
esp_err_t bm62_start_pairing(bm62_module_t module);

/**
 * Connect to last paired device.
 */
esp_err_t bm62_connect(bm62_module_t module);

/**
 * Disconnect from current device.
 */
esp_err_t bm62_disconnect(bm62_module_t module);

/**
 * Accept incoming HFP audio connection (answer the "call").
 * Call this when BM62_EVT_PAIR_REQUEST is received for HFP.
 */
esp_err_t bm62_hfp_answer(bm62_module_t module);

/**
 * Open HFP audio channel for TX (start transmitting to radio).
 * Equivalent to pressing PTT on a phone paired to the radio.
 */
esp_err_t bm62_hfp_open_audio(bm62_module_t module);

/**
 * Close HFP audio channel (release PTT).
 */
esp_err_t bm62_hfp_close_audio(bm62_module_t module);

/**
 * Set HFP speaker volume (0-15, BM62 native scale).
 */
esp_err_t bm62_set_hfp_volume(bm62_module_t module, uint8_t volume);

/**
 * Get current module state.
 */
bm62_state_t bm62_get_state(bm62_module_t module);

/**
 * Get human-readable name of paired device (if connected).
 * Returns NULL if not connected.
 */
const char *bm62_get_peer_name(bm62_module_t module);

/**
 * Send raw AT command to module (for debugging / advanced use).
 * Response is delivered via event callback.
 */
esp_err_t bm62_send_raw(bm62_module_t module, const char *cmd);
