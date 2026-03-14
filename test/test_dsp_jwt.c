/**
 * @file test_dsp_jwt.c
 * @brief Host-native unit tests for the dsp_jwt component (DSP-202).
 *
 * Tests cover:
 *   - dsp_jwt_split(): NULL guards, one-dot / zero-dot rejection, pointer layout
 *   - dsp_jwt_base64url_decode(): NULL guards, empty input, "Hello World",
 *     ES256 header length, ES256 header first byte, small-buffer rejection
 *   - dsp_jwt_parse_alg(): NULL, ES256, RS256, unknown
 *   - dsp_jwt_parse_exp(): NULL guards, valid, far-future value, no-exp
 *   - dsp_jwt_validate_es256(): NULL guards, malformed, wrong alg, expired,
 *     no-exp, valid-format (returns DSP_JWT_ERR_CRYPTO on host), compile guard
 *
 * Test vectors (Python-verified):
 *   ES256 header b64url : eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9
 *                         → {"alg":"ES256","typ":"JWT"} (27 bytes)
 *   RS256 header b64url : eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9
 *   Valid payload b64url : eyJzdWIiOiJkc3AtdGVzdCIsImlhdCI6MTAwMDAwMDAwMCwiZXhwIjo5OTk5OTk5OTk5fQ
 *                          → {"sub":"dsp-test","iat":1000000000,"exp":9999999999}
 *   Expired payload b64url: eyJzdWIiOiJkc3AtdGVzdCIsImlhdCI6MTAwMDAwMDAwMCwiZXhwIjoxMDAwMDAwMDAxfQ
 *                           → exp=1000000001 (Sep 2001 – always in the past)
 *   No-exp payload b64url: eyJzdWIiOiJkc3AtdGVzdCIsImlhdCI6MTAwMDAwMDAwMH0
 *                          → {"sub":"dsp-test","iat":1000000000} (no exp)
 *   Fake sig (86 chars)  : AAAAAAA…AA → 64 zero bytes
 */

#include "unity.h"
#include "dsp_jwt.h"
#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* setUp / tearDown defined in test_smoke.c */

/* -------------------------------------------------------------------------
 * Test token constants
 * ------------------------------------------------------------------------- */

#define ES256_HEADER  "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9"
#define RS256_HEADER  "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9"
#define VALID_PAYLOAD "eyJzdWIiOiJkc3AtdGVzdCIsImlhdCI6MTAwMDAwMDAwMCwiZXhwIjo5OTk5OTk5OTk5fQ"
#define EXPD_PAYLOAD  "eyJzdWIiOiJkc3AtdGVzdCIsImlhdCI6MTAwMDAwMDAwMCwiZXhwIjoxMDAwMDAwMDAxfQ"
#define NOEXP_PAYLOAD "eyJzdWIiOiJkc3AtdGVzdCIsImlhdCI6MTAwMDAwMDAwMH0"
#define FAKE_SIG      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"

/* Full JWTs – ES256 */
#define VALID_JWT    ES256_HEADER "." VALID_PAYLOAD "." FAKE_SIG
#define EXPIRED_JWT  ES256_HEADER "." EXPD_PAYLOAD  "." FAKE_SIG
#define NOEXP_JWT    ES256_HEADER "." NOEXP_PAYLOAD "." FAKE_SIG
#define RS256_JWT    RS256_HEADER "." VALID_PAYLOAD "." FAKE_SIG

/* Full JWTs – RS256 */
#define RS256_VALID_JWT    RS256_HEADER "." VALID_PAYLOAD "." FAKE_SIG
#define RS256_EXPIRED_JWT  RS256_HEADER "." EXPD_PAYLOAD  "." FAKE_SIG
#define RS256_NOEXP_JWT    RS256_HEADER "." NOEXP_PAYLOAD "." FAKE_SIG

/* Minimal non-zero DER buffer – host stub never dereferences it */
static const uint8_t s_dummy_key[] = {0x30u, 0x00u};

/* -------------------------------------------------------------------------
 * dsp_jwt_split() tests
 * ------------------------------------------------------------------------- */

void test_jwt_split_null_jwt_returns_invalid_arg(void)
{
    dsp_jwt_parts_t parts;
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG, dsp_jwt_split(NULL, &parts));
}

void test_jwt_split_null_out_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG, dsp_jwt_split(VALID_JWT, NULL));
}

