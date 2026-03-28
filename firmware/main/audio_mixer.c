/*
 * audio_mixer.c — Core audio mixing and duck logic implementation
 *
 * State machine:
 *   MUSIC → (radio detect) → DUCKING → (fade complete) → RADIO
 *   RADIO → (silence) → UNDUCKING → (fade complete) → MUSIC
 *
 * The mixer runs as a FreeRTOS task consuming audio from multiple
 * I2S sources and producing a single mixed output to the DAC.
 *
 * Audio sources:
 *   - Phone A2DP: decoded PCM from ESP32 BT stack ring buffer
 *   - BM62 radio HFP: I2S from BM62 module (shared I2S bus)
 *   - Wired radio: analog in on TRRS tip, ADC-sampled
 *
 * Output:
 *   - I2S to PCM5102A DAC → TPA6132A2 amp → IEM
 *   - Mic sidetone mixed at SIDETONE_LEVEL_DB below full scale
 */

#include "audio_mixer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/ringbuf.h"
#include <math.h>
#include <string.h>

static const char *TAG = "audio_mixer";

// ─── Internal state ───────────────────────────────────────────────────────────
typedef struct {
    audio_state_t       state;
    EventGroupHandle_t  event_group;

    // Volume control (0.0 to 1.0 linear scale)
    float   phone_gain;         // Current phone gain (ducked or full)
    float   phone_gain_target;  // Target gain (smooth fade toward this)
    float   radio_gain;         // Radio always at 1.0 unless muted
    float   master_volume;      // User volume control (0.0 to 1.0)
    bool    sidetone_enabled;

    // Duck timer
    TimerHandle_t   silence_timer;
    bool            radio_bt_active;
    bool            radio_wired_active;

    // Stats
    uint32_t    duck_count;
    uint32_t    total_radio_rx_ms;

} mixer_ctx_t;

static mixer_ctx_t s_ctx;

// ─── dB to linear conversion ──────────────────────────────────────────────────
static inline float db_to_linear(float db)
{
    return powf(10.0f, db / 20.0f);
}

// ─── Silence timer callback ───────────────────────────────────────────────────
static void silence_timer_cb(TimerHandle_t timer)
{
    if (s_ctx.state == AUDIO_STATE_RADIO) {
        ESP_LOGI(TAG, "Silence timeout — restoring phone audio");
        s_ctx.state = AUDIO_STATE_UNDUCKING;
        s_ctx.phone_gain_target = 1.0f;  // Fade phone back up
    }
}

// ─── Check if any radio source is active ─────────────────────────────────────
static bool any_radio_active(void)
{
    return s_ctx.radio_bt_active || s_ctx.radio_wired_active;
}

// ─── Mixer task ───────────────────────────────────────────────────────────────
/*
 * Runs at audio rate, mixing sources and writing to DAC I2S.
 *
 * Per-sample processing (48kHz, 16-bit stereo):
 *   1. Read phone PCM from A2DP ring buffer
 *   2. Read radio PCM from I2S (BM62 or wired ADC)
 *   3. Apply gain to phone based on duck state
 *   4. Mix: output = (phone * phone_gain) + (radio * radio_gain)
 *   5. Apply master volume
 *   6. Add sidetone: output += mic_sample * sidetone_gain
 *   7. Write to DAC I2S output
 *
 * Gain smoothing: per-sample step toward target to avoid clicks.
 * At 48kHz, 50ms fade = 2400 samples, step = (1.0 - duck) / 2400
 */
#define MIXER_FRAME_SAMPLES     256     // Samples per processing frame
#define FADE_STEP_UP            (1.0f / (SAMPLE_RATE_HZ * DUCK_FADE_MS / 1000))
#define FADE_STEP_DOWN          (1.0f / (SAMPLE_RATE_HZ * 50 / 1000))  // 50ms duck

