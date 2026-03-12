/**
 * @file dsp_psk.c
 * @brief PSK credential management and TLS integration.
 *
 * Module state is held in two static buffers (identity and key) plus a
 * boolean flag.  All credential management functions are always compiled
 * so they can be unit-tested on host without mbedTLS.  dsp_psk_apply()
 * is ESP_PLATFORM-only.
 */

#include "dsp_psk.h"
#include <string.h>
#include <stdbool.h>

/* =========================================================================
 * Module state – always compiled
 * ========================================================================= */

static uint8_t s_identity[DSP_PSK_MAX_IDENTITY_LEN];
static size_t  s_identity_len;
static uint8_t s_key[DSP_PSK_MAX_KEY_LEN];
static size_t  s_key_len;
static bool    s_configured;

/* =========================================================================
 * Public API – always compiled
 * ========================================================================= */

esp_err_t dsp_psk_init(void)
{
    memset(s_identity, 0, sizeof(s_identity));
    memset(s_key,      0, sizeof(s_key));
    s_identity_len = 0;
    s_key_len      = 0;
    s_configured   = false;
    return ESP_OK;
}

void dsp_psk_deinit(void)
{
    /* Explicit zero-wipe of key material before release */
    memset(s_key,      0, sizeof(s_key));
    memset(s_identity, 0, sizeof(s_identity));
    s_identity_len = 0;
    s_key_len      = 0;
    s_configured   = false;
}

esp_err_t dsp_psk_set(const uint8_t *identity, size_t identity_len,
                       const uint8_t *key,      size_t key_len)
{
    if (!identity || identity_len == 0 ||
        identity_len > DSP_PSK_MAX_IDENTITY_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!key || key_len < DSP_PSK_MIN_KEY_LEN ||
        key_len > DSP_PSK_MAX_KEY_LEN) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(s_identity, identity, identity_len);
    s_identity_len = identity_len;
    memcpy(s_key, key, key_len);
    s_key_len    = key_len;
    s_configured = true;
    return ESP_OK;
}

bool dsp_psk_is_configured(void)
{
    return s_configured;
}

const uint8_t *dsp_psk_get_identity(size_t *out_len)
{
    if (!out_len || !s_configured) return NULL;
    *out_len = s_identity_len;
    return s_identity;
}

const uint8_t *dsp_psk_get_key(size_t *out_len)
{
    if (!out_len || !s_configured) return NULL;
    *out_len = s_key_len;
    return s_key;
}

/* =========================================================================
 * TLS integration – ESP_PLATFORM only
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include "esp_log.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"

static const char *TAG = "dsp_psk";

/* PSK-compatible AES-GCM cipher suites (hardware-accelerated on ESP32-S3) */
static const int s_psk_ciphersuite_list[] = {
    MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384,
    0 /* terminator */
};

esp_err_t dsp_psk_apply(mbedtls_ssl_config *conf)
{
    if (!conf) return ESP_ERR_INVALID_ARG;

    if (!s_configured) {
        ESP_LOGE(TAG, "dsp_psk_apply: no PSK credentials configured");
        return ESP_FAIL;
    }

    int ret = mbedtls_ssl_conf_psk(conf,
                                    s_key,      s_key_len,
                                    s_identity, s_identity_len);
    if (ret != 0) {
        char buf[128];
        mbedtls_strerror(ret, buf, sizeof(buf));
        ESP_LOGE(TAG, "mbedtls_ssl_conf_psk returned -0x%04X: %s",
                 (unsigned)(-ret), buf);
        return ESP_FAIL;
    }

    mbedtls_ssl_conf_ciphersuites(conf, s_psk_ciphersuite_list);

    ESP_LOGI(TAG, "PSK configured (identity_len=%u key_len=%u bits=%u)",
             (unsigned)s_identity_len,
             (unsigned)s_key_len,
             (unsigned)(s_key_len * 8u));
    return ESP_OK;
}

#endif /* ESP_PLATFORM */
