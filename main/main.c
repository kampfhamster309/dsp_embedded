/**
 * @file main.c
 * @brief DSP Embedded application entry point.
 *
 * Boot sequence (M5 dual-core architecture):
 *   1. USB-CDC reconnect delay
 *   2. dsp_shared_init()             – shared memory between Core 0 and Core 1
 *   3. Optional WiFi provisioning    – writes NVS credentials on first boot
 *   4. WiFi connect + wait for IP
 *   5. dsp_protocol_start()          – Core 0 task: TLS, HTTP, state machines
 *   6. dsp_application_start()       – Core 1 task: ADC polling, ring buffer
 *
 * Heap checkpoints (M1 AC1):
 *   boot / after-shared / after-wifi / after-protocol / after-application
 *
 * M5 AC2 evidence: dsp_protocol logs
 *   "protocol task running on core 0 (expected 0)" at startup.
 * M5 AC3 evidence: dsp_application logs
 *   "application task running on core 1 (expected 1)" at startup.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "dsp_config.h"
#include "dsp_mem.h"
#include "dsp_power.h"
#include "dsp_wifi.h"
#include "dsp_shared.h"
#include "dsp_protocol.h"
#include "dsp_application.h"

static const char *TAG = "dsp_embedded";

#define WIFI_CONNECTED_BIT  BIT0

static EventGroupHandle_t s_wifi_eg;

/* Single instance of the shared state – lives in internal SRAM. */
static dsp_shared_t g_shared;

/* -------------------------------------------------------------------------
 * Periodic memory report (M1 AC1 watermark capture)
 * ------------------------------------------------------------------------- */
static void mem_report_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        dsp_mem_report("periodic");
    }
}

/* -------------------------------------------------------------------------
 * WiFi event callback
 * ------------------------------------------------------------------------- */
static void wifi_event_cb(dsp_wifi_event_t event, void *user_data)
{
    EventGroupHandle_t eg = (EventGroupHandle_t)user_data;
    switch (event) {
        case DSP_WIFI_EVT_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected");
            xEventGroupSetBits(eg, WIFI_CONNECTED_BIT);
            break;
        case DSP_WIFI_EVT_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected");
            break;
        case DSP_WIFI_EVT_RECONNECTING:
            ESP_LOGI(TAG, "WiFi reconnecting...");
            break;
        case DSP_WIFI_EVT_FAILED:
            ESP_LOGE(TAG, "WiFi failed – max retries exceeded");
            break;
    }
}

/* -------------------------------------------------------------------------
 * app_main
 * ------------------------------------------------------------------------- */
void app_main(void)
{
    ESP_LOGI(TAG, "DSP Embedded starting (M5 dual-core build)...");

    /* Allow USB-CDC host to reconnect after re-enumeration. */
    vTaskDelay(pdMS_TO_TICKS(3000));

    dsp_mem_report("boot");

    /* ------------------------------------------------------------------ */
    /* Shared memory                                                       */
    /* ------------------------------------------------------------------ */
    esp_err_t err = dsp_shared_init(&g_shared);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "dsp_shared_init failed: 0x%x", err);
        return;
    }
    dsp_mem_report("after-shared");

    /* Start periodic report task so it captures the TLS handshake watermark. */
    xTaskCreate(mem_report_task, "mem_report", 2048, NULL, 1, NULL);

    /* ------------------------------------------------------------------ */
    /* WiFi                                                                */
    /* ------------------------------------------------------------------ */
    s_wifi_eg = xEventGroupCreate();
    dsp_wifi_set_event_cb(wifi_event_cb, s_wifi_eg);

    /* When provisioning is enabled, pass credentials directly to init so
     * NVS does not need to be pre-populated. After a successful init the
     * credentials are written to NVS for subsequent cold boots. */
