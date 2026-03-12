/**
 * @file dsp_http.h
 * @brief DSP Embedded HTTP/1.1 server skeleton.
 *
 * Thin wrapper around ESP-IDF's esp_http_server that:
 *   - Exposes a DSP-specific handler registration API.
 *   - Keeps the RAM footprint ≤20 KB (4 KB server task stack,
 *     max 4 open sockets, max DSP_HTTP_MAX_ROUTES handlers).
 *   - Decouples DSP protocol modules from esp_http_server directly.
 *
 * Used by: dsp_protocol (DSP-401+)
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef ESP_PLATFORM
#include "esp_http_server.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of URI+method pairs that can be registered. */
#define DSP_HTTP_MAX_ROUTES 16

/**
 * @brief HTTP methods supported by dsp_http.
 */
typedef enum {
    DSP_HTTP_GET    = 0,
    DSP_HTTP_POST   = 1,
    DSP_HTTP_PUT    = 2,
    DSP_HTTP_DELETE = 3,
} dsp_http_method_t;

/**
 * @brief URI handler callback.
 *
 * On ESP_PLATFORM the argument is the native httpd_req_t pointer, which
 * allows callers to use the full esp_http_server response API directly.
 * On host builds (no ESP_PLATFORM) it is an opaque void pointer; host
 * unit tests only verify registration logic, not handler I/O.
 */
#ifdef ESP_PLATFORM
typedef esp_err_t (*dsp_http_handler_fn_t)(httpd_req_t *req);
#else
typedef esp_err_t (*dsp_http_handler_fn_t)(void *req);
#endif

/**
 * @brief Start the HTTP/1.1 server on the given port.
 *
 * Applies all handlers previously registered via dsp_http_register_handler()
 * and begins accepting connections.  Must be called after the network
 * interface is up (e.g. after the WiFi-ready event from dsp_wifi, DSP-105).
 *
 * @param port  TCP port number (e.g. CONFIG_DSP_HTTP_PORT).
 * @return ESP_OK, ESP_ERR_INVALID_STATE if already running, or an IDF error.
 */
esp_err_t dsp_http_start(uint16_t port);

/**
 * @brief Stop the HTTP server and release all resources.
 *
 * Safe to call if the server is not running.
 */
void dsp_http_stop(void);

/**
 * @brief Register a URI handler.
 *
 * May be called before or after dsp_http_start().  Handlers registered
 * before start are applied at start time; those registered after start
 * are applied immediately via httpd_register_uri_handler().
 *
 * @param uri     Null-terminated URI path, e.g. "/api/v1/catalog".
 *                Maximum length: 63 characters.
 * @param method  HTTP method.
 * @param handler Callback invoked for matching requests.
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if uri or handler is NULL.
 *         ESP_ERR_NO_MEM if DSP_HTTP_MAX_ROUTES has been reached.
 */
esp_err_t dsp_http_register_handler(const char            *uri,
                                     dsp_http_method_t      method,
                                     dsp_http_handler_fn_t  handler);

/**
 * @brief Returns true if the HTTP server is currently running.
 */
bool dsp_http_is_running(void);

#ifdef __cplusplus
}
#endif
