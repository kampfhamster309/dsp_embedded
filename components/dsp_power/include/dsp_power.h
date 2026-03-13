/**
 * @file dsp_power.h
 * @brief Deep-sleep power management (DSP-601/602).
 *
 * Provides two functions, both behind CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX:
 *
 *   dsp_power_enter_deep_sleep()
 *     Saves protocol state to RTC memory, closes the HTTP server, then
 *     enters ESP deep sleep.  Does not return on hardware.
 *     Typical call site: after a data transfer completes.
 *
 *   dsp_power_handle_wakeup()
 *     Detects the wake cause. On timer wake: restores RTC state, reconnects
 *     WiFi, restarts HTTP server.  On cold boot / other cause: returns
 *     ESP_ERR_INVALID_STATE so app_main can skip the restore path.
 *     Typical call site: near the top of app_main, after neg/xfer init.
 *
 * The entire module compiles away when the flag is 0 — zero symbols in the
 * firmware for always-on (debug/development) builds.
 *
 * Host mock API (not available on ESP):
 *   dsp_power_set_sleep_fn()      — intercept sleep entry in tests.
 *   dsp_power_set_wakeup_cause()  — inject a wake cause for tests.
 */

#pragma once

#include "dsp_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Power-save mode (DSP-603) – always available on ESP, no-op on host
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM
/**
 * @brief Configure the task watchdog and (optionally) esp_pm light sleep.
 *
 * Call once from app_main before starting the application task:
 *   - Reconfigures the ESP Task Watchdog Timer (TWDT) with the timeout from
 *     CONFIG_DSP_WATCHDOG_TIMEOUT_MS and trigger_panic = true.
 *   - If CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP is enabled, calls esp_pm_configure()
 *     to allow the CPU to light-sleep during FreeRTOS idle periods.
 *
 * The function is a no-op on the host build (not declared there).
 *
 * @return ESP_OK on success; non-OK if TWDT or PM configuration fails
 *         (the firmware continues; failures are logged as warnings).
 */
esp_err_t dsp_power_pm_init(void);
#endif /* ESP_PLATFORM */

#if CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX

/* -------------------------------------------------------------------------
 * Wakeup cause abstraction
 * ------------------------------------------------------------------------- */

/**
 * @brief Wakeup cause as seen by dsp_power.
 *
 * Mirrors the relevant esp_sleep_wakeup_cause_t values.  Defined as an
 * independent type so tests can use it on the host without esp_sleep.h.
 */
typedef enum {
    DSP_POWER_WAKEUP_UNDEFINED = 0, /**< Cold boot or unknown cause.         */
    DSP_POWER_WAKEUP_TIMER     = 1, /**< Woken by RTC timer (normal cycle).  */
    DSP_POWER_WAKEUP_OTHER     = 2, /**< EXT0/EXT1, touch, or other cause.   */
} dsp_power_wakeup_cause_t;

/* -------------------------------------------------------------------------
 * Host-only test API
 * ------------------------------------------------------------------------- */

#ifndef ESP_PLATFORM
/**
 * @brief Sleep-entry callback type for host unit tests.
 */
typedef void (*dsp_power_sleep_fn_t)(void *arg);

/**
 * @brief Register a mock sleep function for host unit tests.
 *
 * If set, dsp_power_enter_deep_sleep() calls fn(arg) instead of entering
 * hardware deep sleep.  Pass NULL to reset to the default (no-op on host).
 *
 * @param fn   Callback to invoke at sleep entry, or NULL.
 * @param arg  Opaque argument forwarded to fn.
 */
void dsp_power_set_sleep_fn(dsp_power_sleep_fn_t fn, void *arg);

/**
 * @brief Inject a mock wakeup cause for host unit tests.
 *
 * Defaults to DSP_POWER_WAKEUP_UNDEFINED (cold boot) between tests.
 * Set to DSP_POWER_WAKEUP_TIMER to exercise the restore path.
 *
 * @param cause Wakeup cause to return from the next handle_wakeup() call.
 */
void dsp_power_set_wakeup_cause(dsp_power_wakeup_cause_t cause);
#endif /* !ESP_PLATFORM */

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
 * @brief Flush protocol state and enter deep sleep.
 *
 * Execution order:
 *   1. dsp_rtc_state_save()   – snapshot neg + xfer tables to RTC memory.
 *   2. dsp_http_stop()        – close the HTTP server gracefully.
 *   3. esp_deep_sleep_start() (ESP) or mock sleep fn (host).
 *
 * On the ESP target this function does not return.
 * On the host build it returns ESP_OK after invoking the registered mock fn.
 *
 * dsp_neg_init() and dsp_xfer_init() must have been called before this
 * function to ensure the slot tables are in a consistent state.
 *
 * @return ESP_OK (host only — on ESP the function never returns).
 */
esp_err_t dsp_power_enter_deep_sleep(void);

/**
 * @brief Handle wakeup from deep sleep and restore protocol state.
 *
 * Determines the wake cause:
 *   - DSP_POWER_WAKEUP_TIMER  → normal deep-sleep cycle; restores state.
 *   - DSP_POWER_WAKEUP_UNDEFINED / OTHER → cold boot or unexpected wake;
 *                                           returns immediately without restoring.
 *
 * On a timer wake, execution order:
 *   1. dsp_rtc_state_restore() – recover neg + xfer slot tables from RTC.
 *   2. dsp_wifi_connect()      – trigger WiFi reconnect (non-fatal if fails).
 *   3. dsp_http_start()        – resume HTTP server (non-fatal if fails).
 *
 * Preconditions for a successful restore:
 *   - dsp_neg_init() and dsp_xfer_init() have been called (slot tables ready).
 *   - dsp_wifi_init() has been called (WiFi stack initialised).
 *   - dsp_rtc_state_is_valid() returns true (valid saved state present).
 *
 * @return ESP_OK               – woken by timer and state restored successfully.
 *         ESP_ERR_INVALID_STATE – cold boot / non-timer wake / no valid RTC state.
 *         ESP_ERR_INVALID_CRC  – woken by timer but RTC data is corrupted.
 */
esp_err_t dsp_power_handle_wakeup(void);

#endif /* CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX */

#ifdef __cplusplus
}
#endif
