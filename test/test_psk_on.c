/**
 * @file test_psk_on.c
 * @brief Unit tests for the dsp_psk component – ENABLED path (DSP-204).
 *
 * Compiled into dsp_test_psk_on with -DCONFIG_DSP_ENABLE_PSK=1.
 */

#include "unity.h"
#include "dsp_psk.h"
#include "dsp_config.h"
#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* -------------------------------------------------------------------------
 * Fixed test credentials
 * ------------------------------------------------------------------------- */

static const uint8_t s_id[]  = {'d', 's', 'p', '-', 'd', 'e', 'v'};   /* 7 B */
static const uint8_t s_key[] = {                                         /* 32 B */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
};

static void reset_psk(void) { dsp_psk_deinit(); }

/* -------------------------------------------------------------------------
 * Flag verification (enabled path)
 * ------------------------------------------------------------------------- */

void test_psk_on_flag_is_enabled(void)
{
    TEST_ASSERT_EQUAL(1, CONFIG_DSP_ENABLE_PSK);
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

void test_psk_init_returns_ok(void)
{
    reset_psk();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_psk_init());
    dsp_psk_deinit();
}

void test_psk_is_configured_false_before_any_call(void)
{
    reset_psk();
    TEST_ASSERT_FALSE(dsp_psk_is_configured());
}

void test_psk_is_configured_false_after_init(void)
{
    dsp_psk_init();
    TEST_ASSERT_FALSE(dsp_psk_is_configured());
    dsp_psk_deinit();
}

void test_psk_is_configured_false_after_deinit(void)
{
    dsp_psk_init();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    dsp_psk_deinit();
    TEST_ASSERT_FALSE(dsp_psk_is_configured());
}

void test_psk_double_init_safe(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, dsp_psk_init());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_psk_init());
    dsp_psk_deinit();
}

void test_psk_double_deinit_safe(void)
{
    dsp_psk_init();
    dsp_psk_deinit();
    dsp_psk_deinit();
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * dsp_psk_set() argument validation
 * ------------------------------------------------------------------------- */

void test_psk_set_null_identity_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(NULL, sizeof(s_id), s_key, sizeof(s_key)));
}

void test_psk_set_null_key_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(s_id, sizeof(s_id), NULL, sizeof(s_key)));
}

void test_psk_set_zero_identity_len_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(s_id, 0, s_key, sizeof(s_key)));
}

void test_psk_set_zero_key_len_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(s_id, sizeof(s_id), s_key, 0));
}

void test_psk_set_identity_too_long_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(s_id, DSP_PSK_MAX_IDENTITY_LEN + 1,
                                  s_key, sizeof(s_key)));
}

void test_psk_set_key_too_long_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(s_id, sizeof(s_id),
                                  s_key, DSP_PSK_MAX_KEY_LEN + 1));
}

void test_psk_set_key_too_short_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      dsp_psk_set(s_id, sizeof(s_id),
                                  s_key, DSP_PSK_MIN_KEY_LEN - 1));
}

void test_psk_set_valid_returns_ok(void)
{
    reset_psk();
    TEST_ASSERT_EQUAL(ESP_OK,
                      dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key)));
    dsp_psk_deinit();
}

void test_psk_is_configured_true_after_set(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    TEST_ASSERT_TRUE(dsp_psk_is_configured());
    dsp_psk_deinit();
}

/* -------------------------------------------------------------------------
 * dsp_psk_get_identity() / dsp_psk_get_key()
 * ------------------------------------------------------------------------- */

void test_psk_get_identity_null_out_len_returns_null(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    TEST_ASSERT_NULL(dsp_psk_get_identity(NULL));
    dsp_psk_deinit();
}

void test_psk_get_key_null_out_len_returns_null(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    TEST_ASSERT_NULL(dsp_psk_get_key(NULL));
    dsp_psk_deinit();
}

void test_psk_get_identity_null_before_set(void)
{
    reset_psk();
    size_t len = 99;
    TEST_ASSERT_NULL(dsp_psk_get_identity(&len));
}

void test_psk_get_key_null_before_set(void)
{
    reset_psk();
    size_t len = 99;
    TEST_ASSERT_NULL(dsp_psk_get_key(&len));
}

void test_psk_get_identity_nonnull_after_set(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    size_t len = 0;
    TEST_ASSERT_NOT_NULL(dsp_psk_get_identity(&len));
    dsp_psk_deinit();
}

void test_psk_get_key_nonnull_after_set(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    size_t len = 0;
    TEST_ASSERT_NOT_NULL(dsp_psk_get_key(&len));
    dsp_psk_deinit();
}

void test_psk_get_identity_len_correct(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    size_t len = 0;
    dsp_psk_get_identity(&len);
    TEST_ASSERT_EQUAL(sizeof(s_id), len);
    dsp_psk_deinit();
}

void test_psk_get_key_len_correct(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    size_t len = 0;
    dsp_psk_get_key(&len);
    TEST_ASSERT_EQUAL(sizeof(s_key), len);
    dsp_psk_deinit();
}

void test_psk_get_identity_content_correct(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    size_t len = 0;
    const uint8_t *p = dsp_psk_get_identity(&len);
    TEST_ASSERT_EQUAL_MEMORY(s_id, p, sizeof(s_id));
    dsp_psk_deinit();
}

void test_psk_get_key_content_correct(void)
{
    reset_psk();
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    size_t len = 0;
    const uint8_t *p = dsp_psk_get_key(&len);
    TEST_ASSERT_EQUAL_MEMORY(s_key, p, sizeof(s_key));
    dsp_psk_deinit();
}

void test_psk_get_null_after_deinit(void)
{
    dsp_psk_set(s_id, sizeof(s_id), s_key, sizeof(s_key));
    dsp_psk_deinit();
    size_t id_len = 1, key_len = 1;
    TEST_ASSERT_NULL(dsp_psk_get_identity(&id_len));
    TEST_ASSERT_NULL(dsp_psk_get_key(&key_len));
}

void test_psk_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}
