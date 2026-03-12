/**
 * @file test_terminate_on.c
 * @brief Unit tests for the negotiate-terminate ENABLED path (DSP-404).
 *
 * Compiled into dsp_test_terminate_on with
 * -DCONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=1, which overrides the default-0
 * fallback in dsp_config.h.
 *
 * When the flag is 1, dsp_neg.c:
 *   - Compiles terminate_post_handler / neg_terminate_host_stub.
 *   - Registers POST /negotiations/terminate in dsp_neg_register_handlers().
 */

#include "unity.h"
#include "dsp_config.h"
#include "dsp_neg.h"
#include "esp_err.h"

void setUp(void) {}
void tearDown(void) {}

/* -------------------------------------------------------------------------
 * Flag verification (enabled path)
 * ------------------------------------------------------------------------- */

void test_terminate_on_flag_is_enabled(void)
{
    TEST_ASSERT_EQUAL(1, CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE);
}

/* -------------------------------------------------------------------------
 * Handler registration – four routes when flag is on
 * ------------------------------------------------------------------------- */

void test_terminate_on_register_handlers_returns_ok(void)
{
    dsp_neg_deinit();
    dsp_neg_init();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_neg_register_handlers());
    dsp_neg_deinit();
}

/* -------------------------------------------------------------------------
 * Slot-level terminate: any → TERMINATED with flag on
 * ------------------------------------------------------------------------- */

void test_terminate_on_apply_terminate_from_offered(void)
{
    dsp_neg_deinit();
    dsp_neg_init();
    int idx = dsp_neg_create("urn:uuid:c1", "urn:uuid:p1");
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_apply(idx, DSP_NEG_EVENT_TERMINATE));
    dsp_neg_deinit();
}

void test_terminate_on_apply_terminate_from_agreed(void)
{
    dsp_neg_deinit();
    dsp_neg_init();
    int idx = dsp_neg_create("urn:uuid:c2", "urn:uuid:p2");
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    dsp_neg_apply(idx, DSP_NEG_EVENT_AGREE);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_apply(idx, DSP_NEG_EVENT_TERMINATE));
    dsp_neg_deinit();
}

void test_terminate_on_terminated_absorbs_further_events(void)
{
    dsp_neg_deinit();
    dsp_neg_init();
    int idx = dsp_neg_create("urn:uuid:c3", "urn:uuid:p3");
    dsp_neg_apply(idx, DSP_NEG_EVENT_TERMINATE);
    /* OFFER after TERMINATED stays TERMINATED */
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER));
    dsp_neg_deinit();
}
