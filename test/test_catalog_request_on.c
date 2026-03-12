/**
 * @file test_catalog_request_on.c
 * @brief Unit tests for the POST /catalog/request ENABLED path (DSP-407).
 *
 * Compiled into dsp_test_catalog_request_on with
 * -DCONFIG_DSP_ENABLE_CATALOG_REQUEST=1, which overrides the default-0
 * fallback in dsp_config.h.
 *
 * When the flag is 1, dsp_catalog.c:
 *   - Compiles catalog_request_post_handler / catalog_request_host_stub.
 *   - Registers POST /catalog/request in dsp_catalog_register_request_handler().
 */

#include "unity.h"
#include "dsp_config.h"
#include "dsp_catalog.h"
#include "esp_err.h"

void setUp(void) {}
void tearDown(void) {}

/* -------------------------------------------------------------------------
 * Flag verification (enabled path)
 * ------------------------------------------------------------------------- */

void test_catalog_request_on_flag_is_enabled(void)
{
    TEST_ASSERT_EQUAL(1, CONFIG_DSP_ENABLE_CATALOG_REQUEST);
}

/* -------------------------------------------------------------------------
 * Handler registration – enabled path
 * ------------------------------------------------------------------------- */

void test_catalog_request_on_register_before_init_returns_invalid_state(void)
{
    dsp_catalog_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
                      dsp_catalog_register_request_handler());
}

void test_catalog_request_on_register_after_init_returns_ok(void)
{
    dsp_catalog_deinit();
    dsp_catalog_init();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_catalog_register_request_handler());
    dsp_catalog_deinit();
}