void test_jwt_split_no_dots_returns_invalid_format(void)
{
    dsp_jwt_parts_t parts;
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_FORMAT, dsp_jwt_split("nodots", &parts));
}

void test_jwt_split_one_dot_returns_invalid_format(void)
{
    dsp_jwt_parts_t parts;
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_FORMAT, dsp_jwt_split("a.b", &parts));
}

void test_jwt_split_valid_returns_ok(void)
{
    dsp_jwt_parts_t parts;
    TEST_ASSERT_EQUAL(DSP_JWT_OK, dsp_jwt_split(VALID_JWT, &parts));
}

void test_jwt_split_header_ptr_is_start_of_jwt(void)
{
    const char *jwt = VALID_JWT;
    dsp_jwt_parts_t parts;
    dsp_jwt_split(jwt, &parts);
    TEST_ASSERT_EQUAL_PTR(jwt, parts.header);
}

void test_jwt_split_dot_follows_header(void)
{
    const char *jwt = VALID_JWT;
    dsp_jwt_parts_t parts;
    dsp_jwt_split(jwt, &parts);
    TEST_ASSERT_EQUAL('.', jwt[parts.header_len]);
}

void test_jwt_split_payload_follows_first_dot(void)
{
    const char *jwt = VALID_JWT;
    dsp_jwt_parts_t parts;
    dsp_jwt_split(jwt, &parts);
    TEST_ASSERT_EQUAL_PTR(jwt + parts.header_len + 1, parts.payload);
}

void test_jwt_split_sig_follows_second_dot(void)
{
    const char *jwt = VALID_JWT;
    dsp_jwt_parts_t parts;
    dsp_jwt_split(jwt, &parts);
    const char *expected = jwt + parts.header_len + 1 + parts.payload_len + 1;
    TEST_ASSERT_EQUAL_PTR(expected, parts.sig);
}

/* -------------------------------------------------------------------------
 * dsp_jwt_base64url_decode() tests
 * ------------------------------------------------------------------------- */

void test_jwt_b64url_decode_null_src_returns_negative(void)
{
    uint8_t buf[4];
    TEST_ASSERT_LESS_THAN(0, dsp_jwt_base64url_decode(NULL, 4, buf, sizeof(buf)));
}

void test_jwt_b64url_decode_null_dst_returns_negative(void)
{
    TEST_ASSERT_LESS_THAN(0, dsp_jwt_base64url_decode("AAAA", 4, NULL, 4));
}

void test_jwt_b64url_decode_empty_returns_zero(void)
{
    uint8_t buf[4];
    TEST_ASSERT_EQUAL(0, dsp_jwt_base64url_decode("", 0, buf, sizeof(buf)));
}

