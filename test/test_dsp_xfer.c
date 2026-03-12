/**
 * @file test_dsp_xfer.c
 * @brief Host-native Unity tests for dsp_xfer (DSP-405).
 *
 * 27 tests covering:
 *   - Pure SM function: all state × event transitions         ( 7 tests)
 *   - Slot table management                                   (13 tests)
 *   - Notify callback                                         ( 4 tests)
 *   - HTTP handler registration                               ( 3 tests)
 */

#include "unity.h"
#include "dsp_xfer.h"
#include "esp_err.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

#define PID  "urn:uuid:transfer-process-1"
#define PID2 "urn:uuid:transfer-process-2"

static void reset_xfer(void)
{
    dsp_xfer_deinit();
    dsp_xfer_init();
}

/* Notify probe */
static int s_notify_count = 0;
static int s_notified_idx = -1;

static void probe_notify(int idx)
{
    s_notify_count++;
    s_notified_idx = idx;
}

/* -------------------------------------------------------------------------
 * 1. Pure SM: dsp_xfer_sm_next
 * ------------------------------------------------------------------------- */

void test_xfer_sm_initial_start_goes_transferring(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING,
                      dsp_xfer_sm_next(DSP_XFER_STATE_INITIAL, DSP_XFER_EVENT_START));
}

void test_xfer_sm_initial_unhandled_stays(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_INITIAL,
                      dsp_xfer_sm_next(DSP_XFER_STATE_INITIAL, DSP_XFER_EVENT_COMPLETE));
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_INITIAL,
                      dsp_xfer_sm_next(DSP_XFER_STATE_INITIAL, DSP_XFER_EVENT_FAIL));
}

void test_xfer_sm_transferring_complete_goes_completed(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_COMPLETED,
                      dsp_xfer_sm_next(DSP_XFER_STATE_TRANSFERRING,
                                       DSP_XFER_EVENT_COMPLETE));
}

void test_xfer_sm_transferring_fail_goes_failed(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_FAILED,
                      dsp_xfer_sm_next(DSP_XFER_STATE_TRANSFERRING,
                                       DSP_XFER_EVENT_FAIL));
}

void test_xfer_sm_transferring_unhandled_stays(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING,
                      dsp_xfer_sm_next(DSP_XFER_STATE_TRANSFERRING,
                                       DSP_XFER_EVENT_START));
}

void test_xfer_sm_completed_absorbs_all(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_COMPLETED,
                      dsp_xfer_sm_next(DSP_XFER_STATE_COMPLETED, DSP_XFER_EVENT_START));
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_COMPLETED,
                      dsp_xfer_sm_next(DSP_XFER_STATE_COMPLETED, DSP_XFER_EVENT_FAIL));
}

void test_xfer_sm_failed_absorbs_all(void)
{
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_FAILED,
                      dsp_xfer_sm_next(DSP_XFER_STATE_FAILED, DSP_XFER_EVENT_START));
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_FAILED,
                      dsp_xfer_sm_next(DSP_XFER_STATE_FAILED, DSP_XFER_EVENT_COMPLETE));
}

/* -------------------------------------------------------------------------
 * 2. Slot table management
 * ------------------------------------------------------------------------- */

void test_xfer_init_returns_ok(void)
{
    dsp_xfer_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_xfer_init());
    dsp_xfer_deinit();
}

void test_xfer_is_initialized_false_before_init(void)
{
    dsp_xfer_deinit();
    TEST_ASSERT_FALSE(dsp_xfer_is_initialized());
}

void test_xfer_is_initialized_true_after_init(void)
{
    dsp_xfer_deinit();
    dsp_xfer_init();
    TEST_ASSERT_TRUE(dsp_xfer_is_initialized());
    dsp_xfer_deinit();
}

void test_xfer_count_active_zero_after_init(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(0, dsp_xfer_count_active());
}

void test_xfer_create_null_pid_returns_neg1(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(-1, dsp_xfer_create(NULL));
}

void test_xfer_create_returns_zero_first_slot(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(0, dsp_xfer_create(PID));
}

void test_xfer_create_state_is_initial(void)
{
    reset_xfer();
    int idx = dsp_xfer_create(PID);
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_INITIAL, dsp_xfer_get_state(idx));
}

void test_xfer_apply_start_gives_transferring(void)
{
    reset_xfer();
    int idx = dsp_xfer_create(PID);
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING,
                      dsp_xfer_apply(idx, DSP_XFER_EVENT_START));
}

void test_xfer_get_state_invalid_index_returns_initial(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_INITIAL, dsp_xfer_get_state(-1));
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_INITIAL, dsp_xfer_get_state(DSP_XFER_MAX));
}

void test_xfer_get_process_id_correct(void)
{
    reset_xfer();
    int idx = dsp_xfer_create(PID);
    TEST_ASSERT_EQUAL_STRING(PID, dsp_xfer_get_process_id(idx));
}

