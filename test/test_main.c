/**
 * @file test_main.c
 * @brief Unity test runner entry point.
 *
 * Add a RUN_TEST() call here for every test function in every test_*.c file.
 * Test function declarations must be extern so the linker resolves them.
 */

#include "unity.h"

/* -------------------------------------------------------------------------
 * test_smoke.c
 * ------------------------------------------------------------------------- */
extern void test_unity_is_working(void);
extern void test_host_build_flag_is_set(void);
extern void test_esp_platform_not_set(void);
extern void test_config_catalog_request_default(void);
extern void test_config_consumer_default(void);
extern void test_config_daps_shim_default(void);
extern void test_config_negotiate_terminate_default(void);
extern void test_config_tls_session_tickets_default(void);
extern void test_config_psram_default(void);
extern void test_config_deep_sleep_default(void);
extern void test_config_max_negotiations_default(void);
extern void test_config_max_transfers_default(void);
extern void test_config_http_port_default(void);
extern void test_config_verbose_log_default(void);
extern void test_config_daps_gateway_url_default(void);

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();

    /* smoke */
    RUN_TEST(test_unity_is_working);
    RUN_TEST(test_host_build_flag_is_set);
    RUN_TEST(test_esp_platform_not_set);

    /* dsp_config defaults */
    RUN_TEST(test_config_catalog_request_default);
    RUN_TEST(test_config_consumer_default);
    RUN_TEST(test_config_daps_shim_default);
    RUN_TEST(test_config_negotiate_terminate_default);
    RUN_TEST(test_config_tls_session_tickets_default);
    RUN_TEST(test_config_psram_default);
    RUN_TEST(test_config_deep_sleep_default);
    RUN_TEST(test_config_max_negotiations_default);
    RUN_TEST(test_config_max_transfers_default);
    RUN_TEST(test_config_http_port_default);
    RUN_TEST(test_config_verbose_log_default);
    RUN_TEST(test_config_daps_gateway_url_default);

    return UNITY_END();
}
