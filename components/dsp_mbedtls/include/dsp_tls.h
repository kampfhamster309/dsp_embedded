/**
 * @file dsp_tls.h
 * @brief DSP Embedded TLS server context management.
 *
 * Thin wrapper around mbedTLS that configures a TLS server context with the
 * settings defined in mbedtls_config_dsp.h / sdkconfig.defaults:
 *   - TLS 1.2 + TLS 1.3
 *   - ECDHE-RSA / ECDHE-ECDSA cipher suites
 *   - AES-GCM with hardware acceleration
 *   - Optional session tickets (CONFIG_DSP_TLS_SESSION_TICKETS)
 *   - Dynamic buffer allocation (reduced RAM at idle)
 *
 * Used by: dsp_http (DSP-102), dsp_identity (DSP-201)
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "dsp_config.h"

#ifdef ESP_PLATFORM
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#if CONFIG_DSP_TLS_SESSION_TICKETS
#include "mbedtls/ssl_ticket.h"
#endif
#endif /* ESP_PLATFORM */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TLS server context.
 *
 * Opaque aggregate of all mbedTLS objects needed for a TLS server.
 * Allocated and populated by dsp_tls_server_init().
 */
typedef struct dsp_tls_ctx {
#ifdef ESP_PLATFORM
    mbedtls_ssl_config      conf;
    mbedtls_x509_crt        srv_cert;
    mbedtls_pk_context      srv_key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
#if CONFIG_DSP_TLS_SESSION_TICKETS
    mbedtls_ssl_ticket_context ticket_ctx;
#endif
#else
    /* Host-native stub – fields unused, struct must be non-empty */
    int _placeholder;
#endif
    bool initialized;
} dsp_tls_ctx_t;

/**
 * @brief Initialise a TLS server context.
 *
 * Loads the DER- or PEM-encoded server certificate and private key, seeds
 * the RNG, configures the SSL context for server mode with the restricted
 * cipher suite set, and optionally enables session tickets.
 *
 * @param ctx         Caller-allocated context struct to populate.
 * @param cert_der    Server certificate (DER or PEM, NULL-terminated for PEM).
 * @param cert_len    Length in bytes (DER) or 0 to use strlen (PEM).
 * @param key_der     Server private key (DER or PEM).
 * @param key_len     Length in bytes (DER) or 0 to use strlen (PEM).
 * @param pers        Personalisation string for the RNG (e.g. "dsp_tls").
 *
 * @return ESP_OK on success, ESP_FAIL or an mbedTLS error code on failure.
 */
esp_err_t dsp_tls_server_init(dsp_tls_ctx_t *ctx,
                               const unsigned char *cert_der, size_t cert_len,
                               const unsigned char *key_der,  size_t key_len,
                               const char *pers);

/**
 * @brief Free all resources held by a TLS server context.
 *
 * Safe to call on a partially initialised context (checks ctx->initialized).
 *
 * @param ctx   Context previously passed to dsp_tls_server_init().
 */
void dsp_tls_server_deinit(dsp_tls_ctx_t *ctx);

#ifdef ESP_PLATFORM
/**
 * @brief Obtain a pointer to the underlying mbedTLS SSL config.
 *
 * Use this pointer when setting up individual mbedtls_ssl_context objects
 * (one per accepted client connection) via mbedtls_ssl_setup().
 *
 * @param ctx   Initialised TLS server context.
 * @return Pointer to the shared mbedtls_ssl_config, or NULL if not initialised.
 */
const mbedtls_ssl_config *dsp_tls_server_get_config(const dsp_tls_ctx_t *ctx);
#endif

#ifdef __cplusplus
}
#endif
