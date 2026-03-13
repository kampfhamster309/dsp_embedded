/**
 * @file dsp_power.h
 * @brief Deep-sleep power management (DSP-601).
 *
 * Provides dsp_power_enter_deep_sleep(): saves protocol state to RTC memory,
 * gracefully closes the HTTP server, then enters ESP deep sleep.
 *
 * The entire module is compiled away when CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=0
 * so no symbols appear in always-on (development/debug) firmware images.
 *
 * Typical call site (after a transfer completes):
 *
 *   #if CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX
 *       dsp_power_enter_deep_sleep();   // does not return on ESP
 *   #endif
 *
 * On host builds a mock sleep function can be injected so unit tests can
 * intercept the sleep entry point and verify the state-save sequence:
 *
 *   dsp_power_set_sleep_fn(my_mock, ctx);
 *   dsp_power_enter_deep_sleep();   // calls my_mock(ctx) instead of hardware sleep
 */

#pragma once

#include "dsp_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX

#ifndef ESP_PLATFORM
/**
 * @brief Sleep-entry callback type for host unit tests.
 *
 * Only available on the host build.  Allows tests to intercept the sleep entry
 * point to verify that state has been saved before sleep is triggered.
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
#endif /* !ESP_PLATFORM */

/**
 * @brief Flush protocol state and enter deep sleep.
 *
 * Execution order:
 *   1. dsp_rtc_state_save()  – snapshot neg + xfer tables to RTC memory.
 *   2. dsp_http_stop()       – close the HTTP server gracefully.
 *   3. esp_deep_sleep_start() (ESP) or mock sleep fn (host).
 *
 * On the ESP target this function does not return.
 * On the host build it returns ESP_OK after invoking the mock sleep fn.
 *
 * dsp_neg_init() and dsp_xfer_init() must have been called before this
 * function to ensure the slot tables are in a consistent state.
 *
 * @return ESP_OK (host only — on ESP the function never returns).
 */
esp_err_t dsp_power_enter_deep_sleep(void);

#endif /* CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX */

#ifdef __cplusplus
}
#endif
