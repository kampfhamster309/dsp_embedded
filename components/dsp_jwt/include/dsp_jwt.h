/**
 * @file dsp_jwt.h
 * @brief JWT validation for the DSP embedded node.
 *
 * Supports ES256 (ECDSA P-256 + SHA-256) using mbedTLS hardware acceleration.
 * Base64url decoding, JWT splitting, and claim parsing are always compiled so
 * they can be unit-tested on host without hardware.  Signature verification is
 * ESP_PLATFORM-only; the host stub returns DSP_JWT_ERR_CRYPTO after all
 * structural and expiry checks pass.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** Maximum size (bytes) of a decoded JWT header or payload buffer. */
#define DSP_JWT_MAX_DECODE_B  512u

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Error codes
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_JWT_OK                 =  0,
    DSP_JWT_ERR_INVALID_ARG    = -1,  /**< NULL argument or zero-length key   */
    DSP_JWT_ERR_INVALID_FORMAT = -2,  /**< Not a 3-part compact JWT           */
    DSP_JWT_ERR_INVALID_ALG    = -3,  /**< Algorithm != ES256                 */
    DSP_JWT_ERR_EXPIRED        = -4,  /**< exp claim is in the past           */
    DSP_JWT_ERR_NO_EXP         = -5,  /**< exp claim absent                   */
    DSP_JWT_ERR_DECODE         = -6,  /**< Base64url decode failure           */
    DSP_JWT_ERR_CRYPTO         = -7,  /**< Signature verification failed      */
} dsp_jwt_err_t;

/* -------------------------------------------------------------------------
 * Algorithm identifiers
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_JWT_ALG_UNKNOWN = 0,
    DSP_JWT_ALG_ES256   = 1,
    DSP_JWT_ALG_RS256   = 2,
} dsp_jwt_alg_t;

/* -------------------------------------------------------------------------
 * Parsed JWT parts
 * Pointers reference the original token string and are NOT NUL-terminated.
 * ------------------------------------------------------------------------- */

typedef struct {
    const char *header;
    size_t      header_len;
    const char *payload;
    size_t      payload_len;
    const char *sig;
    size_t      sig_len;
} dsp_jwt_parts_t;

/* -------------------------------------------------------------------------
 * Public API – always compiled (platform-agnostic helpers)
 * ------------------------------------------------------------------------- */

/**
 * Split a compact JWT ("hdr.pay.sig") into its three base64url-encoded parts.
 * Pointers in @p out reference the original @p jwt string; nothing is decoded.
 *
 * @return DSP_JWT_OK, DSP_JWT_ERR_INVALID_ARG, or DSP_JWT_ERR_INVALID_FORMAT.
 */
dsp_jwt_err_t dsp_jwt_split(const char *jwt, dsp_jwt_parts_t *out);

/**
 * Decode a base64url string (RFC 4648 §5, no padding) into @p dst.
 *
 * @return Number of decoded bytes, or -1 on error (invalid character or
 *         output buffer too small).
 */
int dsp_jwt_base64url_decode(const char *src, size_t src_len,
                              uint8_t *dst, size_t dst_cap);

/**
 * Extract the "alg" claim from a decoded JSON header buffer.
 * Uses simple substring matching; does not parse full JSON.
 *
 * @return DSP_JWT_ALG_ES256, DSP_JWT_ALG_RS256, or DSP_JWT_ALG_UNKNOWN.
 */
dsp_jwt_alg_t dsp_jwt_parse_alg(const uint8_t *json, size_t len);

/**
 * Extract the "exp" (expiration) claim from a decoded JSON payload buffer.
 *
 * @param[out] out_exp  Expiration timestamp (seconds since Unix epoch).
 * @return DSP_JWT_OK on success, DSP_JWT_ERR_NO_EXP if the field is absent,
 *         or DSP_JWT_ERR_INVALID_ARG for NULL arguments.
 */
dsp_jwt_err_t dsp_jwt_parse_exp(const uint8_t *json, size_t len,
                                 uint64_t *out_exp);

/* -------------------------------------------------------------------------
 * ES256 validation
 *
 * ESP_PLATFORM: full mbedTLS ECDSA P-256 + SHA-256 verification.
 * Host builds: structural checks + expiry check only; returns
 *              DSP_JWT_ERR_CRYPTO once all pre-crypto checks pass.
 * ------------------------------------------------------------------------- */

/**
 * Validate an ES256 JWT token.
 *
 * Checks (in order): NULL arguments, 3-part structure, algorithm == ES256,
 * expiry in the future, ECDSA P-256 signature (ESP_PLATFORM only).
 *
 * @param jwt            NUL-terminated compact JWT string.
 * @param pubkey_der     DER-encoded EC public key.
 * @param pubkey_der_len Length of @p pubkey_der in bytes (must be > 0).
 * @return DSP_JWT_OK on success, or a DSP_JWT_ERR_* code.
 */
dsp_jwt_err_t dsp_jwt_validate_es256(const char *jwt,
                                      const uint8_t *pubkey_der,
                                      size_t pubkey_der_len);

#ifdef __cplusplus
}
#endif
