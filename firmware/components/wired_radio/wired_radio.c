/*
 * wired_radio.c — TRRS wired radio interface implementation
 */

#include "wired_radio.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "wired_radio";

// Squelch debounce: must see N consecutive samples above/below threshold
#define SQUELCH_DEBOUNCE_COUNT      5
#define MONITOR_TASK_PERIOD_MS      20      // 50Hz polling
#define SQUELCH_HOLDOFF_MS          200     // Min RX duration before reporting end

typedef struct {
    wired_radio_config_t    config;
    wired_radio_event_cb_t  event_cb;
    void                   *user_data;
    wired_radio_state_t     state;
    int                     squelch_count;   // Debounce counter
    TimerHandle_t           rx_holdoff_timer;
    bool                    initialized;
} wired_radio_ctx_t;

static wired_radio_ctx_t s_ctx;

// ─── Squelch holdoff timer ────────────────────────────────────────────────────
static void rx_holdoff_expired(TimerHandle_t timer)
{
    if (s_ctx.state == WIRED_RADIO_STATE_RX) {
        s_ctx.state = WIRED_RADIO_STATE_IDLE;
        ESP_LOGI(TAG, "RX ended");
        if (s_ctx.event_cb) {
            s_ctx.event_cb(WIRED_RADIO_EVT_RX_END, s_ctx.user_data);
        }
    }
}

// ─── Monitor task ─────────────────────────────────────────────────────────────
/*
 * Polls squelch GPIO and PTT sense at 50Hz.
 *
 * Squelch detection strategy:
 *   IO40 is connected to the TRRS jack switch contact.
 *   When a cable is inserted, the switch closes — IO40 goes low.
 *   When the radio squelch opens (audio present), we also see this
 *   via audio level on the RADIO_SPK line.
 *
 *   For now: use IO40 as cable detect. Audio level detection via ADC
 *   provides squelch open/close within the connected state.
 *
 * TODO: Implement proper ADC-based audio level detection.
 *       Current implementation uses IO40 GPIO edge as proxy.
 */
