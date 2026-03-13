/**
 * @file dsp_application.c
 * @brief DSP Embedded application task – Core 1 setup (DSP-503).
 */

#include "dsp_application.h"
#include "dsp_config.h"
#include "dsp_shared.h"
#include "esp_err.h"
#include "esp_log.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#endif

static const char *TAG = "dsp_application";

/* Shared state pointer set at start, cleared at stop. */
static dsp_shared_t *s_sh = NULL;

/* True once the init sequence has completed. */
static volatile bool s_running = false;

#ifdef ESP_PLATFORM
static TaskHandle_t s_task_handle = NULL;
#endif

/* =========================================================================
 * ADC stub
 *
 * Returns a deterministic value for each channel so the ring buffer can be
 * verified on host without hardware.  On a real deployment this function
 * would be replaced by the ESP-IDF ADC oneshot driver.
 * ========================================================================= */

static uint32_t adc_stub_read(uint8_t channel)
{
    /* Stub: value scales linearly with channel index.
     * Real ADC: adc_oneshot_read(adc_handle, channel, &raw); */
    return (uint32_t)(channel + 1u) * 1000u;
}

/* =========================================================================
 * Acquisition helpers
 * ========================================================================= */

static void acquire_one_sample(uint8_t channel)
{
    dsp_sample_t sample;
#ifdef ESP_PLATFORM
    sample.timestamp_us = (uint64_t)esp_timer_get_time();
#else
    sample.timestamp_us = 0u;
#endif
    sample.raw_value = adc_stub_read(channel);
    sample.channel   = channel;
    sample._pad[0]   = 0u;
    sample._pad[1]   = 0u;
    sample._pad[2]   = 0u;

    esp_err_t err = dsp_ring_buf_push(s_sh, &sample);
    if (err == ESP_ERR_NO_MEM) {
        /* Ring buffer full – DROP NEWEST policy keeps Core 1 non-blocking. */
        ESP_LOGW(TAG, "ring buffer full, sample on ch%u dropped", (unsigned)channel);
    }
}

/* =========================================================================
 * Initialisation sequence (platform-agnostic)
 * ========================================================================= */

static void application_init(dsp_shared_t *sh)
{
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "application task running on core %d (expected %d)",
             (int)xTaskGetCoreID(NULL), DSP_APPLICATION_TASK_CORE);

    /* Subscribe this task to the Task Watchdog Timer.  The task must call
     * esp_task_wdt_reset() at least once per CONFIG_DSP_WATCHDOG_TIMEOUT_MS
     * milliseconds or the TWDT triggers a panic (DSP-603). */
    esp_err_t wdt_err = esp_task_wdt_add(NULL);
    if (wdt_err != ESP_OK) {
        ESP_LOGW(TAG, "esp_task_wdt_add failed (0x%x) – task not watched",
                 wdt_err);
    } else {
        ESP_LOGI(TAG, "subscribed to TWDT (timeout %d ms)",
                 CONFIG_DSP_WATCHDOG_TIMEOUT_MS);
    }
#endif

    sh->core1_ready = 1;
    s_running = true;

    ESP_LOGI(TAG, "application task ready (poll interval %d ms)",
             CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS);
}

/* =========================================================================
 * ESP-IDF task entry point
 * ========================================================================= */

#ifdef ESP_PLATFORM
static void application_task(void *arg)
{
    dsp_shared_t *sh = (dsp_shared_t *)arg;
    application_init(sh);

    uint8_t channel = 0;
    for (;;) {
        acquire_one_sample(channel);
        channel = (uint8_t)((channel + 1u) % DSP_SAMPLE_CHANNEL_MAX);

        /* Feed the task watchdog once per acquisition cycle. */
        esp_task_wdt_reset();

#if CONFIG_DSP_TEST_WATCHDOG_HANG
        /* DEVICE TEST ONLY: busy-wait without feeding the WDT to verify
         * that the TWDT fires within CONFIG_DSP_WATCHDOG_TIMEOUT_MS ms. */
        ESP_LOGI(TAG, "DSP-603 hang test: blocking acquisition loop "
                      "to trigger TWDT (timeout %d ms) ...",
                 CONFIG_DSP_WATCHDOG_TIMEOUT_MS);
        for (;;) { /* intentional infinite busy-wait – TWDT will fire */ }
#endif

        vTaskDelay(pdMS_TO_TICKS(CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS));
    }
}
#endif /* ESP_PLATFORM */

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t dsp_application_start(dsp_shared_t *sh)
{
    if (!sh) {
        return ESP_ERR_INVALID_ARG;
    }

    s_sh = sh;

#ifdef ESP_PLATFORM
    BaseType_t rc = xTaskCreatePinnedToCore(
        application_task,
        "dsp_application",
        DSP_APPLICATION_TASK_STACK,
        sh,
        DSP_APPLICATION_TASK_PRIO,
        &s_task_handle,
        DSP_APPLICATION_TASK_CORE
    );
    if (rc != pdPASS) {
        s_sh = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
#else
    /* Host: run init and one acquisition cycle synchronously so tests can
     * verify the ring buffer contains at least one sample. */
    application_init(sh);
    acquire_one_sample(0);
    return ESP_OK;
#endif
}

void dsp_application_stop(void)
{
    s_running = false;

    if (s_sh) {
        s_sh->core1_ready = 0;
        s_sh = NULL;
    }

#ifdef ESP_PLATFORM
    if (s_task_handle) {
        /* Unsubscribe from TWDT before deleting the task. */
        esp_task_wdt_delete(s_task_handle);
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }
#endif
}

bool dsp_application_is_running(void)
{
    return s_running;
}
