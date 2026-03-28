/*
 * audio_mixer.h — Core audio mixing and duck logic
 *
 * Mixes phone A2DP audio with radio audio (wired or BT).
 * When radio is active, phone audio is ducked by DUCK_ATTENUATION_DB.
 * After RADIO_SILENCE_TIMEOUT_MS of no radio activity, phone audio
 * fades back in over DUCK_FADE_MS.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

// ─── Configuration ────────────────────────────────────────────────────────────
#define DUCK_ATTENUATION_DB         20      // How much to attenuate phone audio
#define DUCK_FADE_MS                200     // Phone fade-in time after radio ends
#define RADIO_SILENCE_TIMEOUT_MS    5000    // Wait before restoring phone audio
#define SIDETONE_LEVEL_DB          -20      // Mic feedback level in IEM

#define SAMPLE_RATE_HZ              48000
#define AUDIO_BITS                  16
#define AUDIO_CHANNELS              2       // Stereo

// ─── Audio sources ────────────────────────────────────────────────────────────
typedef enum {
    AUDIO_SRC_PHONE = 0,    // A2DP from phone (ESP32 native BT)
    AUDIO_SRC_RADIO_BT,     // HFP from BM62 radio
    AUDIO_SRC_RADIO_WIRED,  // Analog from TRRS jack J3
    AUDIO_SRC_MIC,          // MEMS mic (for monitoring / sidetone)
    AUDIO_SRC_COUNT
} audio_source_t;

// ─── Mixer state machine ──────────────────────────────────────────────────────
typedef enum {
    AUDIO_STATE_MUSIC,       // Phone audio only, radio silent
    AUDIO_STATE_DUCKING,     // Radio detected, fading phone down
    AUDIO_STATE_RADIO,       // Radio active, phone ducked
    AUDIO_STATE_UNDUCKING,   // Radio ended, fading phone back up
} audio_state_t;

// ─── API ──────────────────────────────────────────────────────────────────────

/**
 * Initialize the audio mixer.
 * Must be called after I2S drivers are initialized.
 *
 * @param event_group  System event group for EVT_RADIO_RX_ACTIVE etc
 * @return ESP_OK on success
 */
esp_err_t audio_mixer_init(EventGroupHandle_t event_group);

/**
 * Notify mixer that a radio source has become active or inactive.
 * Called from wired_radio or bm62 component when squelch opens/closes.
 *
 * @param source    Which radio source (RADIO_BT or RADIO_WIRED)
 * @param active    true = radio has audio, false = radio silent
 */
void audio_mixer_set_radio_active(audio_source_t source, bool active);

/**
 * Get current mixer state.
 */
audio_state_t audio_mixer_get_state(void);

/**
 * Set volume (0-100).
 * Applied to IEM output after mixing.
 */
void audio_mixer_set_volume(uint8_t volume_pct);

/**
 * Enable or disable sidetone (mic feedback to IEM).
 */
void audio_mixer_set_sidetone(bool enable);
