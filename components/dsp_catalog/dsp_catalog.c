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
#include "dsp_json.h"
#include "dsp_msg.h"

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

#if CONFIG_DSP_ENABLE_CATALOG_REQUEST
/**
 * POST /catalog/request
 *
 * Validates the incoming CatalogRequestMessage and responds with the same
 * static catalog as GET /catalog.
 *
 * DEV-001 deviation: filter parameters in the request body are accepted but
 * ignored — the full static catalog is always returned.  A runtime JSON-LD
 * processor capable of evaluating ODRL constraints is required for proper
 * filtering support; that is out of scope for the embedded target.
 */
static esp_err_t catalog_request_post_handler(httpd_req_t *req)
{
    char body[DSP_CATALOG_JSON_BUF_SIZE];
    int  recv_len = httpd_req_recv(req, body, sizeof(body) - 1u);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
        return ESP_FAIL;
    }
    body[recv_len] = '\0';

    if (dsp_msg_validate_catalog_request(body) != DSP_MSG_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "invalid CatalogRequestMessage");
        return ESP_FAIL;
    }

    char buf[DSP_CATALOG_JSON_BUF_SIZE];
    dsp_build_err_t rc = dsp_catalog_get_json(buf, sizeof(buf));
    if (rc != DSP_BUILD_OK) {
        ESP_LOGE(TAG, "catalog serialization failed: %d", rc);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "catalog error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    ESP_LOGD(TAG, "POST /catalog/request served (filter ignored, DEV-001)");
    return ESP_OK;
}
#endif /* CONFIG_DSP_ENABLE_CATALOG_REQUEST */

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

esp_err_t dsp_catalog_register_request_handler(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_DSP_ENABLE_CATALOG_REQUEST
    esp_err_t rc = dsp_http_register_handler("/catalog/request", DSP_HTTP_POST,
                                              catalog_request_post_handler);
    if (rc == ESP_OK) {
        ESP_LOGI(TAG, "registered POST /catalog/request");
    }
    return rc;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

#else /* host build */

/* Stub handlers satisfy the non-NULL check in the dsp_http host stub. */
static esp_err_t catalog_host_stub_handler(void *req)
{
    (void)req;
    return ESP_OK;
}

#if CONFIG_DSP_ENABLE_CATALOG_REQUEST
static esp_err_t catalog_request_host_stub(void *req)
{
    (void)req;
    return ESP_OK;
}
#endif

esp_err_t dsp_catalog_register_handler(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return dsp_http_register_handler("/catalog", DSP_HTTP_GET,
                                      catalog_host_stub_handler);
}

esp_err_t dsp_catalog_register_request_handler(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_DSP_ENABLE_CATALOG_REQUEST
    return dsp_http_register_handler("/catalog/request", DSP_HTTP_POST,
                                      catalog_request_host_stub);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

#endif /* ESP_PLATFORM */
