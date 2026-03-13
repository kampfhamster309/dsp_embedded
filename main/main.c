#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "dsp_config.h"
#include "dsp_mem.h"
#include "dsp_identity.h"
#include "dsp_tls.h"
#include "dsp_http.h"
#include "dsp_wifi.h"

static const char *TAG = "dsp_embedded";

/* WiFi-ready event bit */
#define WIFI_CONNECTED_BIT  BIT0

static EventGroupHandle_t s_wifi_eg;

/*
 * TLS server context – statically allocated so its footprint shows up
 * clearly in the before/after heap snapshots rather than adding allocator
 * overhead.
 */
static dsp_tls_ctx_t s_tls_ctx;

/* ---------------------------------------------------------------------- */
/* Periodic memory report task                                             */
/*                                                                         */
/* Fires every 30 s.  The min_free field in each report is a watermark    */
/* maintained by the heap allocator since boot, so it captures the lowest  */
/* free-heap point reached during any TLS handshake.                       */
/*                                                                         */
/* AC1 check:                                                              */
/*   mbedTLS peak consumption ≈ boot.free_internal − min(periodic.min_free)*/
/*   This value must be ≤ 50 KB (51200 bytes).                             */
/* ---------------------------------------------------------------------- */
static void mem_report_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        dsp_mem_report("periodic");
    }
}

static void wifi_event_cb(dsp_wifi_event_t event, void *user_data)
{
    EventGroupHandle_t eg = (EventGroupHandle_t)user_data;
    switch (event) {
        case DSP_WIFI_EVT_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected – IP ready");
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

void app_main(void)
{
    ESP_LOGI(TAG, "DSP Embedded starting...");

    /*
     * Give the USB-CDC host 3 s to reconnect after re-enumeration so the
     * boot-time dsp_mem_report() lines are not lost.  Remove this delay
     * in production builds.
     */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* ------------------------------------------------------------------ */
    /* Measurement 1 – boot baseline                                       */
    /* ------------------------------------------------------------------ */
    dsp_mem_report("boot");

    /* Load device certificate and key from flash */
    if (dsp_identity_init() != ESP_OK) {
        ESP_LOGE(TAG, "Identity init failed – run tools/gen_dev_certs.sh");
    }

    /* ------------------------------------------------------------------ */
    /* Measurement 2 – after cert/key parsed into RAM                     */
    /* ------------------------------------------------------------------ */
    dsp_mem_report("after-identity");

    /* Initialise TLS server context: allocates mbedTLS conf, x509_crt,   */
    /* pk_context, entropy_context, ctr_drbg_context, and (if enabled)    */
    /* ssl_ticket_context.  This is the dominant static mbedTLS footprint. */
    size_t cert_len = 0, key_len = 0;
    const uint8_t *cert = dsp_identity_get_cert(&cert_len);
    const uint8_t *key  = dsp_identity_get_key(&key_len);

    if (!cert || !key) {
        ESP_LOGE(TAG, "No device credentials – run tools/gen_dev_certs.sh");
    } else {
        esp_err_t err = dsp_tls_server_init(&s_tls_ctx,
                                            cert, cert_len,
                                            key,  key_len,
                                            "dsp_tls");
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "TLS server init failed: 0x%x", err);
        }
    }

    /* ------------------------------------------------------------------ */
    /* Measurement 3 – after TLS server context fully initialised         */
    /* delta(boot → after-tls-init) = static mbedTLS allocation           */
    /* This must be << 50 KB to leave room for per-connection SSL context  */
    /* ------------------------------------------------------------------ */
    dsp_mem_report("after-tls-init");

    ESP_LOGI(TAG, "Build config: catalog_request=%d consumer=%d daps_shim=%d "
                  "tls_tickets=%d psram=%d deep_sleep=%d terminate=%d "
                  "max_neg=%d max_tx=%d port=%d verbose=%d",
             CONFIG_DSP_ENABLE_CATALOG_REQUEST,
             CONFIG_DSP_ENABLE_CONSUMER,
             CONFIG_DSP_ENABLE_DAPS_SHIM,
             CONFIG_DSP_TLS_SESSION_TICKETS,
             CONFIG_DSP_PSRAM_ENABLE,
             CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX,
             CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE,
             CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS,
             CONFIG_DSP_MAX_CONCURRENT_TRANSFERS,
             CONFIG_DSP_HTTP_PORT,
             CONFIG_DSP_LOG_LEVEL_VERBOSE);

    /* Start periodic report task before WiFi so it captures the watermark */
    /* during any TLS handshake that occurs after the server is up.        */
    xTaskCreate(mem_report_task, "mem_report", 2048, NULL, 1, NULL);

    /* ------------------------------------------------------------------ */
    /* WiFi                                                                */
    /* ------------------------------------------------------------------ */
    s_wifi_eg = xEventGroupCreate();
    dsp_wifi_set_event_cb(wifi_event_cb, s_wifi_eg);

#if CONFIG_DSP_WIFI_PROVISION
    /*
     * One-shot provisioning: write credentials to NVS, then proceed to
     * connect.  Rebuild with CONFIG_DSP_WIFI_PROVISION=n (or remove
     * sdkconfig.wifi) after the first successful boot.
     */
    {
        esp_err_t prov_err = dsp_wifi_store_credentials(
            CONFIG_DSP_WIFI_PROVISION_SSID,
            CONFIG_DSP_WIFI_PROVISION_PASSWORD);
        if (prov_err == ESP_OK) {
            ESP_LOGI(TAG, "WiFi credentials stored (SSID: %s)",
                     CONFIG_DSP_WIFI_PROVISION_SSID);
        } else {
            ESP_LOGE(TAG, "Failed to store WiFi credentials: 0x%x", prov_err);
        }
    }
#endif /* CONFIG_DSP_WIFI_PROVISION */

    esp_err_t err = dsp_wifi_init(NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed (0x%x) – store credentials first:", err);
        ESP_LOGE(TAG, "  Call dsp_wifi_store_credentials(ssid, pass) once, then reset");
        return;
    }

    dsp_wifi_connect();

    /* Wait up to 30 s for IP */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_eg, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(30000));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "WiFi did not connect within 30 s – check credentials");
        return;
    }

    /* ------------------------------------------------------------------ */
    /* Measurement 4 – after WiFi stack is up                             */
    /* delta(after-tls-init → after-wifi) = TCP/IP + WiFi driver overhead */
    /* ------------------------------------------------------------------ */
    dsp_mem_report("after-wifi");

    /* Start HTTP server */
    err = dsp_http_start(CONFIG_DSP_HTTP_PORT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP start failed: 0x%x", err);
        return;
    }

    /* ------------------------------------------------------------------ */
    /* Measurement 5 – HTTP server idle, no connections yet               */
    /* delta(after-wifi → after-http-start) = HTTP server task + sockets  */
    /* ------------------------------------------------------------------ */
    dsp_mem_report("after-http-start");

    ESP_LOGI(TAG, "DSP Embedded running on port %d", CONFIG_DSP_HTTP_PORT);
    ESP_LOGI(TAG, "Connect a TLS client to trigger a handshake.");
    ESP_LOGI(TAG, "AC1 check: boot.free_internal - min(periodic.min_free) <= 51200 bytes");
}
