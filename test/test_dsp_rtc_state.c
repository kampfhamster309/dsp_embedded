/**
 * @file test_dsp_rtc_state.c
 * @brief Unit tests for dsp_rtc_state – RTC memory persistence (DSP-505).
 *
 * DSP-505 AC: A save/restore round-trip through dsp_rtc_state_save() /
 * dsp_rtc_state_restore() produces byte-identical negotiation and transfer
 * state.  Verified for:
 *   - Empty tables (no active slots)
 *   - A single active negotiation slot at a non-zero state
 *   - A single active transfer slot at a non-initial state
 *   - Fully-populated tables (all negotiation + transfer slots active)
 *   - Corruption detection (CRC mismatch → ESP_ERR_INVALID_CRC)
 *   - Missing save (magic wrong → ESP_ERR_INVALID_STATE)
 *
 * On the host build dsp_rtc_state uses a plain static variable as its
 * backing store, so save/restore within the same process is fully testable.
 */

#include "unity.h"
#include "dsp_rtc_state.h"
#include "dsp_neg.h"
#include "dsp_xfer.h"
#include "esp_err.h"

#include <string.h>

/* setUp / tearDown defined once in test_smoke.c. */

/* Helper: reinitialise both modules and clear the RTC store. */
static void reset_all(void)
{
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();
    dsp_rtc_state_clear();
}

/* -------------------------------------------------------------------------
 * Struct layout
 * ------------------------------------------------------------------------- */

void test_rtc_neg_slot_size(void)
{
    TEST_ASSERT_EQUAL(136u, sizeof(dsp_rtc_neg_slot_t));
}

void test_rtc_xfer_slot_size(void)
{
    TEST_ASSERT_EQUAL(72u, sizeof(dsp_rtc_xfer_slot_t));
}

/* -------------------------------------------------------------------------
 * is_valid / clear
 * ------------------------------------------------------------------------- */

void test_rtc_not_valid_after_clear(void)
{
    dsp_rtc_state_clear();
    TEST_ASSERT_FALSE(dsp_rtc_state_is_valid());
}

void test_rtc_valid_after_save(void)
{
    reset_all();
    dsp_rtc_state_save();
    TEST_ASSERT_TRUE(dsp_rtc_state_is_valid());
}

void test_rtc_not_valid_after_save_then_clear(void)
{
    reset_all();
    dsp_rtc_state_save();
    dsp_rtc_state_clear();
    TEST_ASSERT_FALSE(dsp_rtc_state_is_valid());
}

/* -------------------------------------------------------------------------
 * restore without save
 * ------------------------------------------------------------------------- */

void test_rtc_restore_without_save_returns_invalid_state(void)
{
    dsp_rtc_state_clear();
    dsp_neg_init();
    dsp_xfer_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, dsp_rtc_state_restore());
}

/* -------------------------------------------------------------------------
 * Empty table round-trip (DSP-505 AC: empty table)
 * ------------------------------------------------------------------------- */

void test_rtc_empty_table_round_trip(void)
{
    reset_all();
    /* No slots created — save then restore */
    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());
    TEST_ASSERT_TRUE(dsp_rtc_state_is_valid());

    /* Reinitialise modules (simulate wake-up) */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());

    /* Tables must still be empty */
    TEST_ASSERT_EQUAL(0, dsp_neg_count_active());
    TEST_ASSERT_EQUAL(0, dsp_xfer_count_active());
}

/* -------------------------------------------------------------------------
 * Single negotiation slot (DSP-505 AC: single active slot)
 * ------------------------------------------------------------------------- */

void test_rtc_single_neg_slot_round_trip(void)
{
    reset_all();

    /* Create one negotiation in AGREED state */
    int idx = dsp_neg_create("urn:consumer:1", "urn:provider:1");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    dsp_neg_apply(idx, DSP_NEG_EVENT_AGREE);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED, dsp_neg_get_state(idx));

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());

    /* Simulate wake-up: reinitialise empty */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();
    TEST_ASSERT_EQUAL(0, dsp_neg_count_active());

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());

    /* Verify byte-identical state */
    TEST_ASSERT_TRUE(dsp_neg_is_active(idx));
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_AGREED, dsp_neg_get_state(idx));
    TEST_ASSERT_EQUAL_STRING("urn:consumer:1", dsp_neg_get_consumer_pid(idx));
    TEST_ASSERT_EQUAL_STRING("urn:provider:1", dsp_neg_get_provider_pid(idx));
}

void test_rtc_neg_slot_verified_state_preserved(void)
{
    reset_all();

    int idx = dsp_neg_create("urn:consumer:v", "urn:provider:v");
    dsp_neg_apply(idx, DSP_NEG_EVENT_AGREE);
    dsp_neg_apply(idx, DSP_NEG_EVENT_VERIFY);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_VERIFIED, dsp_neg_get_state(idx));

    dsp_rtc_state_save();

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_VERIFIED, dsp_neg_get_state(idx));
}

