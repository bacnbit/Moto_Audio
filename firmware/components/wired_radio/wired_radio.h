/*
 * wired_radio.h — TRRS wired radio interface (J3)
 *
 * Handles the 3.5mm TRRS radio jack:
 *   Tip   = RADIO_SPK  — audio from radio into device
 *   Ring1 = RADIO_MIC  — audio from device mic to radio
 *   Ring2 = PTT        — active low; ESP32 drives to TX, detects to sense
 *   Sleeve= GND
 *
 * Squelch detection: monitors RADIO_SPK audio level via ADC or
 * digital detect on IO40 (jack switch contact fires on cable insert).
 *
 * PTT drive: IO1 open-drain, pulled low to key radio TX.
 * PTT sense: IO2 input, active low (for external PTT passthrough).
 */

#pragma once

#include "esp_err.h"
#include "freertos/event_groups.h"
#include <stdbool.h>

// ─── Config ───────────────────────────────────────────────────────────────────
typedef struct {
    int             squelch_gpio;       // IO40 — jack switch detect / squelch
    int             ptt_out_gpio;       // IO1  — PTT drive (open-drain, active low)
    int             ptt_in_gpio;        // IO2  — PTT sense (active low)
    int             mic_en_gpio;        // IO21 — mic enable to radio
    int             adc_channel;        // ADC channel on RADIO_SPK for level detect
    uint32_t        squelch_threshold;  // ADC counts above which RX is considered active
    EventGroupHandle_t event_group;     // System event group
} wired_radio_config_t;

// ─── State ────────────────────────────────────────────────────────────────────
typedef enum {
    WIRED_RADIO_STATE_DISCONNECTED = 0, // No cable in J3
    WIRED_RADIO_STATE_IDLE,             // Cable connected, radio silent
    WIRED_RADIO_STATE_RX,               // Radio receiving (squelch open)
    WIRED_RADIO_STATE_TX,               // Device transmitting to radio
} wired_radio_state_t;

// ─── Callback ─────────────────────────────────────────────────────────────────
typedef enum {
    WIRED_RADIO_EVT_CONNECTED,      // Cable inserted
    WIRED_RADIO_EVT_DISCONNECTED,   // Cable removed
    WIRED_RADIO_EVT_RX_START,       // Radio squelch opened — trigger duck
    WIRED_RADIO_EVT_RX_END,         // Radio squelch closed — start timer
    WIRED_RADIO_EVT_TX_START,       // PTT pressed
    WIRED_RADIO_EVT_TX_END,         // PTT released
} wired_radio_event_t;

typedef void (*wired_radio_event_cb_t)(wired_radio_event_t event,
                                        void *user_data);

// ─── API ──────────────────────────────────────────────────────────────────────

/**
 * Initialize wired radio interface.
 * Configures GPIO, starts monitoring task.
 */
esp_err_t wired_radio_init(const wired_radio_config_t *config,
                            wired_radio_event_cb_t cb,
                            void *user_data);

/**
 * Start transmitting to radio (pull PTT low, enable mic).
 * Called by ptt_handler when any PTT source fires.
 */
esp_err_t wired_radio_tx_start(void);

/**
 * Stop transmitting (release PTT, disable mic).
 */
esp_err_t wired_radio_tx_end(void);

/**
 * Get current wired radio state.
 */
wired_radio_state_t wired_radio_get_state(void);

/**
 * Check if a cable is currently connected to J3.
 */
bool wired_radio_is_connected(void);
