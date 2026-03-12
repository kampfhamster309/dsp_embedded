/**
 * @file test_dsp_wifi.c
 * @brief Host-native unit tests for the dsp_wifi component.
 *
 * Tests are divided into two groups:
 *
 * 1. State machine (dsp_wifi_sm_next) – the SM is platform-agnostic and
 *    compiled unconditionally, so every transition can be exercised on host.
 *
 * 2. Stub API – verifies argument validation and host-stub sentinel values
 *    for the platform functions (init, connect, disconnect, etc.).
 */

#include "unity.h"
#include "dsp_wifi.h"
#include "dsp_config.h"
#include "esp_err.h"

#include <string.h>

/* setUp / tearDown defined in test_smoke.c. */

/* -------------------------------------------------------------------------
 * Kconfig default tests
 * ------------------------------------------------------------------------- */

void test_wifi_max_retry_default(void)
{
    TEST_ASSERT_EQUAL(5, CONFIG_DSP_WIFI_MAX_RETRY);
    TEST_ASSERT_GREATER_OR_EQUAL(1,  CONFIG_DSP_WIFI_MAX_RETRY);
    TEST_ASSERT_LESS_OR_EQUAL(20, CONFIG_DSP_WIFI_MAX_RETRY);
}

void test_wifi_reconnect_delay_default(void)
{
    TEST_ASSERT_EQUAL(1000, CONFIG_DSP_WIFI_RECONNECT_DELAY_MS);
    TEST_ASSERT_GREATER_OR_EQUAL(100,   CONFIG_DSP_WIFI_RECONNECT_DELAY_MS);
    TEST_ASSERT_LESS_OR_EQUAL(30000, CONFIG_DSP_WIFI_RECONNECT_DELAY_MS);
}

/* -------------------------------------------------------------------------
 * State machine: IDLE transitions
 * ------------------------------------------------------------------------- */

void test_sm_idle_connect_goes_connecting(void)
{
    int retry = 0;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_CONNECT, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTING, next);
    TEST_ASSERT_EQUAL(0, retry);   /* reset on fresh connect */
}

void test_sm_idle_unhandled_inputs_stay_idle(void)
{
    int retry = 0;
    /* CONNECTED/DISCONNECTED/RETRY while IDLE are ignored */
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_CONNECTED,    &retry, 5));
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5));
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_RETRY,        &retry, 5));
}

/* -------------------------------------------------------------------------
 * State machine: CONNECTING transitions
 * ------------------------------------------------------------------------- */

void test_sm_connecting_connected_goes_connected(void)
{
    int retry = 2; /* pre-existing retries reset on success */
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_CONNECTING, DSP_WIFI_INPUT_CONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTED, next);
    TEST_ASSERT_EQUAL(0, retry);
}

void test_sm_connecting_disconnected_increments_retry(void)
{
    int retry = 0;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_CONNECTING, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_RECONNECTING, next);
    TEST_ASSERT_EQUAL(1, retry);
}

void test_sm_connecting_disconnected_at_max_retry_goes_failed(void)
{
    int retry = 4; /* already at max-1 */
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_CONNECTING, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED, next);
    TEST_ASSERT_EQUAL(5, retry);
}

/* -------------------------------------------------------------------------
 * State machine: CONNECTED transitions
 * ------------------------------------------------------------------------- */

void test_sm_connected_disconnected_goes_reconnecting(void)
{
    int retry = 0;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_CONNECTED, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_RECONNECTING, next);
    TEST_ASSERT_EQUAL(1, retry);
}

void test_sm_connected_disconnected_at_max_retry_goes_failed(void)
{
    int retry = 4;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_CONNECTED, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED, next);
}

void test_sm_connected_unhandled_stays_connected(void)
{
    int retry = 0;
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTED,
        dsp_wifi_sm_next(DSP_WIFI_STATE_CONNECTED, DSP_WIFI_INPUT_RETRY, &retry, 5));
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTED,
        dsp_wifi_sm_next(DSP_WIFI_STATE_CONNECTED, DSP_WIFI_INPUT_CONNECT, &retry, 5));
}

/* -------------------------------------------------------------------------
 * State machine: RECONNECTING transitions
 * ------------------------------------------------------------------------- */

void test_sm_reconnecting_retry_goes_connecting(void)
{
    int retry = 1;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_RECONNECTING, DSP_WIFI_INPUT_RETRY, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTING, next);
    TEST_ASSERT_EQUAL(1, retry); /* retry not reset by RETRY input */
}

void test_sm_reconnecting_connected_goes_connected(void)
{
    int retry = 2;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_RECONNECTING, DSP_WIFI_INPUT_CONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTED, next);
    TEST_ASSERT_EQUAL(0, retry);
}

void test_sm_reconnecting_extra_disconnect_increments_retry(void)
{
    int retry = 2;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_RECONNECTING, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_RECONNECTING, next);
    TEST_ASSERT_EQUAL(3, retry);
}

void test_sm_reconnecting_extra_disconnect_at_max_goes_failed(void)
{
    int retry = 4;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_RECONNECTING, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED, next);
}

/* -------------------------------------------------------------------------
 * State machine: FAILED transitions
 * ------------------------------------------------------------------------- */

