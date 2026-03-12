/**
 * @file test_terminate_on_main.c
 * @brief Unity runner for the negotiate-terminate ENABLED test binary.
 *
 * Compiled into dsp_test_terminate_on with
 * -DCONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=1.
 */

#include "unity.h"

extern void test_terminate_on_flag_is_enabled(void);
extern void test_terminate_on_register_handlers_returns_ok(void);
extern void test_terminate_on_apply_terminate_from_offered(void);
extern void test_terminate_on_apply_terminate_from_agreed(void);
extern void test_terminate_on_terminated_absorbs_further_events(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_terminate_on_flag_is_enabled);
    RUN_TEST(test_terminate_on_register_handlers_returns_ok);
    RUN_TEST(test_terminate_on_apply_terminate_from_offered);
    RUN_TEST(test_terminate_on_apply_terminate_from_agreed);
    RUN_TEST(test_terminate_on_terminated_absorbs_further_events);

    return UNITY_END();
}
