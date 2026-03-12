/**
 * @file test_dsp_tls_tickets.c
 * @brief Unit tests for the TLS session ticket ENABLED path.
 *
 * Compiled into dsp_test_runner with the default host flags, where
 * CONFIG_DSP_TLS_SESSION_TICKETS == 1 (set in dsp_config.h fallback).
 *
 * These tests verify:
 *   1. The feature flag is actually enabled in this build.
 *   2. dsp_tls_server_init() returns ESP_FAIL on host even with tickets on
 *      (mbedTLS not linked — the flag path cannot succeed here).
 *   3. dsp_tls_server_deinit() is safe when ctx->initialized == false
 *      (the ticket_free guarded by if(ctx->initialized) is skipped).
 *   4. dsp_tls_server_deinit() clears initialized when it was true
 *      (ticket_free would run on ESP_PLATFORM; on host the stub just
 *      resets the flag).
 *
 * The disabled path is covered by the separate dsp_test_no_tickets binary.
 */

#include "unity.h"
#include "dsp_tls.h"
#include "dsp_config.h"
#include "esp_err.h"

#include <string.h>
#include <stdbool.h>

/* setUp / tearDown are defined in test_smoke.c. */

/* -------------------------------------------------------------------------
 * Flag verification (enabled path)
 * ------------------------------------------------------------------------- */

void test_tickets_flag_is_enabled(void)
{
    /* This build must have tickets on (default = 1 from dsp_config.h). */
    TEST_ASSERT_EQUAL(1, CONFIG_DSP_TLS_SESSION_TICKETS);
}

/* -------------------------------------------------------------------------
 * Init path: tickets enabled but mbedTLS absent on host
 * ------------------------------------------------------------------------- */

void test_tickets_enabled_init_still_fails_on_host(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* mbedTLS is not linked → stub always returns ESP_FAIL regardless of
     * whether session tickets are compiled in. */
    esp_err_t err = dsp_tls_server_init(
        &ctx,
        (const unsigned char *)"cert", 4,
        (const unsigned char *)"key",  3,
        "dsp_tls");
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}

/* -------------------------------------------------------------------------
 * Deinit path: ticket_free branch gated by ctx->initialized
 *
 * On ESP_PLATFORM, when CONFIG_DSP_TLS_SESSION_TICKETS == 1:
 *
 *   if (ctx->initialized) {
 *       mbedtls_ssl_ticket_free(&ctx->ticket_ctx);  ← runs only here
 *   }
 *
 * On host the stub unconditionally sets initialized = false.
 * These tests confirm the observable outcomes are correct in both cases.
 * ------------------------------------------------------------------------- */

void test_tickets_deinit_initialized_false_is_safe(void)
{
    /* initialized == false → ticket_free is skipped on ESP_PLATFORM.
     * On host: stub accepts the context without crashing. */
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = false;

    dsp_tls_server_deinit(&ctx);   /* must not crash */
    TEST_ASSERT_FALSE(ctx.initialized);
}

void test_tickets_deinit_initialized_true_clears_flag(void)
{
    /* initialized == true → ticket_free would run on ESP_PLATFORM.
     * On host: stub sets initialized = false. */
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = true;

    dsp_tls_server_deinit(&ctx);
    TEST_ASSERT_FALSE(ctx.initialized);
}

void test_tickets_deinit_null_is_safe(void)
{
    dsp_tls_server_deinit(NULL);   /* must not crash */
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * Idempotence: double deinit must not crash
 * ------------------------------------------------------------------------- */

void test_tickets_double_deinit_safe(void)
{
    dsp_tls_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.initialized = true;

    dsp_tls_server_deinit(&ctx);
    TEST_ASSERT_FALSE(ctx.initialized);

    dsp_tls_server_deinit(&ctx);   /* second call: initialized already false */
    TEST_ASSERT_FALSE(ctx.initialized);
}
