/**
 * @file test_tickets_off_main.c
 * @brief Unity runner for the session-tickets-disabled test binary.
 *
 * This file provides main() for the dsp_test_no_tickets CTest target,
 * which is compiled with -DCONFIG_DSP_TLS_SESSION_TICKETS=0.
 */

#include "unity.h"

extern void test_tickets_off_flag_is_disabled(void);
extern void test_tickets_off_init_null_ctx_returns_invalid_arg(void);
extern void test_tickets_off_init_returns_fail_on_host(void);
extern void test_tickets_off_deinit_null_safe(void);
extern void test_tickets_off_deinit_not_initialized_safe(void);
extern void test_tickets_off_deinit_initialized_clears_flag(void);
extern void test_tickets_off_double_deinit_safe(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_tickets_off_flag_is_disabled);
    RUN_TEST(test_tickets_off_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_tickets_off_init_returns_fail_on_host);
    RUN_TEST(test_tickets_off_deinit_null_safe);
    RUN_TEST(test_tickets_off_deinit_not_initialized_safe);
    RUN_TEST(test_tickets_off_deinit_initialized_clears_flag);
    RUN_TEST(test_tickets_off_double_deinit_safe);

    return UNITY_END();
}
