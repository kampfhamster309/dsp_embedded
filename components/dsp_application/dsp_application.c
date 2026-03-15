/**
 * @file dsp_application.c
 * @brief DSP Embedded application task – Core 1 setup (DSP-503).
 *
 * Acquires temperature and humidity from the DHT20 sensor (I2C, address 0x38)
 * and pushes samples into the shared ring buffer for Core 0 to consume during
 * active DSP transfers.
 *
 * Ring-buffer sample encoding:
 *   channel = DSP_DHT20_CH_TEMP (0):
 *       raw_value = (uint32_t)(int32_t)(temperature_c * 100)
 *       Decode:    T(°C) = (float)(int32_t)raw_value / 100.0f
 *   channel = DSP_DHT20_CH_HUM (1):
 *       raw_value = (uint32_t)(humidity_pct * 100)
 *       Decode:    RH(%) = (float)raw_value / 100.0f
 *
 * On host builds, dsp_dht20_read() returns fixed stub values (T=21.5 °C,
 * RH=55.0 %) so unit tests exercise the full acquisition path without hardware.
 */

#include "dsp_application.h"
#include "dsp_config.h"
#include "dsp_dht20.h"
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

/* DHT20 driver handle; initialised in application_init, freed in stop. */
static dsp_dht20_handle_t s_dht20 = NULL;

#ifdef ESP_PLATFORM
static TaskHandle_t s_task_handle = NULL;
#endif

/* =========================================================================
 * Acquisition helper
 *
 * Reads one measurement from the DHT20 and pushes two samples:
 *   - channel DSP_DHT20_CH_TEMP: temperature x100 (int32 cast to uint32)
 *   - channel DSP_DHT20_CH_HUM:  humidity x100 (uint32)
 *
 * A failed read (I2C error, CRC mismatch) is logged and skipped; the
 * ring buffer is not written on error to avoid corrupted readings.
 * ========================================================================= */

static void acquire_dht20_samples(void)
{
    dsp_dht20_reading_t reading;
    esp_err_t err = dsp_dht20_read(s_dht20, &reading);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "DHT20 read failed (0x%x) – sample skipped", err);
        return;
    }

    dsp_sample_t sample;
    sample._pad[0] = 0u;
    sample._pad[1] = 0u;
    sample._pad[2] = 0u;

#ifdef ESP_PLATFORM
    sample.timestamp_us = (uint64_t)esp_timer_get_time();
#else
    sample.timestamp_us = 0u;
#endif

    /* Temperature: fixed-point x100, stored as int32 cast to uint32.
     * Consumer decodes: T(oC) = (float)(int32_t)raw_value / 100.0f */
    sample.channel   = DSP_DHT20_CH_TEMP;
    sample.raw_value = (uint32_t)(int32_t)(reading.temperature_c * 100.0f);
    err = dsp_ring_buf_push(s_sh, &sample);
    if (err == ESP_ERR_NO_MEM) {
        ESP_LOGW(TAG, "ring buffer full, temperature sample dropped");
    }

    /* Humidity: fixed-point x100, stored as uint32.
     * Consumer decodes: RH(%) = (float)raw_value / 100.0f */
    sample.channel   = DSP_DHT20_CH_HUM;
    sample.raw_value = (uint32_t)(reading.humidity_pct * 100.0f);
    err = dsp_ring_buf_push(s_sh, &sample);
    if (err == ESP_ERR_NO_MEM) {
        ESP_LOGW(TAG, "ring buffer full, humidity sample dropped");
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

    /* Initialise the DHT20 sensor.  Non-fatal: the task continues without
     * sensor data if the I2C bus cannot be created (e.g. wrong GPIO config). */
    esp_err_t dht_err = dsp_dht20_init(
        CONFIG_DSP_DHT20_SDA_PIN,
        CONFIG_DSP_DHT20_SCL_PIN,
        CONFIG_DSP_DHT20_I2C_PORT,
        &s_dht20);
    if (dht_err != ESP_OK) {
        ESP_LOGE(TAG, "DHT20 init failed (0x%x) – acquisitions will be skipped",
                 dht_err);
    }

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

    for (;;) {
        if (s_dht20) {
            acquire_dht20_samples();
        }

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
    if (s_dht20) {
        acquire_dht20_samples();
    }
    return ESP_OK;
#endif
}

void dsp_application_stop(void)
{
    s_running = false;

    if (s_dht20) {
        dsp_dht20_deinit(s_dht20);
        s_dht20 = NULL;
    }

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
