/*
 * Moto Audio Unit — Main Application
 * ESP32-S3 + BM62x2 + PCM5102A + TPA6132A2 + Knowles SPM0687
 *
 * See docs/FIRMWARE.md for build instructions
 * See docs/ARCHITECTURE.md for system design
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "driver/uart.h"

#include "audio_mixer.h"
#include "power_manager.h"
#include "ptt_handler.h"

// Component headers (implement in components/)
// #include "bt_a2dp/bt_a2dp.h"
// #include "bm62/bm62.h"
// #include "pcm5102/pcm5102.h"
// #include "tpa6132/tpa6132.h"
// #include "spm0687/spm0687.h"
// #include "ws2812/ws2812.h"
// #include "wired_radio/wired_radio.h"

static const char *TAG = "moto_audio";

// ─── GPIO definitions ────────────────────────────────────────────────────────
#define GPIO_WIRED_PTT_OUT      GPIO_NUM_1   // Open-drain, active low
#define GPIO_WIRED_PTT_IN       GPIO_NUM_2   // Active low, R10 pullup
#define GPIO_BAT_DATA           GPIO_NUM_3   // ADC, NTC via R9 pullup
#define GPIO_BM62A_RST          GPIO_NUM_4   // Active low
#define GPIO_I2S_BCK            GPIO_NUM_5
#define GPIO_I2S_WS             GPIO_NUM_6
#define GPIO_I2S_DOUT           GPIO_NUM_7   // ESP32 → DAC
#define GPIO_I2S_DIN            GPIO_NUM_8   // Mic PDM → ESP32
#define GPIO_PTT_EXT            GPIO_NUM_15  // External PTT jack J7, active low
#define GPIO_ENC_A              GPIO_NUM_16
#define GPIO_ENC_B              GPIO_NUM_17
#define GPIO_ENC_SW             GPIO_NUM_18  // Push to mute
#define GPIO_WIRED_MIC_EN       GPIO_NUM_21  // High = mic enabled to radio
#define GPIO_WS2812             GPIO_NUM_38
#define GPIO_AMP_SD             GPIO_NUM_39  // TPA6132 /SD, low = shutdown
#define GPIO_WIRED_SQU          GPIO_NUM_40  // Squelch detect, active low
#define GPIO_UART0_TX           GPIO_NUM_41  // → BM62-A
#define GPIO_UART0_RX           GPIO_NUM_42  // ← BM62-A
#define GPIO_UART1_TX           GPIO_NUM_43  // → BM62-B
#define GPIO_UART1_RX           GPIO_NUM_44  // ← BM62-B
#define GPIO_BM62B_RST          GPIO_NUM_47
#define GPIO_BM62A_STATUS       GPIO_NUM_48
// #define GPIO_BM62B_STATUS    GPIO_NUM_45

// ─── Event group bits ─────────────────────────────────────────────────────────
#define EVT_PHONE_CONNECTED     BIT0
#define EVT_RADIO_BT_CONNECTED  BIT1
#define EVT_RADIO_RX_ACTIVE     BIT2
#define EVT_RADIO_TX_ACTIVE     BIT3
#define EVT_WIRED_CONNECTED     BIT4
#define EVT_PACK_CONNECTED      BIT5
#define EVT_LOW_KEEPALIVE       BIT6

static EventGroupHandle_t s_events;

// ─── GPIO init ────────────────────────────────────────────────────────────────
static void gpio_init(void)
{
    // Outputs
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << GPIO_WIRED_PTT_OUT) |
                        (1ULL << GPIO_BM62A_RST)     |
                        (1ULL << GPIO_BM62B_RST)     |
                        (1ULL << GPIO_WIRED_MIC_EN)  |
                        (1ULL << GPIO_WS2812)        |
                        (1ULL << GPIO_AMP_SD),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_cfg);

    // PTT OUT is open-drain (drives low to TX, high-Z when idle)
    gpio_set_drive_capability(GPIO_WIRED_PTT_OUT, GPIO_DRIVE_CAP_0);

    // Safe defaults
    gpio_set_level(GPIO_WIRED_PTT_OUT, 1);   // PTT idle (high = not TX)
    gpio_set_level(GPIO_BM62A_RST, 1);        // BM62-A running
    gpio_set_level(GPIO_BM62B_RST, 1);        // BM62-B running
    gpio_set_level(GPIO_WIRED_MIC_EN, 0);     // Mic to radio disabled
    gpio_set_level(GPIO_AMP_SD, 1);           // Amp enabled

    // Inputs with internal pullups
    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << GPIO_WIRED_PTT_IN) |
                        (1ULL << GPIO_PTT_EXT)      |
                        (1ULL << GPIO_ENC_A)        |
                        (1ULL << GPIO_ENC_B)        |
                        (1ULL << GPIO_ENC_SW)       |
                        (1ULL << GPIO_WIRED_SQU)    |
                        (1ULL << GPIO_BM62A_STATUS),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_cfg);
}

// ─── App main ─────────────────────────────────────────────────────────────────
void app_main(void)
{
    ESP_LOGI(TAG, "Moto Audio Unit starting — Rev 2.0");

    // NVS for BT pairing data
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Event group for system state
    s_events = xEventGroupCreate();

    // GPIO
    gpio_init();
    ESP_LOGI(TAG, "GPIO initialized");

    // Power manager (monitors TPS2116 state, keep-alive level)
    power_manager_init(s_events, GPIO_BAT_DATA);
    ESP_LOGI(TAG, "Power manager started");

    // TODO: Initialize components in order:
    //   1. ws2812_init()       — status LED (show "booting" white pulse)
    //   2. spm0687_init()      — MEMS mic PDM driver
    //   3. pcm5102_init()      — DAC I2S driver
    //   4. tpa6132_init()      — amp enable
    //   5. audio_mixer_init()  — mixing engine + duck logic
    //   6. wired_radio_init()  — TRRS interface
    //   7. bm62_init(A)        — BT radio #2 (radio HFP)
    //   8. bm62_init(B)        — BT radio #3 (expansion)
    //   9. bt_a2dp_init()      — phone A2DP sink
    //  10. ptt_handler_init()  — unified PTT logic

    ESP_LOGI(TAG, "All components initialized — entering main loop");

    // Main task: monitor system state and update LED
    while (1) {
        EventBits_t bits = xEventGroupGetBits(s_events);

        // TODO: map event bits to LED color/pattern
        // See docs/ARCHITECTURE.md — LED Status Codes table

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
