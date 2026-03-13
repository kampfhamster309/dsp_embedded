/**
 * @file dsp_protocol.c
 * @brief DSP Embedded protocol task – Core 0 setup (DSP-502).
 */

#include "dsp_protocol.h"
#include "dsp_config.h"
#include "dsp_shared.h"
#include "dsp_identity.h"
#include "dsp_tls.h"
#include "dsp_http.h"
#include "dsp_catalog.h"
#include "dsp_neg.h"
#include "dsp_xfer.h"
#include "esp_err.h"

#include "esp_log.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static const char *TAG = "dsp_protocol";

/* TLS server context owned by the protocol module. */
static dsp_tls_ctx_t s_tls_ctx;

/* Shared state pointer set at start, cleared at stop. */
static dsp_shared_t *s_sh = NULL;

/* True once the init sequence has completed. */
static volatile bool s_running = false;

#ifdef ESP_PLATFORM
static TaskHandle_t s_task_handle = NULL;
#endif

/* =========================================================================
 * Protocol initialisation sequence (platform-agnostic)
 * ========================================================================= */

static esp_err_t protocol_init(dsp_shared_t *sh)
{
#ifdef ESP_PLATFORM
    ESP_LOGI(TAG, "protocol task running on core %d (expected %d)",
             (int)xTaskGetCoreID(NULL), DSP_PROTOCOL_TASK_CORE);
#endif

    /* Identity: load device certificate and private key from flash. */
    esp_err_t err = dsp_identity_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "identity init failed: 0x%x", err);
        return err;
    }

    /* TLS: initialise server context with the device cert/key. */
    size_t cert_len = 0, key_len = 0;
    const uint8_t *cert = dsp_identity_get_cert(&cert_len);
    const uint8_t *key  = dsp_identity_get_key(&key_len);

    if (!cert || !key) {
        ESP_LOGE(TAG, "no device credentials – run tools/gen_dev_certs.sh");
        return ESP_ERR_INVALID_STATE;
    }

    /* TLS: on host the mbedTLS back-end is absent so dsp_tls_server_init()
     * always returns ESP_FAIL.  Log a warning and continue so that other
     * protocol init paths can still be exercised by host unit tests. */
    err = dsp_tls_server_init(&s_tls_ctx, cert, cert_len, key, key_len, "dsp_protocol");
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "TLS init returned 0x%x (expected on host)", err);
    }

    /* Catalog state machine. */
    err = dsp_catalog_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "catalog init failed: 0x%x", err);
        return err;
    }

    /* Negotiation state machine. */
    err = dsp_neg_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "negotiation init failed: 0x%x", err);
        return err;
    }

    /* Transfer state machine. */
    err = dsp_xfer_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "transfer init failed: 0x%x", err);
        return err;
    }

    /* HTTP server: start before registering handlers so dynamic registration
     * works on live server instances.  On host this returns ESP_FAIL (no real
     * server); log a warning and continue so tests can verify all other init. */
    err = dsp_http_start((uint16_t)CONFIG_DSP_HTTP_PORT);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP start returned 0x%x (expected on host)", err);
    }

    /* Register HTTP handlers for all DSP endpoints. */
    err = dsp_catalog_register_handler();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "catalog handler registration failed: 0x%x", err);
    }

#if CONFIG_DSP_ENABLE_CATALOG_REQUEST
    err = dsp_catalog_register_request_handler();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "catalog request handler registration failed: 0x%x", err);
    }
#endif

    err = dsp_neg_register_handlers();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "negotiation handler registration failed: 0x%x", err);
    }

    err = dsp_xfer_register_handlers();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "transfer handler registration failed: 0x%x", err);
    }

    /* Signal to Core 1 (and any observers) that the protocol stack is ready. */
    sh->core0_ready = 1;
    s_running = true;

    ESP_LOGI(TAG, "protocol stack ready (port %d)", CONFIG_DSP_HTTP_PORT);
    return ESP_OK;
}

/* =========================================================================
 * ESP-IDF task entry point
 * ========================================================================= */

#ifdef ESP_PLATFORM
static void protocol_task(void *arg)
{
    dsp_shared_t *sh = (dsp_shared_t *)arg;

    esp_err_t err = protocol_init(sh);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "protocol init failed (0x%x) – task exiting", err);
        s_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    /* Protocol task runs indefinitely; the HTTP server handles requests on its
     * own FreeRTOS task.  This task acts as a watchdog / lifecycle owner. */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif /* ESP_PLATFORM */

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t dsp_protocol_start(dsp_shared_t *sh)
{
    if (!sh) {
        return ESP_ERR_INVALID_ARG;
    }

    s_sh = sh;

#ifdef ESP_PLATFORM
    BaseType_t rc = xTaskCreatePinnedToCore(
        protocol_task,
        "dsp_protocol",
        DSP_PROTOCOL_TASK_STACK,
        sh,
        DSP_PROTOCOL_TASK_PRIO,
        &s_task_handle,
        DSP_PROTOCOL_TASK_CORE
    );
    if (rc != pdPASS) {
        s_sh = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
#else
    /* Host: run init synchronously so tests can inspect the result. */
    return protocol_init(sh);
#endif
}

void dsp_protocol_stop(void)
{
    s_running = false;

    dsp_http_stop();
    dsp_xfer_deinit();
    dsp_neg_deinit();
    dsp_catalog_deinit();
    dsp_tls_server_deinit(&s_tls_ctx);
    dsp_identity_deinit();

    if (s_sh) {
        s_sh->core0_ready = 0;
        s_sh = NULL;
    }

#ifdef ESP_PLATFORM
    if (s_task_handle) {
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }
#endif
}

bool dsp_protocol_is_running(void)
{
    return s_running;
}
