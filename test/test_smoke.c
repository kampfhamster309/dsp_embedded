/**
 * @file test_smoke.c
 * @brief Smoke tests – verify the host build pipeline end-to-end.
 *
 * These tests always pass.  Their purpose is to confirm that:
 *   1. Unity links and runs correctly on the host.
 *   2. dsp_config.h is includable without ESP-IDF.
 *   3. All compile-time flag defaults have the expected values.
 *
 * Add real unit tests in separate test_<component>.c files.
 */

#include "unity.h"
#include "dsp_config.h"

/* Called before every test by Unity. */
void setUp(void) {}

/* Called after every test by Unity. */
void tearDown(void) {}

/* -------------------------------------------------------------------------
 * Build system smoke tests
 * ------------------------------------------------------------------------- */

void test_unity_is_working(void)
{
    TEST_ASSERT_EQUAL(1, 1);
}

void test_host_build_flag_is_set(void)
{
#ifndef DSP_HOST_BUILD
    TEST_FAIL_MESSAGE("DSP_HOST_BUILD must be defined for host builds");
#endif
    TEST_ASSERT_EQUAL(1, DSP_HOST_BUILD);
}

void test_esp_platform_not_set(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must NOT be defined in host builds");
#endif
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * dsp_config.h default value tests
 *
 * Verify that every flag has the documented default when no sdkconfig is
 * present (i.e. in a host build without Kconfig overrides).
 * ------------------------------------------------------------------------- */

void test_config_catalog_request_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_CATALOG_REQUEST);
}

void test_config_consumer_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_CONSUMER);
}

void test_config_daps_shim_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_DAPS_SHIM);
}

void test_config_negotiate_terminate_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE);
}

void test_config_tls_session_tickets_default(void)
{
    TEST_ASSERT_EQUAL(1, CONFIG_DSP_TLS_SESSION_TICKETS);
}

void test_config_psram_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_PSRAM_ENABLE);
}

void test_config_deep_sleep_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX);
}

void test_config_max_negotiations_default(void)
{
    TEST_ASSERT_EQUAL(4, CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS);
    TEST_ASSERT_GREATER_OR_EQUAL(1,  CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS);
    TEST_ASSERT_LESS_OR_EQUAL(16, CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS);
}

void test_config_max_transfers_default(void)
{
    TEST_ASSERT_EQUAL(2, CONFIG_DSP_MAX_CONCURRENT_TRANSFERS);
    TEST_ASSERT_GREATER_OR_EQUAL(1, CONFIG_DSP_MAX_CONCURRENT_TRANSFERS);
    TEST_ASSERT_LESS_OR_EQUAL(8,  CONFIG_DSP_MAX_CONCURRENT_TRANSFERS);
}

void test_config_http_port_default(void)
{
    TEST_ASSERT_EQUAL(80, CONFIG_DSP_HTTP_PORT);
    TEST_ASSERT_GREATER_OR_EQUAL(1,     CONFIG_DSP_HTTP_PORT);
    TEST_ASSERT_LESS_OR_EQUAL(65535, CONFIG_DSP_HTTP_PORT);
}

void test_config_verbose_log_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_LOG_LEVEL_VERBOSE);
}

void test_config_daps_gateway_url_default(void)
{
    TEST_ASSERT_EQUAL_STRING("", CONFIG_DSP_DAPS_GATEWAY_URL);
}