void test_jwt_b64url_decode_hello_world(void)
{
    /* "Hello World" in base64url (no padding): SGVsbG8gV29ybGQ */
    uint8_t buf[16];
    int len = dsp_jwt_base64url_decode("SGVsbG8gV29ybGQ", 15, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(11, len);
    TEST_ASSERT_EQUAL_MEMORY("Hello World", buf, 11);
}

void test_jwt_b64url_decode_es256_header_length(void)
{
    /* ES256 header is 36 chars → 27 decoded bytes */
    uint8_t buf[64];
    int len = dsp_jwt_base64url_decode(ES256_HEADER, sizeof(ES256_HEADER) - 1,
                                        buf, sizeof(buf));
    TEST_ASSERT_EQUAL(27, len);
}

void test_jwt_b64url_decode_es256_header_first_byte(void)
{
    uint8_t buf[64];
    int len = dsp_jwt_base64url_decode(ES256_HEADER, sizeof(ES256_HEADER) - 1,
                                        buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_EQUAL('{', buf[0]);
}

void test_jwt_b64url_decode_insufficient_cap_returns_negative(void)
{
    /* 36-char header decodes to 27 bytes; cap=4 is too small */
    uint8_t buf[4];
    int result = dsp_jwt_base64url_decode(ES256_HEADER, sizeof(ES256_HEADER) - 1,
                                           buf, sizeof(buf));
    TEST_ASSERT_LESS_THAN(0, result);
}

/* DSP-801: cover b64url_char_to_val() branches for '-', '_', and invalid */

void test_jwt_b64url_decode_url_safe_minus_char(void)
{
    /* '-' maps to value 62.  "+" in standard base64 becomes "-" in base64url.
     * ">" (ASCII 62) is value 62 in standard base64; base64url uses "-".
     * Two-char group "-A" (62, 0): output byte = (62<<2)|(0>>4) = 248 = 0xF8. */
    uint8_t buf[4];
    int len = dsp_jwt_base64url_decode("-A", 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0xF8, buf[0]);
}

void test_jwt_b64url_decode_url_safe_underscore_char(void)
{
    /* '_' maps to value 63.  Two-char group "_w" (63, 48):
     * output byte = (63<<2)|(48>>4) = 252|3 = 255 = 0xFF. */
    uint8_t buf[4];
    int len = dsp_jwt_base64url_decode("_w", 2, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0xFF, buf[0]);
}

void test_jwt_b64url_decode_invalid_char_returns_negative(void)
{
    /* '!' is not in the base64url alphabet → decode returns -1. */
    uint8_t buf[4];
    int result = dsp_jwt_base64url_decode("AB!D", 4, buf, sizeof(buf));
    TEST_ASSERT_LESS_THAN(0, result);
}

void test_jwt_b64url_decode_single_char_is_invalid(void)
{
    /* A single trailing character (rem == 1) is not valid base64url. */
    uint8_t buf[4];
    int result = dsp_jwt_base64url_decode("A", 1, buf, sizeof(buf));
    TEST_ASSERT_LESS_THAN(0, result);
}

/* -------------------------------------------------------------------------
 * dsp_jwt_parse_alg() tests
 * ------------------------------------------------------------------------- */

void test_jwt_parse_alg_null_returns_unknown(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ALG_UNKNOWN, dsp_jwt_parse_alg(NULL, 0));
}

void test_jwt_parse_alg_es256_returns_es256(void)
{
    uint8_t buf[64];
    int len = dsp_jwt_base64url_decode(ES256_HEADER, sizeof(ES256_HEADER) - 1,
                                        buf, sizeof(buf));
    TEST_ASSERT_EQUAL(DSP_JWT_ALG_ES256, dsp_jwt_parse_alg(buf, (size_t)len));
}

void test_jwt_parse_alg_rs256_returns_rs256(void)
{
    uint8_t buf[64];
    int len = dsp_jwt_base64url_decode(RS256_HEADER, sizeof(RS256_HEADER) - 1,
                                        buf, sizeof(buf));
    TEST_ASSERT_EQUAL(DSP_JWT_ALG_RS256, dsp_jwt_parse_alg(buf, (size_t)len));
}

void test_jwt_parse_alg_unknown_returns_unknown(void)
{
    static const uint8_t json[] = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    TEST_ASSERT_EQUAL(DSP_JWT_ALG_UNKNOWN,
                      dsp_jwt_parse_alg(json, sizeof(json) - 1));
}

/* -------------------------------------------------------------------------
 * dsp_jwt_parse_exp() tests
 * ------------------------------------------------------------------------- */

void test_jwt_parse_exp_null_json_returns_invalid_arg(void)
{
    uint64_t exp = 0;
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_parse_exp(NULL, 0, &exp));
}

void test_jwt_parse_exp_null_out_returns_invalid_arg(void)
{
    static const uint8_t json[] = "{\"exp\":1000}";
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_parse_exp(json, sizeof(json) - 1, NULL));
}

void test_jwt_parse_exp_valid_returns_ok(void)
{
    uint8_t buf[128];
    int len = dsp_jwt_base64url_decode(VALID_PAYLOAD, sizeof(VALID_PAYLOAD) - 1,
                                        buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);
    uint64_t exp = 0;
    TEST_ASSERT_EQUAL(DSP_JWT_OK,
                      dsp_jwt_parse_exp(buf, (size_t)len, &exp));
}

void test_jwt_parse_exp_valid_value_is_far_future(void)
{
    /* VALID_PAYLOAD decodes to {"sub":"dsp-test","iat":1000000000,"exp":9999999999} */
    uint8_t buf[128];
    int len = dsp_jwt_base64url_decode(VALID_PAYLOAD, sizeof(VALID_PAYLOAD) - 1,
                                        buf, sizeof(buf));
    uint64_t exp = 0;
    dsp_jwt_parse_exp(buf, (size_t)len, &exp);
    TEST_ASSERT_TRUE(exp == 9999999999ULL);
}

