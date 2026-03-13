/**
 * @file test_dsp_protocol.c
 * @brief Unit tests for dsp_protocol – Core 0 protocol task setup (DSP-502).
 *
 * On the host build dsp_protocol_start() runs the full initialisation
 * sequence synchronously (no RTOS task created), allowing all component
 * init paths and the shared-state flags to be verified without hardware.
 *
 * Hardware-only AC ("confirmed pinned to Core 0 by xTaskGetCoreID()") is
 * verified by the log line emitted at task startup on the device.
 */

#include "unity.h"
#include "dsp_protocol.h"
#include "dsp_shared.h"
#include "dsp_catalog.h"
#include "dsp_xfer.h"
#include "esp_err.h"

/* setUp / tearDown defined once in test_smoke.c. */

static dsp_shared_t s_sh;

static void reset_protocol(void)
{
    dsp_protocol_stop();
    dsp_shared_deinit(&s_sh);
    dsp_shared_init(&s_sh);
}

/* -------------------------------------------------------------------------
 * Argument validation
 * ------------------------------------------------------------------------- */

void test_protocol_start_null_sh_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_protocol_start(NULL));
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

void test_protocol_not_running_before_start(void)
{
    dsp_protocol_stop(); /* ensure clean state */
    TEST_ASSERT_FALSE(dsp_protocol_is_running());
}

void test_protocol_start_returns_ok(void)
{
    reset_protocol();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_protocol_start(&s_sh));
    dsp_protocol_stop();
}

void test_protocol_is_running_after_start(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    TEST_ASSERT_TRUE(dsp_protocol_is_running());
    dsp_protocol_stop();
}

void test_protocol_not_running_after_stop(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    dsp_protocol_stop();
    TEST_ASSERT_FALSE(dsp_protocol_is_running());
}

void test_protocol_stop_safe_before_start(void)
{
    dsp_protocol_stop();
    TEST_PASS();
}

void test_protocol_double_stop_safe(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    dsp_protocol_stop();
    dsp_protocol_stop();
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * Shared-state flags
 * ------------------------------------------------------------------------- */

void test_protocol_sets_core0_ready(void)
{
    reset_protocol();
    TEST_ASSERT_EQUAL(0u, s_sh.core0_ready);
    dsp_protocol_start(&s_sh);
    TEST_ASSERT_EQUAL(1u, s_sh.core0_ready);
    dsp_protocol_stop();
}

void test_protocol_clears_core0_ready_on_stop(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    dsp_protocol_stop();
    TEST_ASSERT_EQUAL(0u, s_sh.core0_ready);
}

/* -------------------------------------------------------------------------
 * Component initialisation verified via public is_initialized queries
 * ------------------------------------------------------------------------- */

void test_protocol_catalog_initialized_after_start(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    TEST_ASSERT_TRUE(dsp_catalog_is_initialized());
    dsp_protocol_stop();
}

void test_protocol_xfer_initialized_after_start(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    TEST_ASSERT_TRUE(dsp_xfer_is_initialized());
    dsp_protocol_stop();
}

void test_protocol_catalog_deinitialized_after_stop(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    dsp_protocol_stop();
    TEST_ASSERT_FALSE(dsp_catalog_is_initialized());
}

void test_protocol_xfer_deinitialized_after_stop(void)
{
    reset_protocol();
    dsp_protocol_start(&s_sh);
    dsp_protocol_stop();
    TEST_ASSERT_FALSE(dsp_xfer_is_initialized());
}

/* -------------------------------------------------------------------------
 * Task constants
 * ------------------------------------------------------------------------- */

void test_protocol_task_core_is_zero(void)
{
    TEST_ASSERT_EQUAL(0, DSP_PROTOCOL_TASK_CORE);
}

void test_protocol_task_stack_at_least_4096(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(4096u, DSP_PROTOCOL_TASK_STACK);
}
