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
 * test_dsp_tls.c
 * ------------------------------------------------------------------------- */
extern void test_tls_ctx_sizeof_is_nonzero(void);
extern void test_tls_ctx_zero_init_initialized_is_false(void);
extern void test_tls_ctx_initialized_field_accessible(void);
extern void test_tls_init_returns_invalid_arg_for_null_ctx(void);
extern void test_tls_init_returns_fail_on_host(void);
extern void test_tls_deinit_safe_on_null(void);
extern void test_tls_deinit_clears_initialized(void);
extern void test_tls_esp_platform_absent_in_host_build(void);
extern void test_tls_host_build_flag_set(void);

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

    /* dsp_tls */
    RUN_TEST(test_tls_ctx_sizeof_is_nonzero);
    RUN_TEST(test_tls_ctx_zero_init_initialized_is_false);
    RUN_TEST(test_tls_ctx_initialized_field_accessible);
    RUN_TEST(test_tls_init_returns_invalid_arg_for_null_ctx);
    RUN_TEST(test_tls_init_returns_fail_on_host);
    RUN_TEST(test_tls_deinit_safe_on_null);
    RUN_TEST(test_tls_deinit_clears_initialized);
    RUN_TEST(test_tls_esp_platform_absent_in_host_build);
    RUN_TEST(test_tls_host_build_flag_set);

    return UNITY_END();
}