static void audio_mixer_task(void *arg)
{
    int16_t phone_buf[MIXER_FRAME_SAMPLES * 2];   // Stereo
    int16_t radio_buf[MIXER_FRAME_SAMPLES * 2];
    int16_t out_buf[MIXER_FRAME_SAMPLES * 2];

    ESP_LOGI(TAG, "Mixer task started at %dHz %d-bit stereo",
             SAMPLE_RATE_HZ, AUDIO_BITS);

    while (1) {
        // TODO: Read from actual I2S / ring buffer sources
        // For now: silence (zeros)
        memset(phone_buf, 0, sizeof(phone_buf));
        memset(radio_buf, 0, sizeof(radio_buf));

        // ── Gain smoothing ────────────────────────────────────────────────────
        float gain_diff = s_ctx.phone_gain_target - s_ctx.phone_gain;
        if (gain_diff > 0) {
            s_ctx.phone_gain += FADE_STEP_UP;
            if (s_ctx.phone_gain > s_ctx.phone_gain_target) {
                s_ctx.phone_gain = s_ctx.phone_gain_target;
                if (s_ctx.state == AUDIO_STATE_UNDUCKING) {
                    s_ctx.state = AUDIO_STATE_MUSIC;
                    ESP_LOGI(TAG, "→ MUSIC");
                }
            }
        } else if (gain_diff < 0) {
            s_ctx.phone_gain -= FADE_STEP_DOWN;
            if (s_ctx.phone_gain < s_ctx.phone_gain_target) {
                s_ctx.phone_gain = s_ctx.phone_gain_target;
                if (s_ctx.state == AUDIO_STATE_DUCKING) {
                    s_ctx.state = AUDIO_STATE_RADIO;
                    ESP_LOGI(TAG, "→ RADIO");
                }
            }
        }

        // ── Mix samples ───────────────────────────────────────────────────────
        float duck_linear = db_to_linear(-(float)DUCK_ATTENUATION_DB);
        float effective_phone_gain = s_ctx.phone_gain * s_ctx.master_volume;

        for (int i = 0; i < MIXER_FRAME_SAMPLES * 2; i++) {
            float phone  = (float)phone_buf[i] * effective_phone_gain;
            float radio  = (float)radio_buf[i] * s_ctx.radio_gain * s_ctx.master_volume;
            float mixed  = phone + radio;

            // Soft clip to prevent overflow
            if (mixed >  32767.0f) mixed =  32767.0f;
            if (mixed < -32768.0f) mixed = -32768.0f;

            out_buf[i] = (int16_t)mixed;
        }

        // TODO: Write out_buf to DAC I2S handle
        // size_t written;
        // i2s_channel_write(dac_tx_handle, out_buf, sizeof(out_buf),
        //                   &written, portMAX_DELAY);

        // Yield to avoid starving other tasks if I2S blocks
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

esp_err_t audio_mixer_init(EventGroupHandle_t event_group)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.event_group       = event_group;
    s_ctx.state             = AUDIO_STATE_MUSIC;
    s_ctx.phone_gain        = 1.0f;
    s_ctx.phone_gain_target = 1.0f;
    s_ctx.radio_gain        = 1.0f;
    s_ctx.master_volume     = 0.8f;  // 80% default
    s_ctx.sidetone_enabled  = true;

    s_ctx.silence_timer = xTimerCreate(
        "audio_silence",
        pdMS_TO_TICKS(RADIO_SILENCE_TIMEOUT_MS),
        pdFALSE,        // One-shot
        NULL,
        silence_timer_cb);

    if (!s_ctx.silence_timer) {
        ESP_LOGE(TAG, "Failed to create silence timer");
        return ESP_FAIL;
    }

    BaseType_t res = xTaskCreate(audio_mixer_task, "audio_mixer",
                                  8192, NULL, 10, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create mixer task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Audio mixer initialized");
    return ESP_OK;
}

void audio_mixer_set_radio_active(audio_source_t source, bool active)
{
    bool was_active = any_radio_active();

    if (source == AUDIO_SRC_RADIO_BT) {
        s_ctx.radio_bt_active = active;
    } else if (source == AUDIO_SRC_RADIO_WIRED) {
        s_ctx.radio_wired_active = active;
    }

    bool now_active = any_radio_active();

    if (!was_active && now_active) {
        // Radio just became active — duck phone
        ESP_LOGI(TAG, "Radio RX detected — ducking phone (state: DUCKING)");
        xTimerStop(s_ctx.silence_timer, 0);
        s_ctx.state = AUDIO_STATE_DUCKING;
        s_ctx.phone_gain_target = db_to_linear(-(float)DUCK_ATTENUATION_DB);
        s_ctx.duck_count++;

    } else if (was_active && !now_active) {
        // Radio just went silent — start silence timer
        ESP_LOGI(TAG, "Radio RX ended — starting %dms silence timer",
                 RADIO_SILENCE_TIMEOUT_MS);
        xTimerReset(s_ctx.silence_timer, 0);
    }
}

audio_state_t audio_mixer_get_state(void)
{
    return s_ctx.state;
}

void audio_mixer_set_volume(uint8_t volume_pct)
{
    if (volume_pct > 100) volume_pct = 100;
    s_ctx.master_volume = (float)volume_pct / 100.0f;
    ESP_LOGI(TAG, "Volume: %d%%", volume_pct);
}

void audio_mixer_set_sidetone(bool enable)
{
    s_ctx.sidetone_enabled = enable;
    ESP_LOGI(TAG, "Sidetone: %s", enable ? "on" : "off");
}
