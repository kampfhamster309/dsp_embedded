/**
 * @file test_dsp_daps.c
 * @brief Host-native unit tests for the dsp_daps component (DSP-205).
 *
 * Tests cover:
 *   - Lifecycle: init/deinit idempotence, double-call safety
 *   - dsp_daps_request_token() argument validation (NULL buf, zero cap,
 *     NULL out_len) checked before init state
 *   - Request before init returns NOT_INIT
 *   - Request after init with default config returns DISABLED
 *     (CONFIG_DSP_ENABLE_DAPS_SHIM == 0 in host build)
 *   - Compile-time default value checks for shim flag and gateway URL
 *   - DSP_DAPS_MAX_TOKEN_LEN constant sanity
 *   - Compile-time guard (ESP_PLATFORM absent in host builds)
 *
 * The shim-enabled paths (DSP_DAPS_ERR_NO_URL, DSP_DAPS_ERR_HTTP) require
 * CONFIG_DSP_ENABLE_DAPS_SHIM=1 at compile time and are not exercised here;
 * they are covered by integration tests on device.
 */

#include "unity.h"
#include "dsp_daps.h"
#include "dsp_config.h"
#include "esp_err.h"

#include <stddef.h>
#include <string.h>

/* setUp / tearDown defined in test_smoke.c */

/* Helper: reset module state */
static void reset_daps(void)
{
    dsp_daps_deinit();
}

/* -------------------------------------------------------------------------
 * Lifecycle tests
 * ------------------------------------------------------------------------- */

void test_daps_init_returns_ok(void)
{
    reset_daps();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_daps_init());
    dsp_daps_deinit();
}

void test_daps_is_initialized_false_before_init(void)
{
    reset_daps();
    TEST_ASSERT_FALSE(dsp_daps_is_initialized());
}

void test_daps_is_initialized_true_after_init(void)
{
    reset_daps();
    dsp_daps_init();
    TEST_ASSERT_TRUE(dsp_daps_is_initialized());
    dsp_daps_deinit();
}

void test_daps_is_initialized_false_after_deinit(void)
{
    dsp_daps_init();
    dsp_daps_deinit();
    TEST_ASSERT_FALSE(dsp_daps_is_initialized());
}

void test_daps_double_init_safe(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, dsp_daps_init());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_daps_init());  /* idempotent */
    dsp_daps_deinit();
}

void test_daps_double_deinit_safe(void)
{
    dsp_daps_init();
    dsp_daps_deinit();
    dsp_daps_deinit();  /* must not crash */
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * dsp_daps_request_token() argument validation
 * (Argument checks fire before init-state check, so no init() needed.)
 * ------------------------------------------------------------------------- */

void test_daps_request_null_buf_returns_invalid_arg(void)
{
    size_t len = 0;
    TEST_ASSERT_EQUAL(DSP_DAPS_ERR_INVALID_ARG,
                      dsp_daps_request_token(NULL, 64, &len));
}

void test_daps_request_zero_cap_returns_invalid_arg(void)
{
    char buf[64];
    size_t len = 0;
    TEST_ASSERT_EQUAL(DSP_DAPS_ERR_INVALID_ARG,
                      dsp_daps_request_token(buf, 0, &len));
}

void test_daps_request_null_out_len_returns_invalid_arg(void)
{
    char buf[64];
    TEST_ASSERT_EQUAL(DSP_DAPS_ERR_INVALID_ARG,
                      dsp_daps_request_token(buf, sizeof(buf), NULL));
}

/* -------------------------------------------------------------------------
 * Init-state gate and compile-time config checks
 * ------------------------------------------------------------------------- */

void test_daps_request_before_init_returns_not_init(void)
{
    reset_daps();
    char buf[64];
    size_t len = 0;
    TEST_ASSERT_EQUAL(DSP_DAPS_ERR_NOT_INIT,
                      dsp_daps_request_token(buf, sizeof(buf), &len));
}

void test_daps_shim_disabled_by_default(void)
{
    /* Default host build must have the shim off.  If it were on, the
     * gateway URL would also need to be valid – not appropriate for CI. */
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_DAPS_SHIM);
}

void test_daps_gateway_url_empty_by_default(void)
{
    TEST_ASSERT_EQUAL_STRING("", CONFIG_DSP_DAPS_GATEWAY_URL);
}

void test_daps_request_shim_disabled_returns_disabled(void)
{
    /* With CONFIG_DSP_ENABLE_DAPS_SHIM == 0 (default), any post-init call
     * with valid arguments must return DSP_DAPS_ERR_DISABLED. */
    reset_daps();
    dsp_daps_init();
    char buf[DSP_DAPS_MAX_TOKEN_LEN + 1];
    size_t len = 0;
    TEST_ASSERT_EQUAL(DSP_DAPS_ERR_DISABLED,
                      dsp_daps_request_token(buf, sizeof(buf), &len));
    dsp_daps_deinit();
}

/* -------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */

void test_daps_max_token_len_nonzero(void)
{
    TEST_ASSERT_GREATER_THAN(0u, DSP_DAPS_MAX_TOKEN_LEN);
}

void test_daps_max_token_len_fits_typical_dat(void)
{
    /* A DAPS-issued DAT JWT is typically < 1 KB; verify our buffer is
     * large enough to hold a 900-byte token with room to spare. */
    TEST_ASSERT_GREATER_THAN(900u, DSP_DAPS_MAX_TOKEN_LEN);
}

void test_daps_error_codes_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(DSP_DAPS_OK,              DSP_DAPS_ERR_INVALID_ARG);
    TEST_ASSERT_NOT_EQUAL(DSP_DAPS_ERR_INVALID_ARG, DSP_DAPS_ERR_NOT_INIT);
    TEST_ASSERT_NOT_EQUAL(DSP_DAPS_ERR_NOT_INIT,    DSP_DAPS_ERR_DISABLED);
    TEST_ASSERT_NOT_EQUAL(DSP_DAPS_ERR_DISABLED,    DSP_DAPS_ERR_NO_URL);
    TEST_ASSERT_NOT_EQUAL(DSP_DAPS_ERR_NO_URL,      DSP_DAPS_ERR_HTTP);
}

/* -------------------------------------------------------------------------
 * Compile-time guard
 * ------------------------------------------------------------------------- */

void test_daps_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}
