/*
 * power_manager.h — TPS2116 hot-swap state + keep-alive monitoring
 *
 * Monitors:
 *   - Main pack presence (TPS2116 ST pin, or voltage on BAT_DATA)
 *   - Keep-alive cell voltage (ADC on BAT_DATA / IO3 via R9 divider)
 *   - Pack NTC temperature (ADC on IO3 DATA pin from battery pogo)
 *
 * The TPS2116 handles hardware power mux autonomously (<1µs switchover).
 * This component handles software-level awareness: logging, LED updates,
 * user alerts for low keep-alive cell.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// ─── Voltage thresholds ───────────────────────────────────────────────────────
#define PACK_VOLTAGE_MIN_MV         3200    // Below this = pack critically low
#define KEEPALIVE_LOW_MV            3400    // Below this = warn user
#define KEEPALIVE_CRITICAL_MV       3100    // Below this = imminent shutdown
#define NTC_TEMP_MAX_C              60      // Above this = warn (pack too hot)

// ─── Power source ─────────────────────────────────────────────────────────────
typedef enum {
    POWER_SOURCE_UNKNOWN = 0,
    POWER_SOURCE_MAIN_PACK,     // TPS2116 using IN1 (main pack via pogo)
    POWER_SOURCE_KEEPALIVE,     // TPS2116 using IN2 (keep-alive cell)
    POWER_SOURCE_USB,           // USB-C connected, pack may or may not be present
} power_source_t;

// ─── API ──────────────────────────────────────────────────────────────────────

/**
 * Initialize power manager.
 * @param event_group   System event group (for EVT_PACK_CONNECTED, EVT_LOW_KEEPALIVE)
 * @param bat_data_gpio IO3 — ADC input for NTC / keep-alive voltage
 */
esp_err_t power_manager_init(EventGroupHandle_t event_group, int bat_data_gpio);

/**
 * Get current power source.
 */
power_source_t power_manager_get_source(void);

/**
 * Get estimated keep-alive cell voltage in mV.
 * Returns 0 if not measurable.
 */
uint32_t power_manager_get_keepalive_mv(void);

/**
 * Get pack NTC temperature in degrees C.
 * Returns -1 if no pack connected or NTC not readable.
 */
int8_t power_manager_get_pack_temp_c(void);

/**
 * Check if a main battery pack is currently connected via pogo contacts.
 */
bool power_manager_is_pack_connected(void);
