/**
 * @file dsp_http.c
 * @brief DSP Embedded HTTP/1.1 server implementation.
 *
 * Platform: ESP32-S3 (ESP-IDF / FreeRTOS).
 * The #ifdef ESP_PLATFORM block is the real implementation using
 * esp_http_server.  The #else block provides host-native stubs so the
 * component can be linked into the host unit-test binary.
 */

#include "dsp_http.h"

/* =========================================================================
 * ESP32 implementation
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "dsp_http";

/* -------------------------------------------------------------------------
 * Route table
 * ------------------------------------------------------------------------- */

typedef struct {
    char                  uri[64];
    dsp_http_method_t     method;
    dsp_http_handler_fn_t handler;
    bool                  used;
} route_entry_t;

static route_entry_t  s_routes[DSP_HTTP_MAX_ROUTES];
static httpd_handle_t s_server  = NULL;
static bool           s_running = false;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/**
 * Map a DSP method enum to the equivalent ESP-IDF httpd_method_t value.
 * (ESP-IDF inherits values from http_parser.h: GET=1, POST=3, PUT=4, DELETE=0)
 */
static httpd_method_t map_method(dsp_http_method_t m)
{
    switch (m) {
        case DSP_HTTP_GET:    return HTTP_GET;
        case DSP_HTTP_POST:   return HTTP_POST;
        case DSP_HTTP_PUT:    return HTTP_PUT;
        case DSP_HTTP_DELETE: return HTTP_DELETE;
        default:              return HTTP_GET;
    }
}

/**
 * Single bridge function registered with esp_http_server for every route.
 * The actual dsp_http_handler_fn_t is stored in httpd_req_t::user_ctx.
 */
static esp_err_t bridge_handler(httpd_req_t *req)
{
    dsp_http_handler_fn_t fn = (dsp_http_handler_fn_t)req->user_ctx;
    return fn(req);
}

/** Register one route entry with the running server. */
static esp_err_t apply_route(httpd_handle_t server, const route_entry_t *e)
{
    httpd_uri_t uri_cfg = {
        .uri      = e->uri,
        .method   = map_method(e->method),
        .handler  = bridge_handler,
        .user_ctx = (void *)e->handler,
    };
    return httpd_register_uri_handler(server, &uri_cfg);
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

esp_err_t dsp_http_start(uint16_t port)
{
    if (s_running) {
        ESP_LOGW(TAG, "HTTP server already running on port %u", port);
        return ESP_ERR_INVALID_STATE;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port      = port;
    cfg.stack_size       = 4096;          /* keeps server task ≤4 KB stack  */
    cfg.max_open_sockets = 4;             /* max concurrent connections      */
    cfg.max_uri_handlers = DSP_HTTP_MAX_ROUTES;
    cfg.lru_purge_enable = true;          /* close LRU socket when table full */
    cfg.uri_match_fn     = httpd_uri_match_wildcard; /* wildcard URI matching for path-param routes */

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: 0x%x", err);
        return err;
    }

    /* Apply all pre-registered routes */
    for (int i = 0; i < DSP_HTTP_MAX_ROUTES; i++) {
        if (s_routes[i].used) {
            esp_err_t reg_err = apply_route(s_server, &s_routes[i]);
            if (reg_err != ESP_OK) {
                ESP_LOGE(TAG, "failed to register route %s: 0x%x",
                         s_routes[i].uri, reg_err);
            }
        }
    }

    s_running = true;
    ESP_LOGI(TAG, "HTTP server started on port %u (%d route slots)",
             port, DSP_HTTP_MAX_ROUTES);
    return ESP_OK;
}

void dsp_http_stop(void)
{
    if (!s_running || !s_server) {
        return;
    }
    httpd_stop(s_server);
    s_server  = NULL;
    s_running = false;
    ESP_LOGI(TAG, "HTTP server stopped");
}

esp_err_t dsp_http_register_handler(const char            *uri,
                                     dsp_http_method_t      method,
                                     dsp_http_handler_fn_t  handler)
{
    if (!uri || !handler) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Find a free slot */
    for (int i = 0; i < DSP_HTTP_MAX_ROUTES; i++) {
        if (!s_routes[i].used) {
            strncpy(s_routes[i].uri, uri, sizeof(s_routes[i].uri) - 1);
            s_routes[i].uri[sizeof(s_routes[i].uri) - 1] = '\0';
            s_routes[i].method  = method;
            s_routes[i].handler = handler;
            s_routes[i].used    = true;

            /* If server is already running, register immediately */
            if (s_running) {
                esp_err_t err = apply_route(s_server, &s_routes[i]);
                if (err != ESP_OK) {
                    s_routes[i].used = false;
                    return err;
                }
            }

            ESP_LOGD(TAG, "registered handler: %s %d -> slot %d", uri, method, i);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "route table full (DSP_HTTP_MAX_ROUTES=%d)", DSP_HTTP_MAX_ROUTES);
    return ESP_ERR_NO_MEM;
}

bool dsp_http_is_running(void)
{
    return s_running;
}

#else /* !ESP_PLATFORM – host-native stubs for unit testing */

#include "esp_err.h"
#include <string.h>
#include <stdbool.h>

/* Mirror the route table so host tests can verify registration logic. */
typedef struct {
    char                  uri[64];
    dsp_http_method_t     method;
    dsp_http_handler_fn_t handler;
    bool                  used;
} route_entry_t;

static route_entry_t s_routes[DSP_HTTP_MAX_ROUTES];
static bool          s_running = false;

esp_err_t dsp_http_start(uint16_t port)
{
    (void)port;
    /* Real server not available on host */
    s_running = false;
    return ESP_FAIL;
}

void dsp_http_stop(void)
{
    s_running = false;
}

esp_err_t dsp_http_register_handler(const char            *uri,
                                     dsp_http_method_t      method,
                                     dsp_http_handler_fn_t  handler)
{
    if (!uri || !handler) {
        return ESP_ERR_INVALID_ARG;
    }
    for (int i = 0; i < DSP_HTTP_MAX_ROUTES; i++) {
        if (!s_routes[i].used) {
            strncpy(s_routes[i].uri, uri, sizeof(s_routes[i].uri) - 1);
            s_routes[i].uri[sizeof(s_routes[i].uri) - 1] = '\0';
            s_routes[i].method  = method;
            s_routes[i].handler = handler;
            s_routes[i].used    = true;
            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

bool dsp_http_is_running(void)
{
    return s_running;
}

#endif /* ESP_PLATFORM */
