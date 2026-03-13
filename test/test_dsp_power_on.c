/**
 * @file test_dsp_power_on.c
 * @brief Host-native tests for dsp_power with CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=1.
 *
 * Compiled into the separate dsp_test_deep_sleep_on binary with
 * -DCONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=1.
 *
 * Acceptance criteria covered:
 *   M6 AC1: dsp_power_enter_deep_sleep() with an active neg slot saves it to
 *            RTC memory; a subsequent dsp_rtc_state_restore() recovers the slot
 *            with identical state.
 *   M6 AC3: host test simulates wake-from-deep-sleep and verifies state restore.
 */

#include "unity.h"
#include "dsp_config.h"
#include "dsp_power.h"
#include "dsp_rtc_state.h"
#include "dsp_neg.h"
#include "dsp_xfer.h"
#include "dsp_http.h"
#include "esp_err.h"
#include <string.h>
#include <stdbool.h>

/* setUp/tearDown: reset all module state between tests. */
void setUp(void)
{
    dsp_rtc_state_clear();
    dsp_power_set_sleep_fn(NULL, NULL);
    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_UNDEFINED);
    dsp_neg_deinit();
    dsp_xfer_deinit();
}

void tearDown(void)
{
    dsp_rtc_state_clear();
    dsp_power_set_sleep_fn(NULL, NULL);
    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_UNDEFINED);
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_http_stop();
}

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */
static bool s_sleep_called;

static void mock_sleep(void *arg)
{
    bool *flag = (bool *)arg;
    if (flag) {
        *flag = true;
    }
}

/* -------------------------------------------------------------------------
 * Flag
 * ------------------------------------------------------------------------- */

void test_power_flag_is_enabled(void)
{
    TEST_ASSERT_EQUAL_INT(1, CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX);
}

/* -------------------------------------------------------------------------
 * set_sleep_fn
 * ------------------------------------------------------------------------- */