void test_sm_failed_stays_failed_on_all_inputs(void)
{
    int retry = 5;
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED,
        dsp_wifi_sm_next(DSP_WIFI_STATE_FAILED, DSP_WIFI_INPUT_CONNECT,      &retry, 5));
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED,
        dsp_wifi_sm_next(DSP_WIFI_STATE_FAILED, DSP_WIFI_INPUT_CONNECTED,    &retry, 5));
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED,
        dsp_wifi_sm_next(DSP_WIFI_STATE_FAILED, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5));
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED,
        dsp_wifi_sm_next(DSP_WIFI_STATE_FAILED, DSP_WIFI_INPUT_RETRY,        &retry, 5));
}

/* -------------------------------------------------------------------------
 * State machine: DISCONNECT from any state → IDLE
 * ------------------------------------------------------------------------- */

void test_sm_disconnect_from_idle_stays_idle(void)
{
    int retry = 3;
    dsp_wifi_state_t next = dsp_wifi_sm_next(
        DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_DISCONNECT, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE, next);
    TEST_ASSERT_EQUAL(0, retry);
}

void test_sm_disconnect_from_connecting_goes_idle(void)
{
    int retry = 2;
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_CONNECTING, DSP_WIFI_INPUT_DISCONNECT, &retry, 5));
    TEST_ASSERT_EQUAL(0, retry);
}

void test_sm_disconnect_from_connected_goes_idle(void)
{
    int retry = 0;
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_CONNECTED, DSP_WIFI_INPUT_DISCONNECT, &retry, 5));
}

void test_sm_disconnect_from_reconnecting_goes_idle(void)
{
    int retry = 3;
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_RECONNECTING, DSP_WIFI_INPUT_DISCONNECT, &retry, 5));
    TEST_ASSERT_EQUAL(0, retry);
}

void test_sm_disconnect_from_failed_goes_idle(void)
{
    int retry = 5;
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE,
        dsp_wifi_sm_next(DSP_WIFI_STATE_FAILED, DSP_WIFI_INPUT_DISCONNECT, &retry, 5));
    TEST_ASSERT_EQUAL(0, retry);
}

/* -------------------------------------------------------------------------
 * State machine: full happy-path sequence
 * IDLE → CONNECTING → CONNECTED → RECONNECTING → CONNECTING → CONNECTED
 * ------------------------------------------------------------------------- */

void test_sm_full_happy_path(void)
{
    int retry = 0;
    dsp_wifi_state_t s;

    s = dsp_wifi_sm_next(DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_CONNECT, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTING, s);

    s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_CONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTED, s);
    TEST_ASSERT_EQUAL(0, retry);

    s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_DISCONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_RECONNECTING, s);
    TEST_ASSERT_EQUAL(1, retry);

    s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_RETRY, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTING, s);

    s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_CONNECTED, &retry, 5);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTED, s);
    TEST_ASSERT_EQUAL(0, retry);
}

/* -------------------------------------------------------------------------
 * State machine: exhausted-retry sequence → FAILED
 * ------------------------------------------------------------------------- */

void test_sm_exhausted_retries_reach_failed(void)
{
    int retry = 0;
    dsp_wifi_state_t s;

    s = dsp_wifi_sm_next(DSP_WIFI_STATE_IDLE, DSP_WIFI_INPUT_CONNECT, &retry, 3);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTING, s);

    /* 3 consecutive failures */
    for (int i = 1; i <= 2; i++) {
        s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_DISCONNECTED, &retry, 3);
        TEST_ASSERT_EQUAL(DSP_WIFI_STATE_RECONNECTING, s);
        TEST_ASSERT_EQUAL(i, retry);

        s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_RETRY, &retry, 3);
        TEST_ASSERT_EQUAL(DSP_WIFI_STATE_CONNECTING, s);
    }

    /* Final failure hits max_retry */
    s = dsp_wifi_sm_next(s, DSP_WIFI_INPUT_DISCONNECTED, &retry, 3);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_FAILED, s);
    TEST_ASSERT_EQUAL(3, retry);
}

/* -------------------------------------------------------------------------
 * Stub API tests
 * ------------------------------------------------------------------------- */

void test_wifi_init_returns_ok_on_host(void)
{
    /* Host stub always succeeds so post-init paths can be tested. */
    esp_err_t err = dsp_wifi_init(NULL);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_wifi_get_state_idle_after_init(void)
{
    dsp_wifi_init(NULL);
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE, dsp_wifi_get_state());
}

void test_wifi_connect_returns_fail_on_host(void)
{
    dsp_wifi_init(NULL);
    esp_err_t err = dsp_wifi_connect();
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}

void test_wifi_disconnect_safe_in_idle(void)
{
    dsp_wifi_init(NULL);
    dsp_wifi_disconnect();   /* must not crash */
    TEST_ASSERT_EQUAL(DSP_WIFI_STATE_IDLE, dsp_wifi_get_state());
}

void test_wifi_store_null_ssid_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_wifi_store_credentials(NULL, "password"));
}

void test_wifi_store_empty_ssid_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_wifi_store_credentials("", "password"));
}

void test_wifi_store_null_password_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        dsp_wifi_store_credentials("myssid", NULL));
}

void test_wifi_store_valid_args_fails_on_host(void)
{
    /* Valid args but no NVS on host → ESP_FAIL */
    TEST_ASSERT_EQUAL(ESP_FAIL,
        dsp_wifi_store_credentials("myssid", "mypassword"));
}

void test_wifi_set_event_cb_null_clears(void)
{
    /* Should not crash */
    dsp_wifi_set_event_cb(NULL, NULL);
    TEST_PASS();
}

void test_wifi_deinit_safe(void)
{
    dsp_wifi_init(NULL);
    dsp_wifi_deinit();
    TEST_PASS();
}