void test_jwt_parse_exp_expired_value_is_small(void)
{
    /* EXPD_PAYLOAD → exp=1000000001 (Sep 2001) */
    uint8_t buf[128];
    int len = dsp_jwt_base64url_decode(EXPD_PAYLOAD, sizeof(EXPD_PAYLOAD) - 1,
                                        buf, sizeof(buf));
    uint64_t exp = 0;
    dsp_jwt_parse_exp(buf, (size_t)len, &exp);
    TEST_ASSERT_EQUAL(1000000001ULL, exp);
}

void test_jwt_parse_exp_no_exp_returns_no_exp(void)
{
    uint8_t buf[128];
    int len = dsp_jwt_base64url_decode(NOEXP_PAYLOAD, sizeof(NOEXP_PAYLOAD) - 1,
                                        buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);
    uint64_t exp = 0;
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_NO_EXP,
                      dsp_jwt_parse_exp(buf, (size_t)len, &exp));
}

/* -------------------------------------------------------------------------
 * dsp_jwt_validate_es256() tests
 * ------------------------------------------------------------------------- */

void test_jwt_validate_null_jwt_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_validate_es256(NULL,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_validate_null_pubkey_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_validate_es256(VALID_JWT, NULL, 1));
}

void test_jwt_validate_zero_len_pubkey_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_validate_es256(VALID_JWT, s_dummy_key, 0));
}

void test_jwt_validate_malformed_returns_invalid_format(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_FORMAT,
                      dsp_jwt_validate_es256("notajwt",
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_validate_wrong_alg_returns_invalid_alg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ALG,
                      dsp_jwt_validate_es256(RS256_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_validate_expired_returns_expired(void)
{
    /* exp=1000000001 (Sep 2001) is always in the past */
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_EXPIRED,
                      dsp_jwt_validate_es256(EXPIRED_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_validate_no_exp_returns_no_exp(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_NO_EXP,
                      dsp_jwt_validate_es256(NOEXP_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_validate_valid_format_host_returns_crypto(void)
{
    /* exp=9999999999 (year 2286): not expired.  Host stub returns CRYPTO. */
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_CRYPTO,
                      dsp_jwt_validate_es256(VALID_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

/* -------------------------------------------------------------------------
 * Compile-time guard
 * ------------------------------------------------------------------------- */

void test_jwt_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * dsp_jwt_validate_rs256() tests  (DSP-203)
 *
 * RS256 = RSA PKCS#1 v1.5 + SHA-256.  The signing/structural logic is
 * identical to ES256; the difference is the algorithm tag in the header
 * and that the signature is raw RSA bytes (no R||S→DER step needed).
 * Host stub exercises all pre-crypto checks and returns DSP_JWT_ERR_CRYPTO.
 * ------------------------------------------------------------------------- */

void test_jwt_rs256_alg_enum_distinct_from_es256(void)
{
    TEST_ASSERT_NOT_EQUAL(DSP_JWT_ALG_ES256, DSP_JWT_ALG_RS256);
}

void test_jwt_rs256_validate_null_jwt_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_validate_rs256(NULL,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_rs256_validate_null_pubkey_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_validate_rs256(RS256_VALID_JWT, NULL, 1));
}

void test_jwt_rs256_validate_zero_len_pubkey_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ARG,
                      dsp_jwt_validate_rs256(RS256_VALID_JWT, s_dummy_key, 0));
}

void test_jwt_rs256_validate_malformed_returns_invalid_format(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_FORMAT,
                      dsp_jwt_validate_rs256("notajwt",
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_rs256_validate_es256_alg_returns_invalid_alg(void)
{
    /* VALID_JWT has ES256 in the header – RS256 validator must reject it */
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_INVALID_ALG,
                      dsp_jwt_validate_rs256(VALID_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_rs256_validate_expired_returns_expired(void)
{
    /* exp=1000000001 (Sep 2001) is always in the past */
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_EXPIRED,
                      dsp_jwt_validate_rs256(RS256_EXPIRED_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_rs256_validate_no_exp_returns_no_exp(void)
{
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_NO_EXP,
                      dsp_jwt_validate_rs256(RS256_NOEXP_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}

void test_jwt_rs256_validate_valid_format_host_returns_crypto(void)
{
    /* exp=9999999999 (year 2286): not expired.  Host stub returns CRYPTO. */
    TEST_ASSERT_EQUAL(DSP_JWT_ERR_CRYPTO,
                      dsp_jwt_validate_rs256(RS256_VALID_JWT,
                                              s_dummy_key, sizeof(s_dummy_key)));
}