static void wired_radio_monitor_task(void *arg)
{
    bool prev_cable = false;
    bool prev_squelch = false;
    bool prev_ptt_in = false;

    while (1) {
        // Cable detect (IO40 = low when cable inserted)
        bool cable = (gpio_get_level(s_ctx.config.squelch_gpio) == 0);

        if (cable != prev_cable) {
            if (cable) {
                ESP_LOGI(TAG, "Cable connected");
                s_ctx.state = WIRED_RADIO_STATE_IDLE;
                if (s_ctx.event_cb) {
                    s_ctx.event_cb(WIRED_RADIO_EVT_CONNECTED, s_ctx.user_data);
                }
            } else {
                ESP_LOGI(TAG, "Cable disconnected");
                s_ctx.state = WIRED_RADIO_STATE_DISCONNECTED;
                if (s_ctx.event_cb) {
                    s_ctx.event_cb(WIRED_RADIO_EVT_DISCONNECTED, s_ctx.user_data);
                }
            }
            prev_cable = cable;
        }

        if (cable) {
            // TODO: Read ADC for RADIO_SPK level
            // bool squelch_open = (adc_reading > s_ctx.config.squelch_threshold);
            // Placeholder: always false until ADC implemented
            bool squelch_open = false;

            if (squelch_open != prev_squelch) {
                if (squelch_open && s_ctx.state == WIRED_RADIO_STATE_IDLE) {
                    s_ctx.state = WIRED_RADIO_STATE_RX;
                    xTimerStop(s_ctx.rx_holdoff_timer, 0);
                    ESP_LOGI(TAG, "RX started");
                    if (s_ctx.event_cb) {
                        s_ctx.event_cb(WIRED_RADIO_EVT_RX_START, s_ctx.user_data);
                    }
                } else if (!squelch_open && s_ctx.state == WIRED_RADIO_STATE_RX) {
                    // Start holdoff before reporting RX end
                    xTimerStart(s_ctx.rx_holdoff_timer, 0);
                }
                prev_squelch = squelch_open;
            }

            // External PTT sense (IO2, active low)
            bool ptt_pressed = (gpio_get_level(s_ctx.config.ptt_in_gpio) == 0);
            if (ptt_pressed != prev_ptt_in) {
                if (ptt_pressed) {
                    ESP_LOGI(TAG, "PTT sense: TX start");
                    if (s_ctx.event_cb) {
                        s_ctx.event_cb(WIRED_RADIO_EVT_TX_START, s_ctx.user_data);
                    }
                } else {
                    ESP_LOGI(TAG, "PTT sense: TX end");
                    if (s_ctx.event_cb) {
                        s_ctx.event_cb(WIRED_RADIO_EVT_TX_END, s_ctx.user_data);
                    }
                }
                prev_ptt_in = ptt_pressed;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_TASK_PERIOD_MS));
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

esp_err_t wired_radio_init(const wired_radio_config_t *config,
                            wired_radio_event_cb_t cb,
                            void *user_data)
{
    if (!config) return ESP_ERR_INVALID_ARG;

    memcpy(&s_ctx.config, config, sizeof(wired_radio_config_t));
    s_ctx.event_cb  = cb;
    s_ctx.user_data = user_data;
    s_ctx.state     = WIRED_RADIO_STATE_DISCONNECTED;

    // Configure GPIOs
    // Squelch / cable detect — input pullup
    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << config->squelch_gpio) |
                        (1ULL << config->ptt_in_gpio),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_cfg);

    // PTT drive — open-drain output, idle high
    gpio_config_t ptt_cfg = {
        .pin_bit_mask = (1ULL << config->ptt_out_gpio),
        .mode         = GPIO_MODE_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&ptt_cfg);
    gpio_set_level(config->ptt_out_gpio, 1); // Idle high = not TX

    // Mic enable — output, idle low
    gpio_config_t mic_cfg = {
        .pin_bit_mask = (1ULL << config->mic_en_gpio),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&mic_cfg);
    gpio_set_level(config->mic_en_gpio, 0); // Mic off

    // Holdoff timer
    s_ctx.rx_holdoff_timer = xTimerCreate("wired_rx_holdoff",
                                           pdMS_TO_TICKS(SQUELCH_HOLDOFF_MS),
                                           pdFALSE,
                                           NULL,
                                           rx_holdoff_expired);
    if (!s_ctx.rx_holdoff_timer) {
        ESP_LOGE(TAG, "Failed to create holdoff timer");
        return ESP_FAIL;
    }

    // Monitor task
    BaseType_t res = xTaskCreate(wired_radio_monitor_task,
                                  "wired_radio_mon",
                                  2048, NULL, 4, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitor task");
        return ESP_FAIL;
    }

    s_ctx.initialized = true;
    ESP_LOGI(TAG, "Wired radio interface initialized");
    return ESP_OK;
}

esp_err_t wired_radio_tx_start(void)
{
    if (!s_ctx.initialized || !wired_radio_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }
    s_ctx.state = WIRED_RADIO_STATE_TX;
    gpio_set_level(s_ctx.config.ptt_out_gpio, 0);  // Pull PTT low = TX
    gpio_set_level(s_ctx.config.mic_en_gpio, 1);   // Enable mic to radio
    ESP_LOGI(TAG, "TX started");
    return ESP_OK;
}

esp_err_t wired_radio_tx_end(void)
{
    if (!s_ctx.initialized) return ESP_ERR_INVALID_STATE;
    s_ctx.state = WIRED_RADIO_STATE_IDLE;
    gpio_set_level(s_ctx.config.ptt_out_gpio, 1);  // Release PTT
    gpio_set_level(s_ctx.config.mic_en_gpio, 0);   // Disable mic to radio
    ESP_LOGI(TAG, "TX ended");
    return ESP_OK;
}

wired_radio_state_t wired_radio_get_state(void)
{
    return s_ctx.state;
}

bool wired_radio_is_connected(void)
{
    return (s_ctx.state != WIRED_RADIO_STATE_DISCONNECTED);
}
