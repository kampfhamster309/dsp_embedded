/**
 * @file dsp_jwt.c
 * @brief JWT validation implementation.
 *
 * Platform-agnostic helpers (split, base64url decode, alg/exp parsing) are
 * always compiled for host-native testing.  The ES256 structural checks
 * (argument validation, format, algorithm, expiry) are also always compiled.
 * The mbedTLS ECDSA signature verification is ESP_PLATFORM-only.
 */

#include "dsp_jwt.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* =========================================================================
 * Internal helpers – always compiled
 * ========================================================================= */

static int b64url_char_to_val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

/** Find @p needle in @p hay using memcmp; returns pointer or NULL. */
static const char *mem_find(const char *hay, size_t hay_len,
                             const char *needle, size_t ndl_len)
{
    if (ndl_len == 0) return hay;
    if (ndl_len > hay_len) return NULL;
    for (size_t i = 0; i <= hay_len - ndl_len; i++) {
        if (memcmp(hay + i, needle, ndl_len) == 0) return hay + i;
    }
    return NULL;
}

/* =========================================================================
 * Public helpers – always compiled
 * ========================================================================= */

dsp_jwt_err_t dsp_jwt_split(const char *jwt, dsp_jwt_parts_t *out)
{
    if (!jwt || !out) return DSP_JWT_ERR_INVALID_ARG;

    const char *dot1 = strchr(jwt, '.');
    if (!dot1) return DSP_JWT_ERR_INVALID_FORMAT;

    const char *dot2 = strchr(dot1 + 1, '.');
    if (!dot2) return DSP_JWT_ERR_INVALID_FORMAT;

    out->header      = jwt;
    out->header_len  = (size_t)(dot1 - jwt);
    out->payload     = dot1 + 1;
    out->payload_len = (size_t)(dot2 - dot1 - 1);
    out->sig         = dot2 + 1;
    out->sig_len     = strlen(dot2 + 1);

    return DSP_JWT_OK;
}

int dsp_jwt_base64url_decode(const char *src, size_t src_len,
                              uint8_t *dst, size_t dst_cap)
{
    if (!src || !dst) return -1;

    size_t out = 0;
    size_t i   = 0;

    /* Complete 4-character groups → 3 output bytes each */
    while (i + 3 < src_len) {
        int v0 = b64url_char_to_val(src[i]);
        int v1 = b64url_char_to_val(src[i + 1]);
        int v2 = b64url_char_to_val(src[i + 2]);
        int v3 = b64url_char_to_val(src[i + 3]);
        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return -1;
        if (out + 3 > dst_cap) return -1;
        dst[out++] = (uint8_t)((v0 << 2) | (v1 >> 4));
        dst[out++] = (uint8_t)((v1 << 4) | (v2 >> 2));
        dst[out++] = (uint8_t)((v2 << 6) | v3);
        i += 4;
    }

    /* Handle trailing 0, 2, or 3 characters (1 char remainder is invalid) */
    size_t rem = src_len - i;
    if (rem == 1) {
        return -1;
    } else if (rem == 2) {
        int v0 = b64url_char_to_val(src[i]);
        int v1 = b64url_char_to_val(src[i + 1]);
        if (v0 < 0 || v1 < 0) return -1;
        if (out + 1 > dst_cap) return -1;
        dst[out++] = (uint8_t)((v0 << 2) | (v1 >> 4));
    } else if (rem == 3) {
        int v0 = b64url_char_to_val(src[i]);
        int v1 = b64url_char_to_val(src[i + 1]);
        int v2 = b64url_char_to_val(src[i + 2]);
        if (v0 < 0 || v1 < 0 || v2 < 0) return -1;
        if (out + 2 > dst_cap) return -1;
        dst[out++] = (uint8_t)((v0 << 2) | (v1 >> 4));
        dst[out++] = (uint8_t)((v1 << 4) | (v2 >> 2));
    }
    /* rem == 0: nothing to do */

    return (int)out;
}

