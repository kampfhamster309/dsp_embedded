/**
 * @file dsp_daps.c
 * @brief DAPS compatibility shim implementation.
 *
 * Lifecycle management and error-code dispatch are fully implemented.
 * The HTTP client leg of dsp_daps_request_token() is stubbed out with
 * DSP_DAPS_ERR_HTTP; it will be completed in a future implementation ticket
 * once an OAuth2 client_credentials grant against CONFIG_DSP_DAPS_GATEWAY_URL
 * is needed.
 *
 * The compile-time flag CONFIG_DSP_ENABLE_DAPS_SHIM (default 0) controls
 * whether the gateway path is reachable at all.  The host-native build uses
 * the default (0) so DSP_DAPS_ERR_DISABLED is the expected return for valid
 * post-init calls.
 */

#include "dsp_daps.h"
#include "dsp_config.h"

#include <string.h>
#include <stdbool.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "dsp_daps";
#endif

/* =========================================================================
 * Module state
 * ========================================================================= */

static bool s_initialized = false;

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t dsp_daps_init(void)
{
    s_initialized = true;
    return ESP_OK;
}

void dsp_daps_deinit(void)
{
    s_initialized = false;
}

bool dsp_daps_is_initialized(void)
{
    return s_initialized;
}

dsp_daps_err_t dsp_daps_request_token(char *out_buf, size_t buf_cap,
                                        size_t *out_len)
{
    /* 1. Argument validation */
    if (!out_buf || buf_cap == 0 || !out_len) {
        return DSP_DAPS_ERR_INVALID_ARG;
    }

    /* 2. Initialization guard */
    if (!s_initialized) {
        return DSP_DAPS_ERR_NOT_INIT;
    }

    /* 3. Compile-time shim flag */
#if !CONFIG_DSP_ENABLE_DAPS_SHIM
    (void)buf_cap;
    return DSP_DAPS_ERR_DISABLED;

#else /* CONFIG_DSP_ENABLE_DAPS_SHIM */

    /* 4. Gateway URL presence */
    const char *url = CONFIG_DSP_DAPS_GATEWAY_URL;
    if (!url || url[0] == '\0') {
        return DSP_DAPS_ERR_NO_URL;
    }

    /* 5. HTTP request – stub: OAuth2 client_credentials grant not yet
     *    implemented.  A future ticket will add esp_http_client POST to
     *    CONFIG_DSP_DAPS_GATEWAY_URL/token and parse the access_token field. */
#ifdef ESP_PLATFORM
    ESP_LOGW(TAG, "dsp_daps_request_token: HTTP client stub – not implemented");
#endif
    (void)out_buf;
    (void)buf_cap;
    (void)out_len;
    return DSP_DAPS_ERR_HTTP;

#endif /* CONFIG_DSP_ENABLE_DAPS_SHIM */
}
