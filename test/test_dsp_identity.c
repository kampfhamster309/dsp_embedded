/**
 * @file test_dsp_identity.c
 * @brief Host-native unit tests for the dsp_identity component.
 *
 * On host builds dsp_identity returns stub DER data (two 2-byte placeholders)
 * so tests verify API contracts without needing real device certificates.
 *
 * Tests cover:
 *   - init/deinit lifecycle (idempotence, double-deinit safety)
 *   - get_cert / get_key: return non-NULL with correct length after init
 *   - get_cert / get_key: return NULL with length 0 before init / after deinit
 *   - NULL out_len argument handling
 *   - is_provisioned() reflects init state
 *   - Stub length distinguishable from a real cert (< 10 bytes)
 *   - Compile-time guard (ESP_PLATFORM absent)
 */

#include "unity.h"
#include "dsp_identity.h"
#include "esp_err.h"

#include <stddef.h>

/* setUp / tearDown defined in test_smoke.c. */

/* Helper: ensure a clean slate before state-sensitive tests. */
static void reset_identity(void)
{
    dsp_identity_deinit();
}

/* -------------------------------------------------------------------------
 * Lifecycle tests
 * ------------------------------------------------------------------------- */

void test_identity_init_returns_ok(void)
{
    reset_identity();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_identity_init());
    dsp_identity_deinit();
}

void test_identity_is_provisioned_false_before_init(void)
{
    reset_identity();
    TEST_ASSERT_FALSE(dsp_identity_is_provisioned());
}

void test_identity_is_provisioned_true_after_init(void)
{
    reset_identity();
    dsp_identity_init();
    TEST_ASSERT_TRUE(dsp_identity_is_provisioned());
    dsp_identity_deinit();
}

void test_identity_is_provisioned_false_after_deinit(void)
{
    dsp_identity_init();
    dsp_identity_deinit();
    TEST_ASSERT_FALSE(dsp_identity_is_provisioned());
}

void test_identity_double_deinit_safe(void)
{
    dsp_identity_init();
    dsp_identity_deinit();
    dsp_identity_deinit();  /* must not crash */
    TEST_PASS();
}

void test_identity_double_init_safe(void)
{
    reset_identity();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_identity_init());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_identity_init());  /* idempotent */
    dsp_identity_deinit();
}

/* -------------------------------------------------------------------------
 * get_cert() tests
 * ------------------------------------------------------------------------- */

void test_identity_get_cert_null_out_len_returns_null(void)
{
    dsp_identity_init();
    TEST_ASSERT_NULL(dsp_identity_get_cert(NULL));
    dsp_identity_deinit();
}

void test_identity_get_cert_returns_nonnull_after_init(void)
{
    dsp_identity_init();
    size_t len = 0;
    const uint8_t *ptr = dsp_identity_get_cert(&len);
    TEST_ASSERT_NOT_NULL(ptr);
    dsp_identity_deinit();
}

void test_identity_get_cert_returns_nonzero_len_after_init(void)
{
    dsp_identity_init();
    size_t len = 0;
    dsp_identity_get_cert(&len);
    TEST_ASSERT_GREATER_THAN(0u, len);
    dsp_identity_deinit();
}

void test_identity_get_cert_returns_null_before_init(void)
{
    reset_identity();
    size_t len = 99;
    const uint8_t *ptr = dsp_identity_get_cert(&len);
    TEST_ASSERT_NULL(ptr);
    TEST_ASSERT_EQUAL(0u, len);
}

void test_identity_get_cert_returns_null_after_deinit(void)
{
    dsp_identity_init();
    dsp_identity_deinit();
    size_t len = 99;
    const uint8_t *ptr = dsp_identity_get_cert(&len);
    TEST_ASSERT_NULL(ptr);
    TEST_ASSERT_EQUAL(0u, len);
}

/* -------------------------------------------------------------------------
 * get_key() tests
 * ------------------------------------------------------------------------- */

void test_identity_get_key_null_out_len_returns_null(void)
{
    dsp_identity_init();
    TEST_ASSERT_NULL(dsp_identity_get_key(NULL));
    dsp_identity_deinit();
}

void test_identity_get_key_returns_nonnull_after_init(void)
{
    dsp_identity_init();
    size_t len = 0;
    const uint8_t *ptr = dsp_identity_get_key(&len);
    TEST_ASSERT_NOT_NULL(ptr);
    dsp_identity_deinit();
}

void test_identity_get_key_returns_nonzero_len_after_init(void)
{
    dsp_identity_init();
    size_t len = 0;
    dsp_identity_get_key(&len);
    TEST_ASSERT_GREATER_THAN(0u, len);
    dsp_identity_deinit();
}

void test_identity_get_key_returns_null_before_init(void)
{
    reset_identity();
    size_t len = 99;
    const uint8_t *ptr = dsp_identity_get_key(&len);
    TEST_ASSERT_NULL(ptr);
    TEST_ASSERT_EQUAL(0u, len);
}

void test_identity_get_key_returns_null_after_deinit(void)
{
    dsp_identity_init();
    dsp_identity_deinit();
    size_t len = 99;
    const uint8_t *ptr = dsp_identity_get_key(&len);
    TEST_ASSERT_NULL(ptr);
    TEST_ASSERT_EQUAL(0u, len);
}

/* -------------------------------------------------------------------------
 * Stub-specific tests (host builds only)
 *
 * The host stub returns a 2-byte placeholder SEQUENCE.  A real embedded
 * cert is hundreds of bytes; testing that the stub length is small lets
 * us confirm we are in stub mode and the test is meaningful.
 * ------------------------------------------------------------------------- */

void test_identity_stub_cert_len_is_small(void)
{
    dsp_identity_init();
    size_t len = 0;
    dsp_identity_get_cert(&len);
    /* Stub is 2 bytes; a real DER cert is ≥ 200 bytes */
    TEST_ASSERT_LESS_THAN(10u, len);
    dsp_identity_deinit();
}

void test_identity_stub_key_len_is_small(void)
{
    dsp_identity_init();
    size_t len = 0;
    dsp_identity_get_key(&len);
    TEST_ASSERT_LESS_THAN(10u, len);
    dsp_identity_deinit();
}

void test_identity_stub_cert_starts_with_der_sequence_tag(void)
{
    dsp_identity_init();
    size_t len = 0;
    const uint8_t *ptr = dsp_identity_get_cert(&len);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL(0x30u, ptr[0]);  /* DER SEQUENCE tag */
    dsp_identity_deinit();
}

/* -------------------------------------------------------------------------
 * Compile-time guard
 * ------------------------------------------------------------------------- */

void test_identity_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}
