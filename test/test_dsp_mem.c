/**
 * @file test_dsp_mem.c
 * @brief Host-native unit tests for the dsp_mem component.
 *
 * On host builds dsp_mem stubs return fixed sentinel values so tests can
 * verify the API contracts without a real embedded heap.
 *
 * Tests verify:
 *   - Budget constants match target.md values.
 *   - DSP_MEM_HOST_FREE_B sentinel is above DSP_MEM_BUDGET_TOTAL_B
 *     (distinguishes host stub from a real constrained device read).
 *   - dsp_mem_free_internal() returns the host sentinel.
 *   - dsp_mem_free_psram() returns 0 on host (no SPIRAM).
 *   - dsp_mem_report() returns ESP_OK for valid and NULL tags.
 *   - Compile-time guards (ESP_PLATFORM absent).
 */

#include "unity.h"
#include "dsp_mem.h"
#include "esp_err.h"

/* setUp / tearDown defined in test_smoke.c. */

/* -------------------------------------------------------------------------
 * Budget constant tests
 * ------------------------------------------------------------------------- */

void test_mem_budget_total_is_130kb(void)
{
    TEST_ASSERT_EQUAL(130u * 1024u, DSP_MEM_BUDGET_TOTAL_B);
}

void test_mem_budget_tls_is_50kb(void)
{
    TEST_ASSERT_EQUAL(50u * 1024u, DSP_MEM_BUDGET_TLS_B);
}

void test_mem_budget_http_is_20kb(void)
{
    TEST_ASSERT_EQUAL(20u * 1024u, DSP_MEM_BUDGET_HTTP_B);
}

void test_mem_budget_json_is_15kb(void)
{
    TEST_ASSERT_EQUAL(15u * 1024u, DSP_MEM_BUDGET_JSON_B);
}

void test_mem_budget_jwt_is_25kb(void)
{
    TEST_ASSERT_EQUAL(25u * 1024u, DSP_MEM_BUDGET_JWT_B);
}

void test_mem_budget_dspsm_is_20kb(void)
{
    TEST_ASSERT_EQUAL(20u * 1024u, DSP_MEM_BUDGET_DSP_SM_B);
}

void test_mem_budget_components_fit_in_total(void)
{
    /* Sum of per-component budgets must not exceed the total budget.
     * TLS + HTTP + JSON + JWT + DSP_SM = 50+20+15+25+20 = 130 KB. */
    uint32_t sum = DSP_MEM_BUDGET_TLS_B
                 + DSP_MEM_BUDGET_HTTP_B
                 + DSP_MEM_BUDGET_JSON_B
                 + DSP_MEM_BUDGET_JWT_B
                 + DSP_MEM_BUDGET_DSP_SM_B;
    TEST_ASSERT_LESS_OR_EQUAL(DSP_MEM_BUDGET_TOTAL_B, sum);
}

/* -------------------------------------------------------------------------
 * Host sentinel tests
 * ------------------------------------------------------------------------- */

void test_mem_host_sentinel_above_budget(void)
{
    /* Sentinel must exceed the embedded budget so callers can detect
     * they are running on host vs. a real constrained device. */
    TEST_ASSERT_GREATER_THAN(DSP_MEM_BUDGET_TOTAL_B, DSP_MEM_HOST_FREE_B);
}

void test_mem_host_sentinel_is_512kb(void)
{
    TEST_ASSERT_EQUAL(512u * 1024u, DSP_MEM_HOST_FREE_B);
}

/* -------------------------------------------------------------------------
 * API return value tests
 * ------------------------------------------------------------------------- */

void test_mem_free_internal_returns_sentinel_on_host(void)
{
    TEST_ASSERT_EQUAL(DSP_MEM_HOST_FREE_B, dsp_mem_free_internal());
}

void test_mem_free_internal_is_nonzero(void)
{
    TEST_ASSERT_GREATER_THAN(0u, dsp_mem_free_internal());
}

void test_mem_free_psram_returns_zero_on_host(void)
{
    /* Host build has no SPIRAM → must return 0. */
    TEST_ASSERT_EQUAL(0u, dsp_mem_free_psram());
}

void test_mem_report_valid_tag_returns_ok(void)
{
    esp_err_t err = dsp_mem_report("unit-test");
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_mem_report_null_tag_returns_ok(void)
{
    /* NULL tag must not crash and must still return ESP_OK. */
    esp_err_t err = dsp_mem_report(NULL);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_mem_report_empty_tag_returns_ok(void)
{
    esp_err_t err = dsp_mem_report("");
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/* -------------------------------------------------------------------------
 * Compile-time guard
 * ------------------------------------------------------------------------- */

void test_mem_esp_platform_absent_in_host_build(void)
{
#ifdef ESP_PLATFORM
    TEST_FAIL_MESSAGE("ESP_PLATFORM must not be defined in host builds");
#endif
    TEST_PASS();
}
