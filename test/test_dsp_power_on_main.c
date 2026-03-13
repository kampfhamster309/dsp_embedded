/**
 * @file test_dsp_power_on_main.c
 * @brief Unity runner for dsp_test_deep_sleep_on binary (DSP-601/602).
 *
 * Compiled with -DCONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=1.
 * Exercises the enabled-path of the dsp_power module:
 *   DSP-601: enter_deep_sleep — mock invocation, RTC save, neg/xfer round-trip.
 *   DSP-602: handle_wakeup   — cause detection, state restore, error paths.
 */

#include "unity.h"

/* DSP-601: enter_deep_sleep tests */
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

/* DSP-602: handle_wakeup tests */
extern void test_power_handle_wakeup_undefined_returns_invalid_state(void);
extern void test_power_handle_wakeup_other_returns_invalid_state(void);
extern void test_power_handle_wakeup_timer_no_rtc_state_returns_invalid_state(void);
extern void test_power_handle_wakeup_timer_empty_tables_returns_ok(void);
extern void test_power_handle_wakeup_undefined_does_not_restore_slots(void);
extern void test_power_handle_wakeup_timer_restores_neg_slot(void);
extern void test_power_handle_wakeup_timer_restores_xfer_slot(void);
extern void test_power_handle_wakeup_timer_restores_agreed_neg_state(void);

int main(void)
{
    UNITY_BEGIN();

    /* DSP-601: enter_deep_sleep */
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

    /* DSP-602: handle_wakeup */
    RUN_TEST(test_power_handle_wakeup_undefined_returns_invalid_state);
    RUN_TEST(test_power_handle_wakeup_other_returns_invalid_state);
    RUN_TEST(test_power_handle_wakeup_timer_no_rtc_state_returns_invalid_state);
    RUN_TEST(test_power_handle_wakeup_timer_empty_tables_returns_ok);
    RUN_TEST(test_power_handle_wakeup_undefined_does_not_restore_slots);
    RUN_TEST(test_power_handle_wakeup_timer_restores_neg_slot);
    RUN_TEST(test_power_handle_wakeup_timer_restores_xfer_slot);
    RUN_TEST(test_power_handle_wakeup_timer_restores_agreed_neg_state);

    return UNITY_END();
}
