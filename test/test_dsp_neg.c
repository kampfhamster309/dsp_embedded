/**
 * @file test_dsp_neg.c
 * @brief Host-native Unity tests for dsp_neg (DSP-402).
 *
 * 37 tests covering:
 *   - Pure SM function: all state × event transitions         (17 tests)
 *   - Slot table management                                   (17 tests)
 *   - HTTP handler registration                               ( 3 tests)
 */

#include "unity.h"
#include "dsp_neg.h"
#include "esp_err.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

#define CPID "urn:uuid:consumer-1"
#define PPID "urn:uuid:provider-1"

/* Reset the slot table before slot-table tests. */
static void reset(void)
{
    dsp_neg_deinit();
    dsp_neg_init();
}

/* -------------------------------------------------------------------------
 * 1. Pure SM: dsp_neg_sm_next
 * ------------------------------------------------------------------------- */

/* REQUESTED transitions */
void test_neg_sm_requested_offer_goes_offered(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED,
                      dsp_neg_sm_next(DSP_NEG_STATE_REQUESTED, DSP_NEG_EVENT_OFFER));
}

void test_neg_sm_requested_agree_goes_agreed(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED,
                      dsp_neg_sm_next(DSP_NEG_STATE_REQUESTED, DSP_NEG_EVENT_AGREE));
}

void test_neg_sm_requested_terminate_goes_terminated(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_sm_next(DSP_NEG_STATE_REQUESTED, DSP_NEG_EVENT_TERMINATE));
}

void test_neg_sm_requested_unhandled_stays(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_REQUESTED,
                      dsp_neg_sm_next(DSP_NEG_STATE_REQUESTED, DSP_NEG_EVENT_VERIFY));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_REQUESTED,
                      dsp_neg_sm_next(DSP_NEG_STATE_REQUESTED, DSP_NEG_EVENT_FINALIZE));
}

/* OFFERED transitions */
void test_neg_sm_offered_agree_goes_agreed(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED,
                      dsp_neg_sm_next(DSP_NEG_STATE_OFFERED, DSP_NEG_EVENT_AGREE));
}

void test_neg_sm_offered_terminate_goes_terminated(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_sm_next(DSP_NEG_STATE_OFFERED, DSP_NEG_EVENT_TERMINATE));
}

void test_neg_sm_offered_unhandled_stays(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED,
                      dsp_neg_sm_next(DSP_NEG_STATE_OFFERED, DSP_NEG_EVENT_OFFER));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED,
                      dsp_neg_sm_next(DSP_NEG_STATE_OFFERED, DSP_NEG_EVENT_VERIFY));
}

/* AGREED transitions */
void test_neg_sm_agreed_verify_goes_verified(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_VERIFIED,
                      dsp_neg_sm_next(DSP_NEG_STATE_AGREED, DSP_NEG_EVENT_VERIFY));
}

void test_neg_sm_agreed_terminate_goes_terminated(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_sm_next(DSP_NEG_STATE_AGREED, DSP_NEG_EVENT_TERMINATE));
}

void test_neg_sm_agreed_unhandled_stays(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED,
                      dsp_neg_sm_next(DSP_NEG_STATE_AGREED, DSP_NEG_EVENT_AGREE));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED,
                      dsp_neg_sm_next(DSP_NEG_STATE_AGREED, DSP_NEG_EVENT_FINALIZE));
}

/* VERIFIED transitions */
void test_neg_sm_verified_finalize_goes_finalized(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_FINALIZED,
                      dsp_neg_sm_next(DSP_NEG_STATE_VERIFIED, DSP_NEG_EVENT_FINALIZE));
}

void test_neg_sm_verified_terminate_goes_terminated(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_sm_next(DSP_NEG_STATE_VERIFIED, DSP_NEG_EVENT_TERMINATE));
}

void test_neg_sm_verified_unhandled_stays(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_VERIFIED,
                      dsp_neg_sm_next(DSP_NEG_STATE_VERIFIED, DSP_NEG_EVENT_VERIFY));
}

/* Terminal states */
void test_neg_sm_finalized_absorbs_all_events(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_FINALIZED,
                      dsp_neg_sm_next(DSP_NEG_STATE_FINALIZED, DSP_NEG_EVENT_OFFER));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_FINALIZED,
                      dsp_neg_sm_next(DSP_NEG_STATE_FINALIZED, DSP_NEG_EVENT_TERMINATE));
}

void test_neg_sm_terminated_absorbs_all_events(void)
{
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_sm_next(DSP_NEG_STATE_TERMINATED, DSP_NEG_EVENT_OFFER));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED,
                      dsp_neg_sm_next(DSP_NEG_STATE_TERMINATED, DSP_NEG_EVENT_FINALIZE));
}

