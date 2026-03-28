/*
 * ptt_handler.h — Unified PTT logic
 *
 * Aggregates PTT from all sources and routes to correct radio:
 *
 *   Sources:
 *     1. J7 external PTT jack (IO15, active low, wired handlebar button)
 *     2. Wired radio PTT sense (IO2, from TRRS ring2 passthrough)
 *     3. Future: BM62-B button event (expansion)
 *     4. Future: software PTT from UI
 *
 *   Routing:
 *     - If wired radio cable connected (J3) → TX via wired_radio
 *     - Else if BM62-A connected (BT radio) → TX via bm62_hfp_open_audio
 *     - Else → no-op (nowhere to transmit)
 *
 * Radio priority for TX mirrors RX: wired > BT radio > none.
 */

#pragma once

#include "esp_err.h"
#include "freertos/event_groups.h"

typedef struct {
    int                 ext_ptt_gpio;   // IO15 external PTT jack J7
    EventGroupHandle_t  event_group;
} ptt_handler_config_t;

/**
 * Initialize PTT handler.
 * Starts GPIO monitoring for all PTT sources.
 */
esp_err_t ptt_handler_init(const ptt_handler_config_t *config);

/**
 * Software PTT (for future UI button or BLE remote).
 */
void ptt_handler_sw_press(void);
void ptt_handler_sw_release(void);