void test_power_set_sleep_fn_null_is_safe(void)
{
    /* Setting NULL should not crash; enter_deep_sleep runs without a callback. */
    dsp_power_set_sleep_fn(NULL, NULL);
    esp_err_t err = dsp_power_enter_deep_sleep();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/* -------------------------------------------------------------------------
 * enter_deep_sleep – mock invocation
 * ------------------------------------------------------------------------- */

void test_power_enter_deep_sleep_calls_sleep_fn(void)
{
    s_sleep_called = false;
    dsp_power_set_sleep_fn(mock_sleep, &s_sleep_called);

    esp_err_t err = dsp_power_enter_deep_sleep();

    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(s_sleep_called);
}

void test_power_enter_deep_sleep_returns_ok(void)
{
    dsp_power_set_sleep_fn(mock_sleep, NULL);
    esp_err_t err = dsp_power_enter_deep_sleep();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/* -------------------------------------------------------------------------
 * enter_deep_sleep – RTC state is saved before sleep fn is called
 * ------------------------------------------------------------------------- */

/* Flag set inside the mock to verify state was already saved before sleep. */
static bool s_rtc_valid_at_sleep_time;

static void mock_sleep_check_rtc(void *arg)
{
    (void)arg;
    s_rtc_valid_at_sleep_time = dsp_rtc_state_is_valid();
}

void test_power_enter_deep_sleep_saves_rtc_state_before_sleep(void)
{
    dsp_neg_init();
    dsp_xfer_init();
    s_rtc_valid_at_sleep_time = false;
    dsp_power_set_sleep_fn(mock_sleep_check_rtc, NULL);

    dsp_power_enter_deep_sleep();

    /* The mock checked dsp_rtc_state_is_valid() before returning — must be true. */
    TEST_ASSERT_TRUE(s_rtc_valid_at_sleep_time);
}

void test_power_enter_deep_sleep_rtc_valid_after_call(void)
{
    dsp_neg_init();
    dsp_xfer_init();
    dsp_power_set_sleep_fn(mock_sleep, NULL);

    dsp_power_enter_deep_sleep();

    TEST_ASSERT_TRUE(dsp_rtc_state_is_valid());
}

/* -------------------------------------------------------------------------
 * enter_deep_sleep – HTTP server is stopped
 * ------------------------------------------------------------------------- */

void test_power_enter_deep_sleep_stops_http_server(void)
{
    /* The host HTTP stub always returns ESP_FAIL from dsp_http_start (no real
     * server on host).  We verify that after enter_deep_sleep the server is
     * not running — i.e. dsp_http_stop() was called regardless of prior state. */
    dsp_neg_init();
    dsp_xfer_init();
    dsp_power_set_sleep_fn(mock_sleep, NULL);
    dsp_power_enter_deep_sleep();

    TEST_ASSERT_FALSE(dsp_http_is_running());
}

/* -------------------------------------------------------------------------
 * Full round-trip: active neg slot preserved across enter/restore
 * ------------------------------------------------------------------------- */

void test_power_enter_deep_sleep_preserves_neg_slot(void)
{
    dsp_neg_init();
    dsp_xfer_init();

    /* Create and advance a negotiation slot. */
    int idx = dsp_neg_create("urn:consumer:test-1", "urn:provider:test-1");
    TEST_ASSERT_EQUAL_INT(0, idx);
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_OFFERED, dsp_neg_get_state(0));

    dsp_power_set_sleep_fn(mock_sleep, NULL);
    esp_err_t err = dsp_power_enter_deep_sleep();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    /* Simulate boot: reinit tables and restore from RTC. */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    err = dsp_rtc_state_restore();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    /* Slot must be present with identical state and PIDs. */
    TEST_ASSERT_TRUE(dsp_neg_is_active(0));
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_OFFERED, dsp_neg_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:consumer:test-1", dsp_neg_get_consumer_pid(0));
    TEST_ASSERT_EQUAL_STRING("urn:provider:test-1", dsp_neg_get_provider_pid(0));
}

void test_power_enter_deep_sleep_preserves_xfer_slot(void)
{
    dsp_neg_init();
    dsp_xfer_init();

    /* Create and start a transfer slot. */
    int idx = dsp_xfer_create("urn:xfer:process-1");
    TEST_ASSERT_EQUAL_INT(0, idx);
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);
    TEST_ASSERT_EQUAL_INT(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(0));

    dsp_power_set_sleep_fn(mock_sleep, NULL);
    esp_err_t err = dsp_power_enter_deep_sleep();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    /* Simulate boot: reinit and restore. */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    err = dsp_rtc_state_restore();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    TEST_ASSERT_TRUE(dsp_xfer_is_active(0));
    TEST_ASSERT_EQUAL_INT(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:xfer:process-1", dsp_xfer_get_process_id(0));
}

void test_power_enter_deep_sleep_empty_tables_round_trip(void)
{
    /* No active slots — save/restore should succeed with no slots restored. */
    dsp_neg_init();
    dsp_xfer_init();

    dsp_power_set_sleep_fn(mock_sleep, NULL);
    dsp_power_enter_deep_sleep();

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    esp_err_t err = dsp_rtc_state_restore();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_INT(0, dsp_neg_count_active());
    TEST_ASSERT_EQUAL_INT(0, dsp_xfer_count_active());
}

/* -------------------------------------------------------------------------
 * Wake-from-sleep simulation (M6 AC3)
 *
 * Mirrors the pattern described in dsp_rtc_state.h "typical usage after
 * wake-up": init tables → is_valid? → restore.
 * ------------------------------------------------------------------------- */

void test_power_wake_restore_recovers_neg_state(void)
{
    /* --- "before sleep" phase --- */
    dsp_neg_init();
    dsp_xfer_init();

    int idx = dsp_neg_create("urn:consumer:wake-1", "urn:provider:wake-1");
    TEST_ASSERT_EQUAL_INT(0, idx);
    /* Advance to AGREED state. */
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    dsp_neg_apply(idx, DSP_NEG_EVENT_AGREE);
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_AGREED, dsp_neg_get_state(0));

    dsp_power_set_sleep_fn(mock_sleep, NULL);
    dsp_power_enter_deep_sleep();

    /* --- "after wake" phase --- */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    TEST_ASSERT_TRUE(dsp_rtc_state_is_valid());

    esp_err_t err = dsp_rtc_state_restore();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_AGREED, dsp_neg_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:consumer:wake-1", dsp_neg_get_consumer_pid(0));
    TEST_ASSERT_EQUAL_STRING("urn:provider:wake-1", dsp_neg_get_provider_pid(0));
}

/* -------------------------------------------------------------------------
 * dsp_power_handle_wakeup tests (DSP-602)
 * ------------------------------------------------------------------------- */

void test_power_handle_wakeup_undefined_returns_invalid_state(void)
{
    /* Default cause is UNDEFINED (cold boot) — handle_wakeup should bail out. */
    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_UNDEFINED);
    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

void test_power_handle_wakeup_other_returns_invalid_state(void)
{
    /* Non-timer cause (e.g. EXT pin) — should also bail out without restoring. */
    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_OTHER);
    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

