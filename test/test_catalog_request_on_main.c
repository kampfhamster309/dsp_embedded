/**
 * @file test_catalog_request_on_main.c
 * @brief Unity runner for the catalog-request ENABLED test binary.
 *
 * Compiled into dsp_test_catalog_request_on with
 * -DCONFIG_DSP_ENABLE_CATALOG_REQUEST=1.
 */

#include "unity.h"

extern void test_catalog_request_on_flag_is_enabled(void);
extern void test_catalog_request_on_register_before_init_returns_invalid_state(void);
extern void test_catalog_request_on_register_after_init_returns_ok(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_catalog_request_on_flag_is_enabled);
    RUN_TEST(test_catalog_request_on_register_before_init_returns_invalid_state);
    RUN_TEST(test_catalog_request_on_register_after_init_returns_ok);

    return UNITY_END();
}