void test_rtc_neg_slot_terminated_state_preserved(void)
{
    reset_all();

    int idx = dsp_neg_create("urn:consumer:t", "urn:provider:t");
    dsp_neg_apply(idx, DSP_NEG_EVENT_TERMINATE);
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED, dsp_neg_get_state(idx));

    dsp_rtc_state_save();

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_TERMINATED, dsp_neg_get_state(idx));
    TEST_ASSERT_EQUAL_STRING("urn:consumer:t", dsp_neg_get_consumer_pid(idx));
}

/* -------------------------------------------------------------------------
 * Single transfer slot (DSP-505 AC: single active slot)
 * ------------------------------------------------------------------------- */

void test_rtc_single_xfer_slot_round_trip(void)
{
    reset_all();

    int idx = dsp_xfer_create("urn:xfer:process:1");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    /* Advance to TRANSFERRING without notify (no callback set) */
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(idx));

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());

    TEST_ASSERT_TRUE(dsp_xfer_is_active(idx));
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(idx));
    TEST_ASSERT_EQUAL_STRING("urn:xfer:process:1", dsp_xfer_get_process_id(idx));
}

void test_rtc_xfer_completed_state_preserved(void)
{
    reset_all();

    int idx = dsp_xfer_create("urn:xfer:done");
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);
    dsp_xfer_apply(idx, DSP_XFER_EVENT_COMPLETE);
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_COMPLETED, dsp_xfer_get_state(idx));

    dsp_rtc_state_save();

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_COMPLETED, dsp_xfer_get_state(idx));
    TEST_ASSERT_EQUAL_STRING("urn:xfer:done", dsp_xfer_get_process_id(idx));
}

/* -------------------------------------------------------------------------
 * Fully-populated tables (DSP-505 AC: fully-populated table)
 * ------------------------------------------------------------------------- */

void test_rtc_full_neg_table_round_trip(void)
{
    reset_all();

    /* Fill every negotiation slot with a distinct state and PID */
    const dsp_neg_state_t states[] = {
        DSP_NEG_STATE_REQUESTED,
        DSP_NEG_STATE_OFFERED,
        DSP_NEG_STATE_AGREED,
        DSP_NEG_STATE_VERIFIED,
    };
    char cpid[DSP_NEG_MAX][DSP_NEG_PID_LEN];
    char ppid[DSP_NEG_MAX][DSP_NEG_PID_LEN];

    for (int i = 0; i < DSP_NEG_MAX; i++) {
        /* Use snprintf to build unique PIDs — avoids runtime format issues */
        char c[DSP_NEG_PID_LEN];
        char p[DSP_NEG_PID_LEN];
        int n;
        n = 0;
        /* Build "urn:consumer:N" manually */
        memcpy(c, "urn:consumer:", 13u);
        c[13] = (char)('0' + i); c[14] = '\0';
        memcpy(p, "urn:provider:", 13u);
        p[13] = (char)('0' + i); p[14] = '\0';
        memcpy(cpid[i], c, 15u);
        memcpy(ppid[i], p, 15u);
        (void)n;

        int idx = dsp_neg_create(cpid[i], ppid[i]);
        TEST_ASSERT_EQUAL(i, idx);

        /* Advance to the target state */
        switch (states[i]) {
            case DSP_NEG_STATE_OFFERED:
                dsp_neg_apply(i, DSP_NEG_EVENT_OFFER);
                break;
            case DSP_NEG_STATE_AGREED:
                dsp_neg_apply(i, DSP_NEG_EVENT_AGREE);
                break;
            case DSP_NEG_STATE_VERIFIED:
                dsp_neg_apply(i, DSP_NEG_EVENT_AGREE);
                dsp_neg_apply(i, DSP_NEG_EVENT_VERIFY);
                break;
            default:
                break; /* REQUESTED: already in this state after create */
        }
        TEST_ASSERT_EQUAL(states[i], dsp_neg_get_state(i));
    }

    TEST_ASSERT_EQUAL(DSP_NEG_MAX, dsp_neg_count_active());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());

    /* Simulate wake-up */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());

    TEST_ASSERT_EQUAL(DSP_NEG_MAX, dsp_neg_count_active());
    for (int i = 0; i < DSP_NEG_MAX; i++) {
        TEST_ASSERT_EQUAL(states[i], dsp_neg_get_state(i));
        TEST_ASSERT_EQUAL_STRING(cpid[i], dsp_neg_get_consumer_pid(i));
        TEST_ASSERT_EQUAL_STRING(ppid[i], dsp_neg_get_provider_pid(i));
    }
}

void test_rtc_full_xfer_table_round_trip(void)
{
    reset_all();

    const dsp_xfer_state_t states[] = {
        DSP_XFER_STATE_TRANSFERRING,
        DSP_XFER_STATE_COMPLETED,
    };

    for (int i = 0; i < DSP_XFER_MAX; i++) {
        char pid[DSP_XFER_PID_LEN];
        memcpy(pid, "urn:xfer:", 9u);
        pid[9] = (char)('0' + i); pid[10] = '\0';

        int idx = dsp_xfer_create(pid);
        TEST_ASSERT_EQUAL(i, idx);
        dsp_xfer_apply(i, DSP_XFER_EVENT_START);
        if (states[i] == DSP_XFER_STATE_COMPLETED) {
            dsp_xfer_apply(i, DSP_XFER_EVENT_COMPLETE);
        }
        TEST_ASSERT_EQUAL(states[i], dsp_xfer_get_state(i));
    }

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());

    TEST_ASSERT_EQUAL(DSP_XFER_MAX, dsp_xfer_count_active());
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        TEST_ASSERT_EQUAL(states[i], dsp_xfer_get_state(i));
    }
}

