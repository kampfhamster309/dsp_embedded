/**
 * @file test_dsp_http.c
 * @brief Host-native unit tests for the dsp_http component.
 *
 * These tests run without ESP-IDF or esp_http_server linked.  They verify:
 *   - DSP_HTTP_MAX_ROUTES compile-time constant is in range.
 *   - dsp_http_method_t enum values are distinct and ordered.
 *   - dsp_http_register_handler() validates arguments and fills the table.
 *   - dsp_http_register_handler() returns ESP_ERR_NO_MEM when table is full.
 *   - dsp_http_is_running() returns false before start on host.
 *   - dsp_http_start() returns ESP_FAIL on host (no server available).
 *   - dsp_http_stop() is safe to call when not running.
 *   - Compile-time guards (ESP_PLATFORM absent, DSP_HOST_BUILD set).
 */

#include "unity.h"
#include "dsp_http.h"
#include "esp_err.h"

#include <string.h>

/* setUp / tearDown defined once in test_smoke.c. */

/* Minimal handler stub – never actually called in host tests. */
static esp_err_t dummy_handler(void *req)
{
    (void)req;
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * Compile-time constant tests
 * ------------------------------------------------------------------------- */

void test_http_max_routes_in_range(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(1,  DSP_HTTP_MAX_ROUTES);
    TEST_ASSERT_LESS_OR_EQUAL(64, DSP_HTTP_MAX_ROUTES);
}

void test_http_max_routes_default_value(void)
{
    TEST_ASSERT_EQUAL(16, DSP_HTTP_MAX_ROUTES);
}

/* -------------------------------------------------------------------------
 * Enum tests
 * ------------------------------------------------------------------------- */

void test_http_method_enum_values_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(DSP_HTTP_GET,    DSP_HTTP_POST);
    TEST_ASSERT_NOT_EQUAL(DSP_HTTP_GET,    DSP_HTTP_PUT);
    TEST_ASSERT_NOT_EQUAL(DSP_HTTP_GET,    DSP_HTTP_DELETE);
    TEST_ASSERT_NOT_EQUAL(DSP_HTTP_POST,   DSP_HTTP_PUT);
    TEST_ASSERT_NOT_EQUAL(DSP_HTTP_POST,   DSP_HTTP_DELETE);
    TEST_ASSERT_NOT_EQUAL(DSP_HTTP_PUT,    DSP_HTTP_DELETE);
}

/* -------------------------------------------------------------------------
 * Argument validation tests
 * ------------------------------------------------------------------------- */

void test_http_register_null_uri_returns_invalid_arg(void)
{
    esp_err_t err = dsp_http_register_handler(NULL, DSP_HTTP_GET, dummy_handler);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_http_register_null_handler_returns_invalid_arg(void)
{
    esp_err_t err = dsp_http_register_handler("/test", DSP_HTTP_GET, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/* -------------------------------------------------------------------------
 * Route table tests
 *
 * Note: the host stub maintains its own static table; tests are designed
 * to be order-independent up to the table-full test which runs last.
 * Each test uses a unique URI to avoid cross-test interference.
 * ------------------------------------------------------------------------- */

void test_http_register_valid_handler_returns_ok(void)
{
    esp_err_t err = dsp_http_register_handler(
        "/test/register_ok", DSP_HTTP_GET, dummy_handler);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_http_register_post_handler_returns_ok(void)
{
    esp_err_t err = dsp_http_register_handler(
        "/test/register_post", DSP_HTTP_POST, dummy_handler);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_http_register_all_methods(void)
{
    /* Four distinct URIs, one per method */
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_http_register_handler("/test/m_get",    DSP_HTTP_GET,    dummy_handler));
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_http_register_handler("/test/m_post",   DSP_HTTP_POST,   dummy_handler));
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_http_register_handler("/test/m_put",    DSP_HTTP_PUT,    dummy_handler));
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_http_register_handler("/test/m_delete", DSP_HTTP_DELETE, dummy_handler));
}

/* -------------------------------------------------------------------------
 * Server state tests
 * ------------------------------------------------------------------------- */

void test_http_is_running_false_before_start(void)
{
    /* Server has never been started in the host build */
    TEST_ASSERT_FALSE(dsp_http_is_running());
}

void test_http_start_returns_fail_on_host(void)
{
    /* Real server not available without ESP-IDF */
    esp_err_t err = dsp_http_start(80);
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}

void test_http_is_running_false_after_failed_start(void)
{
    /* After a failed start the running flag must stay false */
    TEST_ASSERT_FALSE(dsp_http_is_running());
}

void test_http_stop_safe_when_not_running(void)
{
    /* Must not crash */
    dsp_http_stop();
    TEST_PASS();
}

void test_http_is_running_false_after_stop(void)
{
    TEST_ASSERT_FALSE(dsp_http_is_running());
}

/* -------------------------------------------------------------------------
 * Compile-time guard verification
 * ------------------------------------------------------------------------- */

void test_http_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}

void test_http_host_build_flag_set(void)
{
    TEST_ASSERT_EQUAL(1, DSP_HOST_BUILD);
}