dsp_jwt_alg_t dsp_jwt_parse_alg(const uint8_t *json, size_t len)
{
    if (!json || len == 0) return DSP_JWT_ALG_UNKNOWN;
    const char *s = (const char *)json;
    if (mem_find(s, len, "\"ES256\"", 7)) return DSP_JWT_ALG_ES256;
    if (mem_find(s, len, "\"RS256\"", 7)) return DSP_JWT_ALG_RS256;
    return DSP_JWT_ALG_UNKNOWN;
}

dsp_jwt_err_t dsp_jwt_parse_exp(const uint8_t *json, size_t len,
                                 uint64_t *out_exp)
{
    if (!json || !out_exp) return DSP_JWT_ERR_INVALID_ARG;
    *out_exp = 0;

    const char  *s   = (const char *)json;
    static const char key[] = "\"exp\":";
    const char  *p   = mem_find(s, len, key, sizeof(key) - 1);
    if (!p) return DSP_JWT_ERR_NO_EXP;

    p += sizeof(key) - 1;
    while (p < s + len && *p == ' ') p++;           /* skip optional space  */
    if (p >= s + len || *p < '0' || *p > '9') return DSP_JWT_ERR_NO_EXP;

    uint64_t val = 0;
    while (p < s + len && *p >= '0' && *p <= '9') {
        val = val * 10u + (uint64_t)(*p - '0');
        p++;
    }
    *out_exp = val;
    return DSP_JWT_OK;
}

/* =========================================================================
 * Shared structural check – always compiled.
 * Validates args, JWT format, algorithm, and expiry.
 * Returns DSP_JWT_OK if all pre-crypto checks pass.
 * ========================================================================= */

static dsp_jwt_err_t jwt_check_structure(const char    *jwt,
                                          const uint8_t *pubkey_der,
                                          size_t         pubkey_der_len,
                                          dsp_jwt_parts_t *parts)
{
    if (!jwt || !pubkey_der || pubkey_der_len == 0) return DSP_JWT_ERR_INVALID_ARG;

    if (dsp_jwt_split(jwt, parts) != DSP_JWT_OK) return DSP_JWT_ERR_INVALID_FORMAT;

    /* Decode header and verify algorithm */
    uint8_t hdr_buf[128];
    int hdr_len = dsp_jwt_base64url_decode(parts->header, parts->header_len,
                                            hdr_buf, sizeof(hdr_buf));
    if (hdr_len < 0) return DSP_JWT_ERR_DECODE;
    if (dsp_jwt_parse_alg(hdr_buf, (size_t)hdr_len) != DSP_JWT_ALG_ES256) {
        return DSP_JWT_ERR_INVALID_ALG;
    }

    /* Decode payload and verify expiry */
    uint8_t pay_buf[DSP_JWT_MAX_DECODE_B];
    int pay_len = dsp_jwt_base64url_decode(parts->payload, parts->payload_len,
                                            pay_buf, sizeof(pay_buf));
    if (pay_len < 0) return DSP_JWT_ERR_DECODE;

    uint64_t exp = 0;
    dsp_jwt_err_t exp_err = dsp_jwt_parse_exp(pay_buf, (size_t)pay_len, &exp);
    if (exp_err != DSP_JWT_OK) return exp_err;   /* propagates DSP_JWT_ERR_NO_EXP */

    time_t now = time(NULL);
    if (now < 0 || (uint64_t)now >= exp) return DSP_JWT_ERR_EXPIRED;

    return DSP_JWT_OK;
}

/* =========================================================================
 * ES256 validation – platform split
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "esp_log.h"

static const char *TAG = "dsp_jwt";

/**
 * Convert a 64-byte raw R||S signature (IEEE P-1363) to DER-encoded
 * ECDSA-Sig-Value for use with mbedTLS pk_verify().
 *
 * ASN.1 DER:  SEQUENCE { INTEGER r, INTEGER s }
 * Prepend a 0x00 byte if the high bit of r or s is set to preserve sign.
 * Max output length for P-256: 2 + 2+33 + 2+33 = 72 bytes.
 */