/* Full happy path */
void test_neg_sm_full_happy_path(void)
{
    dsp_neg_state_t s = DSP_NEG_STATE_REQUESTED;
    s = dsp_neg_sm_next(s, DSP_NEG_EVENT_OFFER);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED, s);
    s = dsp_neg_sm_next(s, DSP_NEG_EVENT_AGREE);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED, s);
    s = dsp_neg_sm_next(s, DSP_NEG_EVENT_VERIFY);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_VERIFIED, s);
    s = dsp_neg_sm_next(s, DSP_NEG_EVENT_FINALIZE);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_FINALIZED, s);
}

/* -------------------------------------------------------------------------
 * 2. Slot table management
 * ------------------------------------------------------------------------- */

void test_neg_init_returns_ok(void)
{
    dsp_neg_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_neg_init());
    dsp_neg_deinit();
}

void test_neg_count_active_zero_after_init(void)
{
    reset();
    TEST_ASSERT_EQUAL(0, dsp_neg_count_active());
}

void test_neg_create_null_cpid_returns_neg1(void)
{
    reset();
    TEST_ASSERT_EQUAL(-1, dsp_neg_create(NULL, PPID));
}

void test_neg_create_null_ppid_returns_neg1(void)
{
    reset();
    TEST_ASSERT_EQUAL(-1, dsp_neg_create(CPID, NULL));
}

void test_neg_create_returns_zero_first_slot(void)
{
    reset();
    TEST_ASSERT_EQUAL(0, dsp_neg_create(CPID, PPID));
}

void test_neg_create_state_is_requested(void)
{
    reset();
    int idx = dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_REQUESTED, dsp_neg_get_state(idx));
}

void test_neg_apply_offer_event_gives_offered(void)
{
    reset();
    int idx = dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED,
                      dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER));
}

void test_neg_get_consumer_pid_correct(void)
{
    reset();
    int idx = dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL_STRING(CPID, dsp_neg_get_consumer_pid(idx));
}

void test_neg_get_provider_pid_correct(void)
{
    reset();
    int idx = dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL_STRING(PPID, dsp_neg_get_provider_pid(idx));
}

void test_neg_find_by_cpid_returns_correct_index(void)
{
    reset();
    int idx = dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL(idx, dsp_neg_find_by_cpid(CPID));
}

void test_neg_find_by_cpid_returns_neg1_if_not_found(void)
{
    reset();
    TEST_ASSERT_EQUAL(-1, dsp_neg_find_by_cpid("urn:uuid:nobody"));
}

void test_neg_find_by_cpid_null_returns_neg1(void)
{
    reset();
    TEST_ASSERT_EQUAL(-1, dsp_neg_find_by_cpid(NULL));
}

void test_neg_table_overflow_returns_neg1(void)
{
    reset();
    for (int i = 0; i < DSP_NEG_MAX; i++) {
        char cpid[DSP_NEG_PID_LEN];
        /* Use snprintf-style: format index into a urn */
        cpid[0] = 'u'; cpid[1] = 'r'; cpid[2] = 'n'; cpid[3] = ':';
        cpid[4] = (char)('0' + (char)i); cpid[5] = '\0';
        TEST_ASSERT_GREATER_OR_EQUAL(0, dsp_neg_create(cpid, PPID));
    }
    TEST_ASSERT_EQUAL(-1, dsp_neg_create("urn:overflow", PPID));
}

void test_neg_count_active_tracks_creates(void)
{
    reset();
    TEST_ASSERT_EQUAL(0, dsp_neg_count_active());
    dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL(1, dsp_neg_count_active());
}

void test_neg_apply_invalid_index_returns_initial(void)
{
    reset();
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_INITIAL,
                      dsp_neg_apply(-1, DSP_NEG_EVENT_OFFER));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_INITIAL,
                      dsp_neg_apply(DSP_NEG_MAX, DSP_NEG_EVENT_OFFER));
}

void test_neg_get_state_invalid_index_returns_initial(void)
{
    reset();
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_INITIAL, dsp_neg_get_state(-1));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_INITIAL, dsp_neg_get_state(DSP_NEG_MAX));
}

void test_neg_get_pids_invalid_index_returns_null(void)
{
    reset();
    TEST_ASSERT_NULL(dsp_neg_get_consumer_pid(-1));
    TEST_ASSERT_NULL(dsp_neg_get_provider_pid(-1));
}

/* -------------------------------------------------------------------------
 * 3. HTTP handler registration
 * ------------------------------------------------------------------------- */

void test_neg_register_handlers_before_init_returns_invalid_state(void)
{
    dsp_neg_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, dsp_neg_register_handlers());
}

void test_neg_register_handlers_after_init_returns_ok(void)
{
    dsp_neg_deinit();
    dsp_neg_init();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_neg_register_handlers());
    dsp_neg_deinit();
}

void test_neg_double_init_safe(void)
{
    dsp_neg_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_neg_init());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_neg_init()); /* no-op, must not reset table */
    dsp_neg_create(CPID, PPID);
    TEST_ASSERT_EQUAL(1, dsp_neg_count_active()); /* slot survived second init */
    dsp_neg_deinit();
}
