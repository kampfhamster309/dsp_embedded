/**
 * @file test_dsp_tls.c
 * @brief Host-native unit tests for the dsp_mbedtls component.
 *
 * These tests run without ESP-IDF or mbedTLS linked.  They verify:
 *   - dsp_tls_ctx_t struct layout and zero-init behaviour.
 *   - Host stubs return documented sentinel values.
 *   - Compile-time guards (ESP_PLATFORM absent, DSP_HOST_BUILD set).
 *
 * Tests that require actual TLS I/O are integration tests and run
 * on-device (not covered here).
 */

#include "unity.h"
#include "dsp_tls.h"
#include "esp_err.h"

#include <string.h>
#include <stdbool.h>

/* setUp / tearDown are defined once in test_smoke.c; Unity links them. */

/* -------------------------------------------------------------------------
 * Struct layout tests
 * ------------------------------------------------------------------------- */

void test_tls_ctx_sizeof_is_nonzero(void)
{
    TEST_ASSERT_GREATER_THAN(0, sizeof(dsp_tls_ctx_t));
}

void test_tls_ctx_zero_init_initialized_is_false(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_FALSE(ctx.initialized);
}

void test_tls_ctx_initialized_field_accessible(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = true;
    TEST_ASSERT_TRUE(ctx.initialized);
    ctx.initialized = false;
    TEST_ASSERT_FALSE(ctx.initialized);
}

/* -------------------------------------------------------------------------
 * Host-stub behaviour tests
 *
 * In a host build (no ESP_PLATFORM), dsp_tls_server_init() must return
 * ESP_ERR_INVALID_ARG for NULL ctx and ESP_FAIL for any other call,
 * because mbedTLS is not linked.
 * dsp_tls_server_deinit() must be safe to call on a zeroed context.
 * ------------------------------------------------------------------------- */

void test_tls_init_returns_invalid_arg_for_null_ctx(void)
{
    esp_err_t err = dsp_tls_server_init(
        NULL,
        (const unsigned char *)"cert", 4,
        (const unsigned char *)"key",  3,
        "pers");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_tls_init_returns_fail_on_host(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    esp_err_t err = dsp_tls_server_init(
        &ctx,
        (const unsigned char *)"cert", 4,
        (const unsigned char *)"key",  3,
        "pers");
    /* Host stub always returns ESP_FAIL – no mbedTLS linked */
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}

void test_tls_deinit_safe_on_null(void)
{
    /* Must not crash */
    dsp_tls_server_deinit(NULL);
    TEST_PASS();
}

void test_tls_deinit_clears_initialized(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = true;

    dsp_tls_server_deinit(&ctx);
    TEST_ASSERT_FALSE(ctx.initialized);
}

/* -------------------------------------------------------------------------
 * Compile-time guard verification
 * ------------------------------------------------------------------------- */

void test_tls_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}

void test_tls_host_build_flag_set(void)
{
    TEST_ASSERT_EQUAL(1, DSP_HOST_BUILD);
}