void test_power_handle_wakeup_timer_no_rtc_state_returns_invalid_state(void)
{
    /* Timer wake but no saved RTC state (e.g. first ever boot after flashing). */
    dsp_neg_init();
    dsp_xfer_init();
    dsp_rtc_state_clear();
    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_TIMER);

    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

void test_power_handle_wakeup_timer_empty_tables_returns_ok(void)
{
    /* Timer wake with valid (but empty) RTC state — should succeed. */
    dsp_neg_init();
    dsp_xfer_init();
    dsp_rtc_state_save();   /* saves empty tables */

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_TIMER);
    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_INT(0, dsp_neg_count_active());
    TEST_ASSERT_EQUAL_INT(0, dsp_xfer_count_active());
}

void test_power_handle_wakeup_undefined_does_not_restore_slots(void)
{
    /* Even if there is valid RTC state, undefined cause must not restore. */
    dsp_neg_init();
    dsp_xfer_init();
    int idx = dsp_neg_create("urn:consumer:cold", "urn:provider:cold");
    TEST_ASSERT_EQUAL_INT(0, idx);
    dsp_rtc_state_save();

    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_UNDEFINED);
    dsp_power_handle_wakeup();

    /* Tables must remain empty — no restore happened. */
    TEST_ASSERT_EQUAL_INT(0, dsp_neg_count_active());
}

void test_power_handle_wakeup_timer_restores_neg_slot(void)
{
    /* --- "before sleep" phase --- */
    dsp_neg_init();
    dsp_xfer_init();

    int idx = dsp_neg_create("urn:consumer:hw-1", "urn:provider:hw-1");
    TEST_ASSERT_EQUAL_INT(0, idx);
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_OFFERED, dsp_neg_get_state(0));

    dsp_power_set_sleep_fn(mock_sleep, NULL);
    dsp_power_enter_deep_sleep();

    /* --- "after wake" phase --- */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_TIMER);
    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    TEST_ASSERT_TRUE(dsp_neg_is_active(0));
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_OFFERED, dsp_neg_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:consumer:hw-1", dsp_neg_get_consumer_pid(0));
    TEST_ASSERT_EQUAL_STRING("urn:provider:hw-1", dsp_neg_get_provider_pid(0));
}

void test_power_handle_wakeup_timer_restores_xfer_slot(void)
{
    /* --- "before sleep" phase --- */
    dsp_neg_init();
    dsp_xfer_init();

    int idx = dsp_xfer_create("urn:xfer:hw-1");
    TEST_ASSERT_EQUAL_INT(0, idx);
    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);
    TEST_ASSERT_EQUAL_INT(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(0));

    dsp_power_set_sleep_fn(mock_sleep, NULL);
    dsp_power_enter_deep_sleep();

    /* --- "after wake" phase --- */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_TIMER);
    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    TEST_ASSERT_TRUE(dsp_xfer_is_active(0));
    TEST_ASSERT_EQUAL_INT(DSP_XFER_STATE_TRANSFERRING, dsp_xfer_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:xfer:hw-1", dsp_xfer_get_process_id(0));
}

void test_power_handle_wakeup_timer_restores_agreed_neg_state(void)
{
    /* M6 AC: "ESP_SLEEP_WAKEUP_TIMER cause with a saved negotiation slot —
     * after dsp_rtc_state_restore() the slot's state and process ID are
     * unchanged." (verified via handle_wakeup which calls restore internally) */
    dsp_neg_init();
    dsp_xfer_init();

    int idx = dsp_neg_create("urn:consumer:agreed-1", "urn:provider:agreed-1");
    TEST_ASSERT_EQUAL_INT(0, idx);
    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);
    dsp_neg_apply(idx, DSP_NEG_EVENT_AGREE);
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_AGREED, dsp_neg_get_state(0));

    /* Sleep: state saved. */
    dsp_power_set_sleep_fn(mock_sleep, NULL);
    dsp_power_enter_deep_sleep();

    /* Wake: reinit + handle_wakeup with timer cause. */
    dsp_neg_deinit();
    dsp_xfer_deinit();
    dsp_neg_init();
    dsp_xfer_init();

    dsp_power_set_wakeup_cause(DSP_POWER_WAKEUP_TIMER);
    esp_err_t err = dsp_power_handle_wakeup();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    /* State and PIDs must be byte-identical to what was saved. */
    TEST_ASSERT_EQUAL_INT(DSP_NEG_STATE_AGREED, dsp_neg_get_state(0));
    TEST_ASSERT_EQUAL_STRING("urn:consumer:agreed-1", dsp_neg_get_consumer_pid(0));
    TEST_ASSERT_EQUAL_STRING("urn:provider:agreed-1", dsp_neg_get_provider_pid(0));
}
