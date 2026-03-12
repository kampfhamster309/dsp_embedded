/**
 * @file dsp_identity.c
 * @brief DSP Embedded device identity implementation.
 *
 * ESP_PLATFORM: references the DER blobs embedded by COMPONENT_EMBED_FILES
 * and validates the certificate with mbedTLS at init time.
 *
 * Host-native stubs: use a minimal static byte array so tests can exercise
 * the API without a real certificate on disk.
 */

#include "dsp_identity.h"

/* =========================================================================
 * ESP32 implementation
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include <string.h>
#include "esp_log.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"

static const char *TAG = "dsp_identity";

/* Symbols injected by the linker for EMBED_FILES entries. */
extern const uint8_t _binary_device_cert_der_start[];
extern const uint8_t _binary_device_cert_der_end[];
extern const uint8_t _binary_device_key_der_start[];
extern const uint8_t _binary_device_key_der_end[];

static bool              s_provisioned = false;
static mbedtls_x509_crt s_crt;

esp_err_t dsp_identity_init(void)
{
    size_t cert_len = (size_t)(_binary_device_cert_der_end
                              - _binary_device_cert_der_start);
    size_t key_len  = (size_t)(_binary_device_key_der_end
                              - _binary_device_key_der_start);

    if (cert_len == 0 || key_len == 0) {
        ESP_LOGE(TAG, "embedded cert/key is empty – run tools/gen_dev_certs.sh");
        return ESP_ERR_INVALID_STATE;
    }

    mbedtls_x509_crt_init(&s_crt);

    int ret = mbedtls_x509_crt_parse_der(&s_crt,
                                          _binary_device_cert_der_start,
                                          cert_len);
    if (ret != 0) {
        char buf[128];
        mbedtls_strerror(ret, buf, sizeof(buf));
        ESP_LOGE(TAG, "cert parse failed -0x%04X: %s", (unsigned)(-ret), buf);
        mbedtls_x509_crt_free(&s_crt);
        return ESP_ERR_INVALID_ARG;
    }

    /* Log subject and validity for diagnostics. */
    char subject[128] = {0};
    mbedtls_x509_dn_gets(subject, sizeof(subject), &s_crt.subject);
    ESP_LOGI(TAG, "device cert loaded: subject='%s'  cert=%u B  key=%u B",
             subject, (unsigned)cert_len, (unsigned)key_len);
    ESP_LOGI(TAG, "  valid %04d-%02d-%02d to %04d-%02d-%02d",
             s_crt.valid_from.year, s_crt.valid_from.mon, s_crt.valid_from.day,
             s_crt.valid_to.year,   s_crt.valid_to.mon,   s_crt.valid_to.day);

    s_provisioned = true;
    return ESP_OK;
}

void dsp_identity_deinit(void)
{
    if (s_provisioned) {
        mbedtls_x509_crt_free(&s_crt);
        s_provisioned = false;
    }
}

const uint8_t *dsp_identity_get_cert(size_t *out_len)
{
    if (!out_len) {
        return NULL;
    }
    if (!s_provisioned) {
        *out_len = 0;
        return NULL;
    }
    *out_len = (size_t)(_binary_device_cert_der_end
                        - _binary_device_cert_der_start);
    return _binary_device_cert_der_start;
}

const uint8_t *dsp_identity_get_key(size_t *out_len)
{
    if (!out_len) {
        return NULL;
    }
    if (!s_provisioned) {
        *out_len = 0;
        return NULL;
    }
    *out_len = (size_t)(_binary_device_key_der_end
                        - _binary_device_key_der_start);
    return _binary_device_key_der_start;
}

bool dsp_identity_is_provisioned(void)
{
    return s_provisioned;
}

#else /* !ESP_PLATFORM – host-native stubs */

#include <stdbool.h>
#include <stddef.h>

/*
 * Minimal stub DER data for host unit tests.
 *
 * These are NOT real certificates – they are placeholder byte sequences
 * used only to verify that the API returns non-NULL pointers and correct
 * lengths.  No crypto operation is performed on host.
 *
 * A DER SEQUENCE tag (0x30) + length (0x00) → empty sequence, 2 bytes.
 * Real certs are hundreds of bytes; the length difference lets tests
 * distinguish "stub mode" from a real embedded build.
 */
static const uint8_t s_stub_cert[] = {
    /* DER SEQUENCE { } – clearly minimal, not a valid X.509 cert */
    0x30, 0x00
};
static const uint8_t s_stub_key[] = {
    /* DER SEQUENCE { } – clearly minimal, not a real EC key */
    0x30, 0x00
};

#define STUB_CERT_LEN  sizeof(s_stub_cert)
#define STUB_KEY_LEN   sizeof(s_stub_key)

static bool s_provisioned = false;

esp_err_t dsp_identity_init(void)
{
    s_provisioned = true;
    return ESP_OK;
}

void dsp_identity_deinit(void)
{
    s_provisioned = false;
}

const uint8_t *dsp_identity_get_cert(size_t *out_len)
{
    if (!out_len) {
        return NULL;
    }
    if (!s_provisioned) {
        *out_len = 0;
        return NULL;
    }
    *out_len = STUB_CERT_LEN;
    return s_stub_cert;
}

const uint8_t *dsp_identity_get_key(size_t *out_len)
{
    if (!out_len) {
        return NULL;
    }
    if (!s_provisioned) {
        *out_len = 0;
        return NULL;
    }
    *out_len = STUB_KEY_LEN;
    return s_stub_key;
}

bool dsp_identity_is_provisioned(void)
{
    return s_provisioned;
}

#endif /* ESP_PLATFORM */
