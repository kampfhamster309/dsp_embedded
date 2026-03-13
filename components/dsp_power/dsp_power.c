/**
 * @file dsp_power.c
 * @brief Deep-sleep power management (DSP-601/602).
 *
 * This entire translation unit is compiled away when
 * CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=0, leaving no symbols in the binary.
 */

#include "dsp_power.h"

#if CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX

#include "esp_log.h"
#include "dsp_rtc_state.h"
#include "dsp_http.h"
#include "dsp_wifi.h"
#include "dsp_config.h"

#ifdef ESP_PLATFORM
#include "esp_sleep.h"
#else
/* Host: injected mock state. */
static dsp_power_sleep_fn_t     s_sleep_fn     = NULL;
static void                    *s_sleep_arg    = NULL;
static dsp_power_wakeup_cause_t s_wakeup_cause = DSP_POWER_WAKEUP_UNDEFINED;
#endif

static const char *TAG = "dsp_power";

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM
/** Map esp_sleep_wakeup_cause_t to our abstraction. */
static dsp_power_wakeup_cause_t get_wakeup_cause(void)
{
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:     return DSP_POWER_WAKEUP_TIMER;
        case ESP_SLEEP_WAKEUP_UNDEFINED: return DSP_POWER_WAKEUP_UNDEFINED;
        default:                         return DSP_POWER_WAKEUP_OTHER;
    }
}
#else
static dsp_power_wakeup_cause_t get_wakeup_cause(void)
{
    return s_wakeup_cause;
}
#endif

/* -------------------------------------------------------------------------
 * Host-only test API
 * ------------------------------------------------------------------------- */
#ifndef ESP_PLATFORM
void dsp_power_set_sleep_fn(dsp_power_sleep_fn_t fn, void *arg)
{
    s_sleep_fn  = fn;
    s_sleep_arg = arg;
}

void dsp_power_set_wakeup_cause(dsp_power_wakeup_cause_t cause)
{
    s_wakeup_cause = cause;
}
#endif /* !ESP_PLATFORM */

/* -------------------------------------------------------------------------
 * dsp_power_enter_deep_sleep  (DSP-601)
 * ------------------------------------------------------------------------- */
esp_err_t dsp_power_enter_deep_sleep(void)
{
    /* Step 1: flush active negotiation and transfer state to RTC memory so it
     * survives the power-down and can be recovered on the next wake-up. */
    ESP_LOGI(TAG, "saving protocol state to RTC memory");
    dsp_rtc_state_save();

    /* Step 2: close the HTTP server so in-flight connections are terminated
     * cleanly before power is cut to the radio. */
    ESP_LOGI(TAG, "stopping HTTP server");
    dsp_http_stop();

    /* Step 3: enter deep sleep (or invoke mock on host). */
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "entering deep sleep");
    esp_deep_sleep_start();
    /* esp_deep_sleep_start() never returns on hardware. */
    return ESP_OK;
#else
    ESP_LOGI(TAG, "host build: invoking mock sleep fn");
    if (s_sleep_fn != NULL) {
        s_sleep_fn(s_sleep_arg);
    }
    return ESP_OK;
#endif
}

/* -------------------------------------------------------------------------
 * dsp_power_handle_wakeup  (DSP-602)
 * ------------------------------------------------------------------------- */
esp_err_t dsp_power_handle_wakeup(void)
{
    dsp_power_wakeup_cause_t cause = get_wakeup_cause();

    if (cause != DSP_POWER_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "wakeup cause %d — cold boot or non-timer wake, "
                      "skipping state restore", (int)cause);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "timer wakeup detected — restoring protocol state");

    /* Step 1: recover negotiation and transfer slot tables from RTC memory. */
    esp_err_t err = dsp_rtc_state_restore();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "state restore failed: 0x%x", err);
        return err;
    }
    ESP_LOGI(TAG, "protocol state restored successfully");

    /* Step 2: trigger WiFi reconnect.  Non-fatal — the connection manager
     * will retry autonomously.  On the host this always returns ESP_FAIL. */
    err = dsp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi connect returned 0x%x (expected on host, "
                      "retry handled by SM)", err);
    }

    /* Step 3: restart the HTTP server.  Non-fatal — if this fails (e.g. on
     * host where the stub always returns ESP_FAIL) the caller can decide how
     * to recover. */
    err = dsp_http_start(CONFIG_DSP_HTTP_PORT);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP restart returned 0x%x (expected on host)", err);
    }

    return ESP_OK;
}

#endif /* CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX */
