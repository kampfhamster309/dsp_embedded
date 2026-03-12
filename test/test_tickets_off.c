/**
 * @file test_tickets_off.c
 * @brief Unit tests for the TLS session ticket DISABLED path.
 *
 * Compiled into dsp_test_no_tickets with
 * -DCONFIG_DSP_TLS_SESSION_TICKETS=0, which overrides the default-1
 * fallback in dsp_config.h.
 *
 * When the flag is 0, dsp_tls.c omits:
 *   - #include "mbedtls/ssl_ticket.h"
 *   - mbedtls_ssl_ticket_init / setup / conf_session_tickets_cb
 *   - mbedtls_ssl_ticket_free in dsp_tls_server_deinit()
 * and dsp_tls_ctx_t on ESP_PLATFORM omits the ticket_ctx member.
 *
 * Host-native tests can verify:
 *   1. The override is in effect (CONFIG_DSP_TLS_SESSION_TICKETS == 0).
 *   2. dsp_tls_server_init() still returns ESP_FAIL (no mbedTLS on host).
 *   3. dsp_tls_server_deinit() is safe for both initialized states
 *      (ticket_free is entirely absent from the compiled code).
 */

#include "unity.h"
#include "dsp_tls.h"
#include "dsp_config.h"
#include "esp_err.h"

#include <string.h>
#include <stdbool.h>

void setUp(void) {}
void tearDown(void) {}

/* -------------------------------------------------------------------------
 * Flag verification (disabled path)
 * ------------------------------------------------------------------------- */

void test_tickets_off_flag_is_disabled(void)
{
    /* The compile definition -DCONFIG_DSP_TLS_SESSION_TICKETS=0 must win
     * over the #ifndef fallback in dsp_config.h. */
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_TLS_SESSION_TICKETS);
}

/* -------------------------------------------------------------------------
 * Init: still fails on host (mbedTLS absent regardless of ticket flag)
 * ------------------------------------------------------------------------- */

void test_tickets_off_init_null_ctx_returns_invalid_arg(void)
{
    esp_err_t err = dsp_tls_server_init(
        NULL,
        (const unsigned char *)"cert", 4,
        (const unsigned char *)"key",  3,
        "pers");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_tickets_off_init_returns_fail_on_host(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    esp_err_t err = dsp_tls_server_init(
        &ctx,
        (const unsigned char *)"cert", 4,
        (const unsigned char *)"key",  3,
        "dsp_tls");
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}

/* -------------------------------------------------------------------------
 * Deinit: ticket_free absent from binary when flag == 0
 *
 * On ESP_PLATFORM the entire ticket block is preprocessed out, so
 * dsp_tls_server_deinit() never references mbedtls_ssl_ticket_free.
 * On host the stub is identical in both builds; we confirm it is safe.
 * ------------------------------------------------------------------------- */

void test_tickets_off_deinit_null_safe(void)
{
    dsp_tls_server_deinit(NULL);
    TEST_PASS();
}

void test_tickets_off_deinit_not_initialized_safe(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = false;

    dsp_tls_server_deinit(&ctx);
    TEST_ASSERT_FALSE(ctx.initialized);
}

void test_tickets_off_deinit_initialized_clears_flag(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = true;

    dsp_tls_server_deinit(&ctx);
    TEST_ASSERT_FALSE(ctx.initialized);
}

void test_tickets_off_double_deinit_safe(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = true;

    dsp_tls_server_deinit(&ctx);
    dsp_tls_server_deinit(&ctx);
    TEST_ASSERT_FALSE(ctx.initialized);
}
