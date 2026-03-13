/**
 * @file dsp_power.c
 * @brief Deep-sleep power management (DSP-601).
 *
 * This entire translation unit is compiled away when
 * CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=0, leaving no symbols in the binary.
 */

#include "dsp_power.h"

#if CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX

#include "esp_log.h"
#include "dsp_rtc_state.h"
#include "dsp_http.h"

#ifdef ESP_PLATFORM
#include "esp_sleep.h"
#else
/* Host: track the injected mock sleep function. */
static dsp_power_sleep_fn_t s_sleep_fn  = NULL;
static void                *s_sleep_arg = NULL;
#endif

static const char *TAG = "dsp_power";

/* -------------------------------------------------------------------------
 * Host-only test API
 * ------------------------------------------------------------------------- */
#ifndef ESP_PLATFORM
void dsp_power_set_sleep_fn(dsp_power_sleep_fn_t fn, void *arg)
{
    s_sleep_fn  = fn;
    s_sleep_arg = arg;
}
#endif /* !ESP_PLATFORM */

/* -------------------------------------------------------------------------
 * dsp_power_enter_deep_sleep
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

#endif /* CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX */
