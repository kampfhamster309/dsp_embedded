/**
 * @file test_dsp_application.c
 * @brief Unit tests for dsp_application – Core 1 acquisition task (DSP-503).
 *
 * On the host build dsp_application_start() runs init and one acquisition
 * cycle synchronously, allowing ring buffer content and ready flags to be
 * verified without hardware or an RTOS.
 *
 * Hardware-only AC ("confirmed pinned to Core 1 by xTaskGetCoreID()") is
 * verified by the log line emitted at task startup on the device.
 */

#include "unity.h"
#include "dsp_application.h"
#include "dsp_shared.h"
#include "esp_err.h"

/* setUp / tearDown defined once in test_smoke.c. */

static dsp_shared_t s_sh;

static void reset_application(void)
{
    dsp_application_stop();
    dsp_shared_deinit(&s_sh);
    dsp_shared_init(&s_sh);
}

/* -------------------------------------------------------------------------
 * Argument validation
 * ------------------------------------------------------------------------- */

void test_application_start_null_sh_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_application_start(NULL));
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

void test_application_not_running_before_start(void)
{
    dsp_application_stop(); /* ensure clean state */
    TEST_ASSERT_FALSE(dsp_application_is_running());
}

void test_application_start_returns_ok(void)
{
    reset_application();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_application_start(&s_sh));
    dsp_application_stop();
}

void test_application_is_running_after_start(void)
{
    reset_application();
    dsp_application_start(&s_sh);
    TEST_ASSERT_TRUE(dsp_application_is_running());
    dsp_application_stop();
}

void test_application_not_running_after_stop(void)
{
    reset_application();
    dsp_application_start(&s_sh);
    dsp_application_stop();
    TEST_ASSERT_FALSE(dsp_application_is_running());
}

void test_application_stop_safe_before_start(void)
{
    dsp_application_stop();
    TEST_PASS();
}

void test_application_double_stop_safe(void)
{
    reset_application();
    dsp_application_start(&s_sh);
    dsp_application_stop();
    dsp_application_stop();
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * Shared-state flags
 * ------------------------------------------------------------------------- */

void test_application_sets_core1_ready(void)
{
    reset_application();
    TEST_ASSERT_EQUAL(0u, s_sh.core1_ready);
    dsp_application_start(&s_sh);
    TEST_ASSERT_EQUAL(1u, s_sh.core1_ready);
    dsp_application_stop();
}

void test_application_clears_core1_ready_on_stop(void)
{
    reset_application();
    dsp_application_start(&s_sh);
    dsp_application_stop();
    TEST_ASSERT_EQUAL(0u, s_sh.core1_ready);
}

/* -------------------------------------------------------------------------
 * Ring buffer – acquisition output
 * ------------------------------------------------------------------------- */

void test_application_ring_buf_has_sample_after_start(void)
{
    /* On host, start() runs one acquisition cycle synchronously.
     * The ring buffer must contain at least one sample. */
    reset_application();
    dsp_application_start(&s_sh);
    TEST_ASSERT_GREATER_THAN(0u, dsp_ring_buf_count(&s_sh));
    dsp_application_stop();
}

void test_application_sample_channel_is_valid(void)
{
    reset_application();
    dsp_application_start(&s_sh);

    dsp_sample_t out;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&s_sh, &out));
    TEST_ASSERT_LESS_THAN(DSP_SAMPLE_CHANNEL_MAX, (uint32_t)out.channel);

    dsp_application_stop();
}

void test_application_sample_raw_value_nonzero(void)
{
    /* The ADC stub returns (channel + 1) * 1000, always > 0 for channel 0. */
    reset_application();
    dsp_application_start(&s_sh);

    dsp_sample_t out;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&s_sh, &out));
    TEST_ASSERT_GREATER_THAN(0u, out.raw_value);

    dsp_application_stop();
}

void test_application_ring_buf_empty_after_stop_and_pop(void)
{
    reset_application();
    dsp_application_start(&s_sh);
    dsp_application_stop();
    /* Buffer is not cleared by stop; samples pushed before stop remain. */
    /* Drain it, then verify empty. */
    dsp_sample_t out;
    while (dsp_ring_buf_pop(&s_sh, &out) == ESP_OK) { /* drain */ }
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&s_sh));
}

/* -------------------------------------------------------------------------
 * Task constants
 * ------------------------------------------------------------------------- */

void test_application_task_core_is_one(void)
{
    TEST_ASSERT_EQUAL(1, DSP_APPLICATION_TASK_CORE);
}

void test_application_task_stack_at_least_4096(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(4096u, DSP_APPLICATION_TASK_STACK);
}
