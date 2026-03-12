/**
 * @file dsp_catalog.c
 * @brief Static DSP catalog module implementation.
 */

#include "dsp_catalog.h"
#include "dsp_build.h"
#include "dsp_config.h"
#include "dsp_http.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "dsp_catalog";

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */

static bool s_initialized = false;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

esp_err_t dsp_catalog_init(void)
{
    if (!s_initialized) {
        s_initialized = true;
        ESP_LOGI(TAG, "init: dataset=%s title=%s",
                 CONFIG_DSP_CATALOG_DATASET_ID,
                 CONFIG_DSP_CATALOG_TITLE);
    }
    return ESP_OK;
}

void dsp_catalog_deinit(void)
{
    s_initialized = false;
}

bool dsp_catalog_is_initialized(void)
{
    return s_initialized;
}

/* -------------------------------------------------------------------------
 * Serialisation
 * ------------------------------------------------------------------------- */

dsp_build_err_t dsp_catalog_get_json(char *out, size_t cap)
{
    return dsp_build_catalog(out, cap,
                              CONFIG_DSP_CATALOG_DATASET_ID,
                              CONFIG_DSP_CATALOG_TITLE);
}

/* -------------------------------------------------------------------------
 * HTTP handler
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM

#include "esp_http_server.h"

static esp_err_t catalog_get_handler(httpd_req_t *req)
{
    char buf[DSP_CATALOG_JSON_BUF_SIZE];
    dsp_build_err_t rc = dsp_catalog_get_json(buf, sizeof(buf));
    if (rc != DSP_BUILD_OK) {
        ESP_LOGE(TAG, "catalog serialization failed: %d", rc);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "catalog error");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

esp_err_t dsp_catalog_register_handler(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t rc = dsp_http_register_handler("/catalog", DSP_HTTP_GET,
                                              catalog_get_handler);
    if (rc == ESP_OK) {
        ESP_LOGI(TAG, "registered GET /catalog");
    }
    return rc;
}

#else /* host build */

/* Stub handler satisfies the non-NULL check in the dsp_http host stub.
 * It is never actually called from a real HTTP server on the host. */
static esp_err_t catalog_host_stub_handler(void *req)
{
    (void)req;
    return ESP_OK;
}

esp_err_t dsp_catalog_register_handler(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return dsp_http_register_handler("/catalog", DSP_HTTP_GET,
                                      catalog_host_stub_handler);
}

#endif /* ESP_PLATFORM */