void test_xfer_get_process_id_invalid_returns_null(void)
{
    reset_xfer();
    TEST_ASSERT_NULL(dsp_xfer_get_process_id(-1));
    TEST_ASSERT_NULL(dsp_xfer_get_process_id(DSP_XFER_MAX));
}

void test_xfer_find_by_pid_returns_index(void)
{
    reset_xfer();
    int idx = dsp_xfer_create(PID);
    TEST_ASSERT_EQUAL(idx, dsp_xfer_find_by_pid(PID));
}

void test_xfer_find_by_pid_null_returns_neg1(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(-1, dsp_xfer_find_by_pid(NULL));
}

void test_xfer_find_by_pid_not_found_returns_neg1(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(-1, dsp_xfer_find_by_pid("urn:uuid:nobody"));
}

void test_xfer_table_overflow_returns_neg1(void)
{
    reset_xfer();
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        char pid[DSP_XFER_PID_LEN];
        pid[0] = 'u'; pid[1] = ':'; pid[2] = (char)('0' + (char)i); pid[3] = '\0';
        TEST_ASSERT_GREATER_OR_EQUAL(0, dsp_xfer_create(pid));
    }
    TEST_ASSERT_EQUAL(-1, dsp_xfer_create("urn:uuid:overflow"));
}

void test_xfer_count_active_tracks_creates(void)
{
    reset_xfer();
    TEST_ASSERT_EQUAL(0, dsp_xfer_count_active());
    dsp_xfer_create(PID);
    TEST_ASSERT_EQUAL(1, dsp_xfer_count_active());
    dsp_xfer_create(PID2);
    TEST_ASSERT_EQUAL(2, dsp_xfer_count_active());
}

void test_xfer_double_init_safe(void)
{
    dsp_xfer_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_xfer_init());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_xfer_init()); /* no-op, must not reset table */
    dsp_xfer_create(PID);
    TEST_ASSERT_EQUAL(1, dsp_xfer_count_active()); /* slot survived second init */
    dsp_xfer_deinit();
}

/* -------------------------------------------------------------------------
 * 3. Notify callback
 * ------------------------------------------------------------------------- */

void test_xfer_notify_null_by_default(void)
{
    /* Set to known state then clear; apply START must not crash. */
    dsp_xfer_set_notify(NULL);
    reset_xfer();
    int idx = dsp_xfer_create(PID);
    /* Must not crash even with NULL notify. */
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING,
                      dsp_xfer_apply(idx, DSP_XFER_EVENT_START));
}

void test_xfer_notify_called_on_start(void)
{
    reset_xfer();
    s_notify_count = 0;
    s_notified_idx = -1;
    dsp_xfer_set_notify(probe_notify);

    int idx = dsp_xfer_create(PID);
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);

    TEST_ASSERT_EQUAL(1, s_notify_count);
    TEST_ASSERT_EQUAL(idx, s_notified_idx);

    dsp_xfer_set_notify(NULL);
}

void test_xfer_notify_not_called_on_complete(void)
{
    reset_xfer();
    s_notify_count = 0;
    dsp_xfer_set_notify(probe_notify);

    int idx = dsp_xfer_create(PID);
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START); /* notify fires here (count=1) */
    s_notify_count = 0;                         /* reset counter */
    dsp_xfer_apply(idx, DSP_XFER_EVENT_COMPLETE); /* must not fire again */

    TEST_ASSERT_EQUAL(0, s_notify_count);

    dsp_xfer_set_notify(NULL);
}

void test_xfer_notify_not_called_when_already_transferring(void)
{
    /* START on an already-TRANSFERRING slot stays TRANSFERRING but
     * sm_next returns TRANSFERRING — the notify must NOT fire twice. */
    reset_xfer();
    s_notify_count = 0;
    dsp_xfer_set_notify(probe_notify);

    int idx = dsp_xfer_create(PID);
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);   /* fires once (count=1) */
    s_notify_count = 0;
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);   /* no state change (TRANSFERRING stays) */

    TEST_ASSERT_EQUAL(0, s_notify_count);

    dsp_xfer_set_notify(NULL);
}

/* -------------------------------------------------------------------------
 * 4. HTTP handler registration
 * ------------------------------------------------------------------------- */

void test_xfer_register_handlers_before_init_returns_invalid_state(void)
{
    dsp_xfer_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, dsp_xfer_register_handlers());
}

void test_xfer_register_handlers_after_init_returns_ok(void)
{
    dsp_xfer_deinit();
    dsp_xfer_init();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_xfer_register_handlers());
    dsp_xfer_deinit();
}

void test_xfer_is_initialized_false_after_deinit(void)
{
    dsp_xfer_init();
    dsp_xfer_deinit();
    TEST_ASSERT_FALSE(dsp_xfer_is_initialized());
}
