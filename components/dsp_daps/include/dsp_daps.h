/**
 * @file dsp_daps.h
 * @brief DAPS (Dynamic Attribute Provisioning Service) compatibility shim.
 *
 * Provides an API for requesting Dynamic Attribute Tokens (DATs) from an
 * external DAPS/OAuth2 gateway.  Enabled at compile time by setting
 * CONFIG_DSP_ENABLE_DAPS_SHIM=y (Kconfig) and CONFIG_DSP_DAPS_GATEWAY_URL.
 *
 * This is a structural stub.  The lifecycle management and error-code
 * classification are complete; the HTTP client leg (OAuth2
 * client_credentials grant to the gateway) is scaffolded and returns
 * DSP_DAPS_ERR_HTTP until a future implementation ticket completes it.
 *
 * Error code precedence in dsp_daps_request_token():
 *   1. DSP_DAPS_ERR_INVALID_ARG  – NULL or zero-size output arguments
 *   2. DSP_DAPS_ERR_NOT_INIT     – dsp_daps_init() not called
 *   3. DSP_DAPS_ERR_DISABLED     – CONFIG_DSP_ENABLE_DAPS_SHIM == 0
 *   4. DSP_DAPS_ERR_NO_URL       – CONFIG_DSP_DAPS_GATEWAY_URL is empty
 *   5. DSP_DAPS_ERR_HTTP         – HTTP request to gateway failed (stub)
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */

/** Maximum size of a DAT JWT (bytes, excluding NUL terminator).
 *  A typical DAPS-issued DAT is 500–900 bytes; 1 KB gives comfortable margin. */
#define DSP_DAPS_MAX_TOKEN_LEN  1024u

/* -------------------------------------------------------------------------
 * Error codes
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_DAPS_OK              =  0,
    DSP_DAPS_ERR_INVALID_ARG = -1,  /**< NULL or zero-capacity argument        */
    DSP_DAPS_ERR_NOT_INIT    = -2,  /**< dsp_daps_init() has not been called   */
    DSP_DAPS_ERR_DISABLED    = -3,  /**< CONFIG_DSP_ENABLE_DAPS_SHIM == 0      */
    DSP_DAPS_ERR_NO_URL      = -4,  /**< CONFIG_DSP_DAPS_GATEWAY_URL is empty  */
    DSP_DAPS_ERR_HTTP        = -5,  /**< HTTP request to gateway failed        */
} dsp_daps_err_t;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Initialise the DAPS shim module.
 * Safe to call multiple times (idempotent).
 *
 * @return ESP_OK always.
 */
esp_err_t dsp_daps_init(void);

/**
 * Deinitialise the module and reset to uninitialized state.
 * Safe to call when already deinitialized.
 */
void dsp_daps_deinit(void);

/**
 * @return true if dsp_daps_init() has been called and dsp_daps_deinit()
 *         has not been called since.
 */
bool dsp_daps_is_initialized(void);

/* -------------------------------------------------------------------------
 * Token request
 * ------------------------------------------------------------------------- */

/**
 * Request a Dynamic Attribute Token (DAT) from the configured DAPS gateway.
 *
 * Error code precedence (checked in order):
 *   1. DSP_DAPS_ERR_INVALID_ARG  if @p out_buf is NULL, @p buf_cap is 0,
 *                                  or @p out_len is NULL.
 *   2. DSP_DAPS_ERR_NOT_INIT     if dsp_daps_init() has not been called.
 *   3. DSP_DAPS_ERR_DISABLED     if CONFIG_DSP_ENABLE_DAPS_SHIM == 0.
 *   4. DSP_DAPS_ERR_NO_URL       if CONFIG_DSP_DAPS_GATEWAY_URL is empty.
 *   5. DSP_DAPS_ERR_HTTP         stub: HTTP client not yet implemented.
 *
 * @param[out] out_buf   Caller-supplied buffer for the NUL-terminated token.
 * @param      buf_cap   Capacity of @p out_buf in bytes (≥ 1).
 * @param[out] out_len   Set to the token length (excluding NUL) on success.
 * @return DSP_DAPS_OK on success, or a DSP_DAPS_ERR_* code.
 */
dsp_daps_err_t dsp_daps_request_token(char *out_buf, size_t buf_cap,
                                        size_t *out_len);

#ifdef __cplusplus
}
#endif
