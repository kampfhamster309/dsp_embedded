/**
 * @file dsp_psk.h
 * @brief Pre-Shared Key (PSK) credential management for DSP Embedded TLS.
 *
 * Stores a single PSK identity + key pair in static module memory and, on
 * ESP_PLATFORM, applies it to an mbedTLS SSL config so TLS-PSK connections
 * can be established without X.509 certificates.
 *
 * Credential management (init / set / get / is_configured / deinit) is
 * always compiled and fully unit-testable on host.
 * dsp_psk_apply() is ESP_PLATFORM-only; it calls mbedtls_ssl_conf_psk() and
 * restricts the cipher suite list to AES-GCM PSK suites.
 *
 * Security constraints enforced at dsp_psk_set():
 *   – identity length: 1 … DSP_PSK_MAX_IDENTITY_LEN bytes
 *   – key length:      DSP_PSK_MIN_KEY_LEN … DSP_PSK_MAX_KEY_LEN bytes
 *
 * DSP Embedded memory budget for this component: ≤1 KB (static buffers only).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef ESP_PLATFORM
#include "mbedtls/ssl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Limits
 * ------------------------------------------------------------------------- */

/** Maximum PSK identity length in bytes (RFC 4279 §5.3 allows up to 65535;
 *  we enforce a tighter embedded-friendly limit). */
#define DSP_PSK_MAX_IDENTITY_LEN  64u

/** Minimum PSK key length: 16 bytes = 128-bit AES-GCM minimum. */
#define DSP_PSK_MIN_KEY_LEN       16u

/** Maximum PSK key length: 32 bytes = 256-bit AES-GCM. */
#define DSP_PSK_MAX_KEY_LEN       32u

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Initialise the PSK module and clear any previously stored credentials.
 * Safe to call multiple times (idempotent).
 *
 * @return ESP_OK always.
 */
esp_err_t dsp_psk_init(void);

/**
 * Clear stored credentials and reset the module to uninitialized state.
 * Safe to call when no credentials are stored.
 */
void dsp_psk_deinit(void);

/* -------------------------------------------------------------------------
 * Credential management – always compiled
 * ------------------------------------------------------------------------- */

/**
 * Store a PSK identity and key in the module.
 *
 * Validates length constraints before copying.  Overwrites any previously
 * stored credentials.
 *
 * @param identity      PSK identity bytes (need not be NUL-terminated).
 * @param identity_len  Length of @p identity in bytes
 *                      (1 … DSP_PSK_MAX_IDENTITY_LEN).
 * @param key           PSK key bytes.
 * @param key_len       Length of @p key in bytes
 *                      (DSP_PSK_MIN_KEY_LEN … DSP_PSK_MAX_KEY_LEN).
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if any argument is invalid.
 */
esp_err_t dsp_psk_set(const uint8_t *identity, size_t identity_len,
                       const uint8_t *key,      size_t key_len);

/**
 * @return true if valid credentials have been stored via dsp_psk_set().
 */
bool dsp_psk_is_configured(void);

/**
 * Retrieve a pointer to the stored PSK identity.
 *
 * @param[out] out_len  Set to the identity length on success.
 * @return Pointer to the stored identity bytes, or NULL if credentials are
 *         not configured or @p out_len is NULL.
 */
const uint8_t *dsp_psk_get_identity(size_t *out_len);

/**
 * Retrieve a pointer to the stored PSK key.
 *
 * @param[out] out_len  Set to the key length on success.
 * @return Pointer to the stored key bytes, or NULL if credentials are not
 *         configured or @p out_len is NULL.
 */
const uint8_t *dsp_psk_get_key(size_t *out_len);

/* -------------------------------------------------------------------------
 * TLS integration – ESP_PLATFORM only
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM
/**
 * Apply the stored PSK credentials to an mbedTLS SSL server configuration.
 *
 * Calls mbedtls_ssl_conf_psk() and overrides the cipher suite list to
 * PSK-compatible AES-GCM suites (hardware-accelerated on ESP32-S3):
 *   – TLS_PSK_WITH_AES_128_GCM_SHA256
 *   – TLS_PSK_WITH_AES_256_GCM_SHA384
 *
 * @param conf  Initialised mbedTLS SSL configuration to update.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG for NULL @p conf,
 *         ESP_FAIL if credentials are not configured or mbedTLS returns
 *         an error.
 */
esp_err_t dsp_psk_apply(mbedtls_ssl_config *conf);
#endif /* ESP_PLATFORM */

#ifdef __cplusplus
}
#endif