/* -------------------------------------------------------------------------
 * Mixed neg + xfer full round-trip
 * ------------------------------------------------------------------------- */

void test_rtc_mixed_neg_and_xfer_round_trip(void)
{
    reset_all();

    int neg0 = dsp_neg_create("urn:c:0", "urn:p:0");
    dsp_neg_apply(neg0, DSP_NEG_EVENT_OFFER);

    int xfer0 = dsp_xfer_create("urn:x:0");
    dsp_xfer_apply(xfer0, DSP_XFER_EVENT_START);

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_restore());

    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED,      dsp_neg_get_state(neg0));
    TEST_ASSERT_EQUAL(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(xfer0));
    TEST_ASSERT_EQUAL_STRING("urn:c:0", dsp_neg_get_consumer_pid(neg0));
    TEST_ASSERT_EQUAL_STRING("urn:x:0", dsp_xfer_get_process_id(xfer0));
}

/* -------------------------------------------------------------------------
 * Save clears stale slots (second save overwrites first)
 * ------------------------------------------------------------------------- */

void test_rtc_second_save_overwrites_first(void)
{
    reset_all();

    /* First save with one neg slot */
    dsp_neg_create("urn:old:consumer", "urn:old:provider");
    dsp_rtc_state_save();

    /* Clear and create a different slot */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();
    int idx = dsp_neg_create("urn:new:consumer", "urn:new:provider");
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    dsp_rtc_state_save();

    /* Restore and verify only the new slot is present */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();
    dsp_rtc_state_restore();

    TEST_ASSERT_EQUAL(1, dsp_neg_count_active());
    TEST_ASSERT_EQUAL(DSP_NEG_STATE_OFFERED, dsp_neg_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:new:consumer", dsp_neg_get_consumer_pid(0));
}

/* -------------------------------------------------------------------------
 * load_slot API (called by restore; also testable directly)
 * ------------------------------------------------------------------------- */

void test_neg_load_slot_null_cpid_returns_invalid_arg(void)
{
    dsp_neg_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_neg_load_slot(0, DSP_NEG_STATE_AGREED, NULL, "urn:p"));
    dsp_neg_deinit();
}

void test_neg_load_slot_null_ppid_returns_invalid_arg(void)
{
    dsp_neg_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_neg_load_slot(0, DSP_NEG_STATE_AGREED, "urn:c", NULL));
    dsp_neg_deinit();
}

void test_neg_load_slot_bad_idx_returns_invalid_arg(void)
{
    dsp_neg_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_neg_load_slot(-1, DSP_NEG_STATE_AGREED, "urn:c", "urn:p"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_neg_load_slot(DSP_NEG_MAX, DSP_NEG_STATE_AGREED, "urn:c", "urn:p"));
    dsp_neg_deinit();
}

void test_neg_load_slot_not_initialized_returns_invalid_state(void)
{
    dsp_neg_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
        dsp_neg_load_slot(0, DSP_NEG_STATE_AGREED, "urn:c", "urn:p"));
}

void test_xfer_load_slot_null_pid_returns_invalid_arg(void)
{
    dsp_xfer_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_xfer_load_slot(0, DSP_XFER_STATE_TRANSFERRING, NULL));
    dsp_xfer_deinit();
}

void test_xfer_load_slot_bad_idx_returns_invalid_arg(void)
{
    dsp_xfer_init();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_xfer_load_slot(-1, DSP_XFER_STATE_TRANSFERRING, "urn:p"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_xfer_load_slot(DSP_XFER_MAX, DSP_XFER_STATE_TRANSFERRING, "urn:p"));
    dsp_xfer_deinit();
}

void test_xfer_load_slot_not_initialized_returns_invalid_state(void)
{
    dsp_xfer_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
        dsp_xfer_load_slot(0, DSP_XFER_STATE_TRANSFERRING, "urn:p"));
}

/* -------------------------------------------------------------------------
 * DSP-801: CRC mismatch detection (valid magic, corrupted CRC)
 * ------------------------------------------------------------------------- */

void test_rtc_crc_mismatch_returns_invalid_crc(void)
{
    reset_all();

    /* Save a valid state so magic and CRC are set correctly. */
    dsp_neg_create("urn:c:crc-test", "urn:p:crc-test");
    TEST_ASSERT_EQUAL(ESP_OK, dsp_rtc_state_save());
    TEST_ASSERT_TRUE(dsp_rtc_state_is_valid());

    /* Corrupt the stored CRC without touching the magic sentinel. */
    dsp_rtc_state_corrupt_crc_for_testing();

    /* Restore must detect the mismatch and return INVALID_CRC. */
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_CRC, dsp_rtc_state_restore());
}
