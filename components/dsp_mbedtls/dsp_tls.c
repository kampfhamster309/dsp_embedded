/**
 * @file dsp_tls.c
 * @brief DSP Embedded TLS server context implementation.
 *
 * Configures mbedTLS for server mode with the minimal cipher suite set
 * required by DSP Embedded. See mbedtls_config_dsp.h for rationale.
 *
 * Platform: ESP32-S3 (ESP-IDF / FreeRTOS).
 * This file is compiled only on the embedded target (ESP_PLATFORM guard).
 * The host-native build gets stub linkage via the header guards in dsp_tls.h.
 */

#ifdef ESP_PLATFORM

#include <string.h>
#include "esp_log.h"
#include "dsp_tls.h"

#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#if CONFIG_DSP_TLS_SESSION_TICKETS
#include "mbedtls/ssl_ticket.h"
#endif

static const char *TAG = "dsp_tls";

/* ALPN protocols – HTTP/1.1 is the only transport in initial scope */
static const char *s_alpn_protos[] = { "http/1.1", NULL };

/* ---------------------------------------------------------------------------
 * Restricted cipher suite list
 *
 * Only ECDHE suites with AES-GCM are allowed:
 *  – forward secrecy via ECDHE
 *  – AES-GCM AEAD (hardware accelerated on ESP32-S3)
 *  – SHA-256 / SHA-384 (hardware accelerated on ESP32-S3)
 *
 * TLS 1.3 cipher suites are managed separately by mbedTLS internally and
 * do not appear in this list; TLS 1.3 always uses AEAD.
 * ------------------------------------------------------------------------- */
