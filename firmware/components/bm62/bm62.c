/*
 * bm62.c — Microchip BM62 Bluetooth module driver implementation
 */

#include "bm62.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "bm62";

#define BM62_UART_BUF_SIZE      512
#define BM62_CMD_TIMEOUT_MS     500
#define BM62_RESET_HOLD_MS      10
#define BM62_BOOT_WAIT_MS       1500
#define BM62_MAX_MODULES        2

// ─── Module state ─────────────────────────────────────────────────────────────
typedef struct {
    bm62_config_t   config;
    bm62_state_t    state;
    char            peer_name[64];
    char            peer_addr[18];  // "XX:XX:XX:XX:XX:XX"
    bool            initialized;
    QueueHandle_t   uart_queue;
    TaskHandle_t    rx_task;
} bm62_ctx_t;

static bm62_ctx_t s_ctx[BM62_MAX_MODULES];

// ─── Internal helpers ─────────────────────────────────────────────────────────

static bm62_ctx_t *get_ctx(bm62_module_t module)
{
    if (module >= BM62_MAX_MODULES) return NULL;
    return &s_ctx[module];
}

static esp_err_t send_cmd(bm62_ctx_t *ctx, const char *cmd)
{
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "AT+AB %s\r\n", cmd);
    int written = uart_write_bytes(ctx->config.uart_port, buf, len);
    if (written != len) {
        ESP_LOGE(TAG, "[M%d] UART write failed: %s", ctx->config.module, cmd);
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "[M%d] CMD: %s", ctx->config.module, cmd);
    return ESP_OK;
}

// ─── UART RX task ─────────────────────────────────────────────────────────────
/*
 * Reads lines from BM62 UART and dispatches events.
 *
 * Key BM62 event strings:
 *   +CONNECTED:<addr>:<profile>
 *   +DISCONNECTED:<addr>
 *   +HFP_AUDIO_CONNECTED
 *   +HFP_AUDIO_DISCONNECTED
 *   +BT_PAIR_REQUEST:<addr>
 *   +VOLUME:<level>
 *   OK
 *   ERROR
 */