#if CONFIG_DSP_WIFI_PROVISION
    {
        dsp_wifi_config_t provision_cfg = {0};
        strncpy(provision_cfg.ssid, CONFIG_DSP_WIFI_PROVISION_SSID,
                sizeof(provision_cfg.ssid) - 1);
        strncpy(provision_cfg.password, CONFIG_DSP_WIFI_PROVISION_PASSWORD,
                sizeof(provision_cfg.password) - 1);
        err = dsp_wifi_init(&provision_cfg);
    }
#else
    err = dsp_wifi_init(NULL);
#endif
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi init failed (0x%x) – continuing without WiFi",
                 err);
    } else {
#if CONFIG_DSP_WIFI_PROVISION
        /* Persist credentials so subsequent boots work without the flag. */
        {
            esp_err_t prov_err = dsp_wifi_store_credentials(
                CONFIG_DSP_WIFI_PROVISION_SSID,
                CONFIG_DSP_WIFI_PROVISION_PASSWORD);
            if (prov_err == ESP_OK) {
                ESP_LOGI(TAG, "WiFi credentials stored (SSID: %s)",
                         CONFIG_DSP_WIFI_PROVISION_SSID);
            } else {
                ESP_LOGW(TAG, "WiFi credentials persist failed: 0x%x", prov_err);
            }
        }
#endif
        dsp_wifi_connect();

        EventBits_t bits = xEventGroupWaitBits(s_wifi_eg, WIFI_CONNECTED_BIT,
                                               pdFALSE, pdFALSE,
                                               pdMS_TO_TICKS(30000));
        if (!(bits & WIFI_CONNECTED_BIT)) {
            ESP_LOGE(TAG, "WiFi did not connect in 30 s");
        }
    }

    dsp_mem_report("after-wifi");

    /* ------------------------------------------------------------------ */
    /* Power management (DSP-603)                                          */
    /* Configure TWDT and optional light sleep before spawning tasks.     */
    /* ------------------------------------------------------------------ */
    dsp_power_pm_init();

    /* ------------------------------------------------------------------ */
    /* Core 0 – protocol stack                                             */
    /*                                                                     */
    /* M5 AC2: log line expected:                                          */
    /*   I (dsp_protocol): protocol task running on core 0 (expected 0)   */
    /* ------------------------------------------------------------------ */
    err = dsp_protocol_start(&g_shared);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "dsp_protocol_start failed: 0x%x", err);
        return;
    }

    /* Give the Core 0 task a moment to log its core ID. */
    vTaskDelay(pdMS_TO_TICKS(200));
    dsp_mem_report("after-protocol");

    /* ------------------------------------------------------------------ */
    /* Core 1 – acquisition task                                           */
    /*                                                                     */
    /* M5 AC3: log line expected:                                          */
    /*   I (dsp_application): application task running on core 1 (expected 1) */
    /* ------------------------------------------------------------------ */
    err = dsp_application_start(&g_shared);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "dsp_application_start failed: 0x%x", err);
        return;
    }

    /* Give Core 1 at least one 100 ms acquisition interval to run. */
    vTaskDelay(pdMS_TO_TICKS(500));
    dsp_mem_report("after-application");

    /* ------------------------------------------------------------------ */
    /* M5 AC3 runtime check – ring buffer must have at least one sample   */
    /* ------------------------------------------------------------------ */
    uint32_t ring_count = dsp_ring_buf_count(&g_shared);
    if (ring_count > 0u) {
        ESP_LOGI(TAG, "M5 AC3 PASS: ring buffer has %lu sample(s) after "
                      "first acquisition cycle", (unsigned long)ring_count);
    } else {
        ESP_LOGE(TAG, "M5 AC3 FAIL: ring buffer empty after application start");
    }

    ESP_LOGI(TAG, "DSP Embedded running on port %d", CONFIG_DSP_HTTP_PORT);
    ESP_LOGI(TAG, "shared.core0_ready=%lu  shared.core1_ready=%lu",
             (unsigned long)g_shared.core0_ready,
             (unsigned long)g_shared.core1_ready);
}