static const int s_ciphersuite_list[] = {
    /* ECDHE-ECDSA (preferred – used with ECDSA device certificate) */
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,

    /* ECDHE-RSA (fallback – used with RSA device certificate) */
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,

    0 /* terminator */
};

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static void log_mbedtls_error(const char *fn, int ret)
{
    char buf[128];
    mbedtls_strerror(ret, buf, sizeof(buf));
    ESP_LOGE(TAG, "%s returned -0x%04X: %s", fn, (unsigned)(-ret), buf);
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

esp_err_t dsp_tls_server_init(dsp_tls_ctx_t *ctx,
                               const unsigned char *cert_der, size_t cert_len,
                               const unsigned char *key_der,  size_t key_len,
                               const char *pers)
{
    if (!ctx || !cert_der || !key_der || !pers) {
        return ESP_ERR_INVALID_ARG;
    }

    int ret;
    memset(ctx, 0, sizeof(*ctx));

    /* ---- Entropy + RNG -------------------------------------------------- */
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                 &ctx->entropy,
                                 (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        log_mbedtls_error("mbedtls_ctr_drbg_seed", ret);
        goto fail;
    }

    /* ---- Certificate ----------------------------------------------------- */
    mbedtls_x509_crt_init(&ctx->srv_cert);

    if (cert_len == 0) {
        /* PEM: cert_der is a NULL-terminated string */
        ret = mbedtls_x509_crt_parse(&ctx->srv_cert, cert_der,
                                     strlen((const char *)cert_der) + 1);
    } else {
        ret = mbedtls_x509_crt_parse_der(&ctx->srv_cert, cert_der, cert_len);
    }
    if (ret != 0) {
        log_mbedtls_error("mbedtls_x509_crt_parse", ret);
        goto fail;
    }

    /* ---- Private key ----------------------------------------------------- */
    mbedtls_pk_init(&ctx->srv_key);

    if (key_len == 0) {
        /* PEM */
        ret = mbedtls_pk_parse_key(&ctx->srv_key, key_der,
                                   strlen((const char *)key_der) + 1,
                                   NULL, 0,
                                   mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    } else {
        ret = mbedtls_pk_parse_key(&ctx->srv_key, key_der, key_len,
                                   NULL, 0,
                                   mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    }
    if (ret != 0) {
        log_mbedtls_error("mbedtls_pk_parse_key", ret);
        goto fail;
    }

    /* ---- SSL configuration ---------------------------------------------- */
    mbedtls_ssl_config_init(&ctx->conf);

    ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        log_mbedtls_error("mbedtls_ssl_config_defaults", ret);
        goto fail;
    }

    /* RNG */
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    /* Certificate and key */
    ret = mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->srv_cert, &ctx->srv_key);
    if (ret != 0) {
        log_mbedtls_error("mbedtls_ssl_conf_own_cert", ret);
        goto fail;
    }

    /* Restrict to approved cipher suites (TLS 1.2) */
    mbedtls_ssl_conf_ciphersuites(&ctx->conf, s_ciphersuite_list);

    /* TLS version range: 1.2 minimum, 1.3 maximum */
    mbedtls_ssl_conf_min_tls_version(&ctx->conf, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&ctx->conf, MBEDTLS_SSL_VERSION_TLS1_3);

    /* ALPN – announce HTTP/1.1 */
    ret = mbedtls_ssl_conf_alpn_protocols(&ctx->conf, s_alpn_protos);
    if (ret != 0) {
        log_mbedtls_error("mbedtls_ssl_conf_alpn_protocols", ret);
        goto fail;
    }

    /* No client certificate required (counterparts authenticate via DSP tokens) */
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);

    /* Session tickets (reduces handshake cost after deep sleep) */
#if CONFIG_DSP_TLS_SESSION_TICKETS
    mbedtls_ssl_ticket_init(&ctx->ticket_ctx);
    ret = mbedtls_ssl_ticket_setup(&ctx->ticket_ctx,
                                   mbedtls_ctr_drbg_random, &ctx->ctr_drbg,
                                   MBEDTLS_CIPHER_AES_256_GCM,
                                   86400 /* lifetime: 24 h */);
    if (ret != 0) {
        log_mbedtls_error("mbedtls_ssl_ticket_setup", ret);
        goto fail;
    }
    mbedtls_ssl_conf_session_tickets_cb(&ctx->conf,
                                        mbedtls_ssl_ticket_write,
                                        mbedtls_ssl_ticket_parse,
                                        &ctx->ticket_ctx);
    ESP_LOGI(TAG, "TLS session tickets enabled (lifetime 24 h)");
#endif

    ctx->initialized = true;
    ESP_LOGI(TAG, "TLS server context initialised (TLS 1.2/1.3, ECDHE+AES-GCM)");
    return ESP_OK;

fail:
    dsp_tls_server_deinit(ctx);
    return ESP_FAIL;
}

void dsp_tls_server_deinit(dsp_tls_ctx_t *ctx)
{
    if (!ctx) {
        return;
    }

#if CONFIG_DSP_TLS_SESSION_TICKETS
    if (ctx->initialized) {
        mbedtls_ssl_ticket_free(&ctx->ticket_ctx);
    }
#endif

    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->srv_cert);
    mbedtls_pk_free(&ctx->srv_key);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);

    ctx->initialized = false;
}

const mbedtls_ssl_config *dsp_tls_server_get_config(const dsp_tls_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    return &ctx->conf;
}

#else /* !ESP_PLATFORM – host-native stubs for unit testing */

#include "dsp_tls.h"
#include "esp_err.h"

esp_err_t dsp_tls_server_init(dsp_tls_ctx_t *ctx,
                               const unsigned char *cert_der, size_t cert_len,
                               const unsigned char *key_der,  size_t key_len,
                               const char *pers)
{
    (void)cert_der; (void)cert_len;
    (void)key_der;  (void)key_len;
    (void)pers;
    if (!ctx) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Stubs always fail – mbedTLS is not linked in host builds */
    return ESP_FAIL;
}

void dsp_tls_server_deinit(dsp_tls_ctx_t *ctx)
{
    if (!ctx) {
        return;
    }
    ctx->initialized = false;
}

#endif /* ESP_PLATFORM */