static void bm62_rx_task(void *arg)
{
    bm62_ctx_t *ctx = (bm62_ctx_t *)arg;
    uint8_t buf[BM62_UART_BUF_SIZE];
    char line[BM62_UART_BUF_SIZE];
    int line_len = 0;

    ESP_LOGI(TAG, "[M%d] RX task started on UART%d",
             ctx->config.module, ctx->config.uart_port);

    while (1) {
        int len = uart_read_bytes(ctx->config.uart_port, buf,
                                  sizeof(buf) - 1,
                                  pdMS_TO_TICKS(20));
        if (len <= 0) continue;

        for (int i = 0; i < len; i++) {
            char c = (char)buf[i];
            if (c == '\n') {
                line[line_len] = '\0';
                // Trim trailing \r
                if (line_len > 0 && line[line_len - 1] == '\r') {
                    line[--line_len] = '\0';
                }
                if (line_len == 0) { line_len = 0; continue; }

                ESP_LOGD(TAG, "[M%d] RX: %s", ctx->config.module, line);

                // ── Dispatch events ───────────────────────────────────────────
                if (strncmp(line, "+CONNECTED:", 11) == 0) {
                    ctx->state = BM62_STATE_CONNECTED;
                    // Parse addr from line+11
                    strncpy(ctx->peer_addr, line + 11, 17);
                    ctx->peer_addr[17] = '\0';
                    ESP_LOGI(TAG, "[M%d] Connected to %s",
                             ctx->config.module, ctx->peer_addr);
                    if (ctx->config.event_cb) {
                        ctx->config.event_cb(ctx->config.module,
                                             BM62_EVT_CONNECTED,
                                             ctx->peer_addr,
                                             ctx->config.user_data);
                    }

                } else if (strncmp(line, "+DISCONNECTED:", 14) == 0) {
                    ctx->state = BM62_STATE_IDLE;
                    ESP_LOGI(TAG, "[M%d] Disconnected", ctx->config.module);
                    if (ctx->config.event_cb) {
                        ctx->config.event_cb(ctx->config.module,
                                             BM62_EVT_DISCONNECTED,
                                             NULL,
                                             ctx->config.user_data);
                    }

                } else if (strcmp(line, "+HFP_AUDIO_CONNECTED") == 0) {
                    ctx->state = BM62_STATE_HFP_CALL;
                    ESP_LOGI(TAG, "[M%d] HFP audio opened — radio RX/TX",
                             ctx->config.module);
                    if (ctx->config.event_cb) {
                        ctx->config.event_cb(ctx->config.module,
                                             BM62_EVT_HFP_AUDIO_OPEN,
                                             NULL,
                                             ctx->config.user_data);
                    }

                } else if (strcmp(line, "+HFP_AUDIO_DISCONNECTED") == 0) {
                    ctx->state = BM62_STATE_CONNECTED;
                    ESP_LOGI(TAG, "[M%d] HFP audio closed", ctx->config.module);
                    if (ctx->config.event_cb) {
                        ctx->config.event_cb(ctx->config.module,
                                             BM62_EVT_HFP_AUDIO_CLOSE,
                                             NULL,
                                             ctx->config.user_data);
                    }

                } else if (strncmp(line, "+BT_PAIR_REQUEST:", 17) == 0) {
                    ESP_LOGI(TAG, "[M%d] Pair request from %s",
                             ctx->config.module, line + 17);
                    if (ctx->config.event_cb) {
                        ctx->config.event_cb(ctx->config.module,
                                             BM62_EVT_PAIR_REQUEST,
                                             line + 17,
                                             ctx->config.user_data);
                    }
                }
                // else: OK, ERROR, +RSSI, etc — log only

                line_len = 0;
            } else {
                if (line_len < (int)sizeof(line) - 1) {
                    line[line_len++] = c;
                }
            }
        }
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

esp_err_t bm62_init(const bm62_config_t *config)
{
    if (!config || config->module >= BM62_MAX_MODULES) {
        return ESP_ERR_INVALID_ARG;
    }

    bm62_ctx_t *ctx = &s_ctx[config->module];
    memcpy(&ctx->config, config, sizeof(bm62_config_t));
    ctx->state = BM62_STATE_IDLE;
    ctx->initialized = false;

    // Configure UART
    uart_config_t uart_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(config->uart_port, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(config->uart_port,
                                 config->uart_tx_pin,
                                 config->uart_rx_pin,
                                 UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(config->uart_port,
                                        BM62_UART_BUF_SIZE * 2,
                                        BM62_UART_BUF_SIZE * 2,
                                        10,
                                        &ctx->uart_queue,
                                        0));

    // Configure RST pin
    gpio_config_t rst_cfg = {
        .pin_bit_mask = (1ULL << config->rst_pin),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst_cfg);

    // Reset sequence: pull low, wait, release
    ESP_LOGI(TAG, "[M%d] Resetting BM62...", config->module);
    gpio_set_level(config->rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(BM62_RESET_HOLD_MS));
    gpio_set_level(config->rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(BM62_BOOT_WAIT_MS));

    // Start RX task
    char task_name[16];
    snprintf(task_name, sizeof(task_name), "bm62_rx_%d", config->module);
    BaseType_t res = xTaskCreate(bm62_rx_task, task_name,
                                  4096, ctx, 5, &ctx->rx_task);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "[M%d] Failed to create RX task", config->module);
        return ESP_FAIL;
    }

    // Configure BM62 for HFP
    vTaskDelay(pdMS_TO_TICKS(100));
    send_cmd(ctx, "SetHFPFeature 1");     // Enable HFP
    send_cmd(ctx, "SetAutoConnection 1"); // Auto-reconnect to paired device
    send_cmd(ctx, "SetLinkBackDevice 1"); // Remember last device

    ctx->initialized = true;
    ESP_LOGI(TAG, "[M%d] BM62 initialized on UART%d",
             config->module, config->uart_port);
    return ESP_OK;
}

esp_err_t bm62_start_pairing(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    ctx->state = BM62_STATE_PAIRING;
    return send_cmd(ctx, "BtStartPairing");
}

esp_err_t bm62_connect(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    return send_cmd(ctx, "BtConnect");
}

esp_err_t bm62_disconnect(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    return send_cmd(ctx, "BtDisconnect");
}

esp_err_t bm62_hfp_answer(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    return send_cmd(ctx, "BtHfpAnswer");
}

esp_err_t bm62_hfp_open_audio(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    return send_cmd(ctx, "BtHfpOpenAudio");
}

esp_err_t bm62_hfp_close_audio(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    return send_cmd(ctx, "BtHfpCloseAudio");
}

esp_err_t bm62_set_hfp_volume(bm62_module_t module, uint8_t volume)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    if (volume > 15) volume = 15;
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "SetHFPVolume %d", volume);
    return send_cmd(ctx, cmd);
}

bm62_state_t bm62_get_state(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx) return BM62_STATE_IDLE;
    return ctx->state;
}

const char *bm62_get_peer_name(bm62_module_t module)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || ctx->state == BM62_STATE_IDLE) return NULL;
    return ctx->peer_addr; // TODO: request name via AT+AB GetRemoteName
}

esp_err_t bm62_send_raw(bm62_module_t module, const char *cmd)
{
    bm62_ctx_t *ctx = get_ctx(module);
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    return send_cmd(ctx, cmd);
}
