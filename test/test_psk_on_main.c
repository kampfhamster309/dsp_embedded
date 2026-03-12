/**
 * @file test_psk_on_main.c
 * @brief Unity runner for the PSK ENABLED test binary.
 *
 * Compiled into dsp_test_psk_on with -DCONFIG_DSP_ENABLE_PSK=1.
 */

#include "unity.h"

extern void test_psk_on_flag_is_enabled(void);
extern void test_psk_init_returns_ok(void);
extern void test_psk_is_configured_false_before_any_call(void);
extern void test_psk_is_configured_false_after_init(void);
extern void test_psk_is_configured_false_after_deinit(void);
extern void test_psk_double_init_safe(void);
extern void test_psk_double_deinit_safe(void);
extern void test_psk_set_null_identity_returns_invalid_arg(void);
extern void test_psk_set_null_key_returns_invalid_arg(void);
extern void test_psk_set_zero_identity_len_returns_invalid_arg(void);
extern void test_psk_set_zero_key_len_returns_invalid_arg(void);
extern void test_psk_set_identity_too_long_returns_invalid_arg(void);
extern void test_psk_set_key_too_long_returns_invalid_arg(void);
extern void test_psk_set_key_too_short_returns_invalid_arg(void);
extern void test_psk_set_valid_returns_ok(void);
extern void test_psk_is_configured_true_after_set(void);
extern void test_psk_get_identity_null_out_len_returns_null(void);
extern void test_psk_get_key_null_out_len_returns_null(void);
extern void test_psk_get_identity_null_before_set(void);
extern void test_psk_get_key_null_before_set(void);
extern void test_psk_get_identity_nonnull_after_set(void);
extern void test_psk_get_key_nonnull_after_set(void);
extern void test_psk_get_identity_len_correct(void);
extern void test_psk_get_key_len_correct(void);
extern void test_psk_get_identity_content_correct(void);
extern void test_psk_get_key_content_correct(void);
extern void test_psk_get_null_after_deinit(void);
extern void test_psk_esp_platform_absent_in_host_build(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_psk_on_flag_is_enabled);
    RUN_TEST(test_psk_init_returns_ok);
    RUN_TEST(test_psk_is_configured_false_before_any_call);
    RUN_TEST(test_psk_is_configured_false_after_init);
    RUN_TEST(test_psk_is_configured_false_after_deinit);
    RUN_TEST(test_psk_double_init_safe);
    RUN_TEST(test_psk_double_deinit_safe);
    RUN_TEST(test_psk_set_null_identity_returns_invalid_arg);
    RUN_TEST(test_psk_set_null_key_returns_invalid_arg);
    RUN_TEST(test_psk_set_zero_identity_len_returns_invalid_arg);
    RUN_TEST(test_psk_set_zero_key_len_returns_invalid_arg);
    RUN_TEST(test_psk_set_identity_too_long_returns_invalid_arg);
    RUN_TEST(test_psk_set_key_too_long_returns_invalid_arg);
    RUN_TEST(test_psk_set_key_too_short_returns_invalid_arg);
    RUN_TEST(test_psk_set_valid_returns_ok);
    RUN_TEST(test_psk_is_configured_true_after_set);
    RUN_TEST(test_psk_get_identity_null_out_len_returns_null);
    RUN_TEST(test_psk_get_key_null_out_len_returns_null);
    RUN_TEST(test_psk_get_identity_null_before_set);
    RUN_TEST(test_psk_get_key_null_before_set);
    RUN_TEST(test_psk_get_identity_nonnull_after_set);
    RUN_TEST(test_psk_get_key_nonnull_after_set);
    RUN_TEST(test_psk_get_identity_len_correct);
    RUN_TEST(test_psk_get_key_len_correct);
    RUN_TEST(test_psk_get_identity_content_correct);
    RUN_TEST(test_psk_get_key_content_correct);
    RUN_TEST(test_psk_get_null_after_deinit);
    RUN_TEST(test_psk_esp_platform_absent_in_host_build);

    return UNITY_END();
}
