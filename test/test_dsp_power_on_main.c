/**
 * @file test_dsp_power_on_main.c
 * @brief Unity runner for dsp_test_deep_sleep_on binary (DSP-601).
 *
 * Compiled with -DCONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=1.
 * Exercises the enabled-path of the dsp_power module.
 */

#include "unity.h"

extern void test_power_flag_is_enabled(void);
extern void test_power_set_sleep_fn_null_is_safe(void);
extern void test_power_enter_deep_sleep_calls_sleep_fn(void);
extern void test_power_enter_deep_sleep_returns_ok(void);
extern void test_power_enter_deep_sleep_saves_rtc_state_before_sleep(void);
extern void test_power_enter_deep_sleep_rtc_valid_after_call(void);
extern void test_power_enter_deep_sleep_stops_http_server(void);
extern void test_power_enter_deep_sleep_preserves_neg_slot(void);
extern void test_power_enter_deep_sleep_preserves_xfer_slot(void);
extern void test_power_enter_deep_sleep_empty_tables_round_trip(void);
extern void test_power_wake_restore_recovers_neg_state(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_power_flag_is_enabled);
    RUN_TEST(test_power_set_sleep_fn_null_is_safe);
    RUN_TEST(test_power_enter_deep_sleep_calls_sleep_fn);
    RUN_TEST(test_power_enter_deep_sleep_returns_ok);
    RUN_TEST(test_power_enter_deep_sleep_saves_rtc_state_before_sleep);
    RUN_TEST(test_power_enter_deep_sleep_rtc_valid_after_call);
    RUN_TEST(test_power_enter_deep_sleep_stops_http_server);
    RUN_TEST(test_power_enter_deep_sleep_preserves_neg_slot);
    RUN_TEST(test_power_enter_deep_sleep_preserves_xfer_slot);
    RUN_TEST(test_power_enter_deep_sleep_empty_tables_round_trip);
    RUN_TEST(test_power_wake_restore_recovers_neg_state);

    return UNITY_END();
}