static int rs_to_der(const uint8_t *rs, uint8_t *der, size_t *der_len)
{
    const uint8_t *r = rs;
    const uint8_t *s = rs + 32;

    int    r_pad  = (r[0] & 0x80u) ? 1 : 0;
    int    s_pad  = (s[0] & 0x80u) ? 1 : 0;
    size_t r_len  = 32u + (size_t)r_pad;
    size_t s_len  = 32u + (size_t)s_pad;
    size_t seq_len = 2u + r_len + 2u + s_len;

    if (seq_len + 2u > 74u) return -1;   /* should never happen for P-256 */

    uint8_t *p = der;
    *p++ = 0x30u;
    *p++ = (uint8_t)seq_len;
    *p++ = 0x02u;
    *p++ = (uint8_t)r_len;
    if (r_pad) *p++ = 0x00u;
    memcpy(p, r, 32); p += 32;
    *p++ = 0x02u;
    *p++ = (uint8_t)s_len;
    if (s_pad) *p++ = 0x00u;
    memcpy(p, s, 32); p += 32;

    *der_len = (size_t)(p - der);
    return 0;
}

dsp_jwt_err_t dsp_jwt_validate_es256(const char    *jwt,
                                      const uint8_t *pubkey_der,
                                      size_t         pubkey_der_len)
{
    dsp_jwt_parts_t parts;
    dsp_jwt_err_t err = jwt_check_structure(jwt, pubkey_der, pubkey_der_len, &parts);
    if (err != DSP_JWT_OK) return err;

    /* Decode the raw R||S signature – must be exactly 64 bytes for P-256 */
    uint8_t sig_raw[64];
    int sig_len = dsp_jwt_base64url_decode(parts.sig, parts.sig_len,
                                            sig_raw, sizeof(sig_raw));
    if (sig_len != 64) return DSP_JWT_ERR_INVALID_FORMAT;

    /* Convert to DER */
    uint8_t sig_der[74];
    size_t  sig_der_len = 0;
    if (rs_to_der(sig_raw, sig_der, &sig_der_len) != 0) return DSP_JWT_ERR_CRYPTO;

    /* SHA-256 of the signing input: "<header_b64url>.<payload_b64url>" */
    size_t signing_len = (size_t)(parts.payload + parts.payload_len - jwt);
    uint8_t digest[32];
    mbedtls_sha256((const unsigned char *)jwt, signing_len, digest, 0);

    /* Load public key and verify */
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    int ret = mbedtls_pk_parse_public_key(&pk,
                                           (const unsigned char *)pubkey_der,
                                           pubkey_der_len);
    if (ret != 0) {
        ESP_LOGW(TAG, "pk_parse_public_key failed: -0x%04x", (unsigned)-ret);
        mbedtls_pk_free(&pk);
        return DSP_JWT_ERR_CRYPTO;
    }

    ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256,
                             digest, sizeof(digest),
                             sig_der, sig_der_len);
    mbedtls_pk_free(&pk);

    if (ret != 0) {
        ESP_LOGW(TAG, "pk_verify failed: -0x%04x", (unsigned)-ret);
        return DSP_JWT_ERR_CRYPTO;
    }
    return DSP_JWT_OK;
}

#else /* !ESP_PLATFORM – host-native stub */

dsp_jwt_err_t dsp_jwt_validate_es256(const char    *jwt,
                                      const uint8_t *pubkey_der,
                                      size_t         pubkey_der_len)
{
    dsp_jwt_parts_t parts;
    dsp_jwt_err_t err = jwt_check_structure(jwt, pubkey_der, pubkey_der_len, &parts);
    if (err != DSP_JWT_OK) return err;

    /* All pre-crypto checks passed; crypto is unavailable on host */
    return DSP_JWT_ERR_CRYPTO;
}

#endif /* ESP_PLATFORM */
