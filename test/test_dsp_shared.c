/**
 * @file test_dsp_shared.c
 * @brief Unit tests for dsp_shared – struct layout and ring buffer API (DSP-501).
 *
 * These tests cover the DSP-501 AC:
 *   - Struct sizes are deterministic (static_asserts pass at compile time;
 *     runtime checks mirror them for explicit test failure messages).
 *   - dsp_shared_init() / dsp_shared_deinit() lifecycle.
 *   - Ring buffer: push/pop argument validation, 1-item round-trip,
 *     capacity fill without loss, and overflow (drop-newest) behaviour.
 *
 * The full ring-buffer stress tests (DSP-504 AC: capacity, overflow policy
 * across all slots) will be added in test_dsp_ring_buf.c.
 */

#include "unity.h"
#include "dsp_shared.h"
#include "esp_err.h"

#include <string.h>
#include <stddef.h>
#include <stdint.h>

/* setUp / tearDown defined once in test_smoke.c. */

/* -------------------------------------------------------------------------
 * Compile-time layout verification (mirrored as runtime assertions)
 * ------------------------------------------------------------------------- */

void test_shared_sample_size(void)
{
    TEST_ASSERT_EQUAL(16u, sizeof(dsp_sample_t));
}

void test_shared_sample_offsets(void)
{
    TEST_ASSERT_EQUAL(0u,  offsetof(dsp_sample_t, timestamp_us));
    TEST_ASSERT_EQUAL(8u,  offsetof(dsp_sample_t, raw_value));
    TEST_ASSERT_EQUAL(12u, offsetof(dsp_sample_t, channel));
}

void test_shared_ring_buf_size(void)
{
    TEST_ASSERT_EQUAL(528u, sizeof(dsp_ring_buf_t));
}

void test_shared_ring_buf_offsets(void)
{
    TEST_ASSERT_EQUAL(0u,   offsetof(dsp_ring_buf_t, data));
    TEST_ASSERT_EQUAL(512u, offsetof(dsp_ring_buf_t, head));
    TEST_ASSERT_EQUAL(516u, offsetof(dsp_ring_buf_t, tail));
    TEST_ASSERT_EQUAL(520u, offsetof(dsp_ring_buf_t, count));
}

void test_shared_ring_buf_at_offset_zero_in_shared(void)
{
    TEST_ASSERT_EQUAL(0u, offsetof(dsp_shared_t, ring_buf));
}

void test_shared_capacity_is_power_of_two(void)
{
    TEST_ASSERT_EQUAL(0u, DSP_RING_BUF_CAPACITY & (DSP_RING_BUF_CAPACITY - 1u));
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

void test_shared_init_null_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_shared_init(NULL));
}

void test_shared_init_returns_ok(void)
{
    dsp_shared_t sh;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_shared_init(&sh));
    dsp_shared_deinit(&sh);
}

void test_shared_init_zeroes_ring_buf(void)
{
    dsp_shared_t sh;
    /* Poison the struct before init */
    memset(&sh, 0xFF, sizeof(sh));
    dsp_shared_init(&sh);
    TEST_ASSERT_EQUAL(0u, sh.ring_buf.head);
    TEST_ASSERT_EQUAL(0u, sh.ring_buf.tail);
    TEST_ASSERT_EQUAL(0u, sh.ring_buf.count);
    dsp_shared_deinit(&sh);
}

void test_shared_init_zeroes_ready_flags(void)
{
    dsp_shared_t sh;
    memset(&sh, 0xFF, sizeof(sh));
    dsp_shared_init(&sh);
    TEST_ASSERT_EQUAL(0u, sh.core0_ready);
    TEST_ASSERT_EQUAL(0u, sh.core1_ready);
    dsp_shared_deinit(&sh);
}

void test_shared_deinit_null_safe(void)
{
    dsp_shared_deinit(NULL);
    TEST_PASS();
}

void test_shared_double_init_safe(void)
{
    dsp_shared_t sh;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_shared_init(&sh));
    TEST_ASSERT_EQUAL(ESP_OK, dsp_shared_init(&sh));
    dsp_shared_deinit(&sh);
}

void test_shared_double_deinit_safe(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    dsp_shared_deinit(&sh);
    dsp_shared_deinit(&sh);
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * dsp_ring_buf_push / dsp_ring_buf_pop argument validation
 * ------------------------------------------------------------------------- */

void test_ring_push_null_sh_returns_invalid_arg(void)
{
    dsp_sample_t s = {0};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_ring_buf_push(NULL, &s));
}

void test_ring_push_null_sample_returns_invalid_arg(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_ring_buf_push(&sh, NULL));
    dsp_shared_deinit(&sh);
}

void test_ring_pop_null_sh_returns_invalid_arg(void)
{
    dsp_sample_t s;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_ring_buf_pop(NULL, &s));
}

void test_ring_pop_null_out_returns_invalid_arg(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_ring_buf_pop(&sh, NULL));
    dsp_shared_deinit(&sh);
}

void test_ring_count_null_returns_zero(void)
{
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(NULL));
}

/* -------------------------------------------------------------------------
 * Ring buffer behaviour
 * ------------------------------------------------------------------------- */

void test_ring_empty_pop_returns_not_found(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    dsp_sample_t out;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, dsp_ring_buf_pop(&sh, &out));
    dsp_shared_deinit(&sh);
}

void test_ring_count_zero_after_init(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&sh));
    dsp_shared_deinit(&sh);
}

void test_ring_one_item_round_trip(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    dsp_sample_t in  = { .timestamp_us = 1000u, .raw_value = 42u, .channel = 2u };
    dsp_sample_t out = {0};

    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &in));
    TEST_ASSERT_EQUAL(1u,     dsp_ring_buf_count(&sh));
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&sh, &out));
    TEST_ASSERT_EQUAL(0u,     dsp_ring_buf_count(&sh));

    TEST_ASSERT_EQUAL(1000u, out.timestamp_us);
    TEST_ASSERT_EQUAL(42u,   out.raw_value);
    TEST_ASSERT_EQUAL(2u,    out.channel);

    dsp_shared_deinit(&sh);
}

void test_ring_fifo_order(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0; i < 4u; i++) {
        dsp_sample_t s = { .raw_value = i };
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }
    for (uint32_t i = 0; i < 4u; i++) {
        dsp_sample_t out;
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&sh, &out));
        TEST_ASSERT_EQUAL(i, out.raw_value);
    }

    dsp_shared_deinit(&sh);
}

void test_ring_fill_to_capacity_no_loss(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = { .raw_value = i };
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, dsp_ring_buf_count(&sh));

    for (uint32_t i = 0; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t out;
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&sh, &out));
        TEST_ASSERT_EQUAL(i, out.raw_value);
    }
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&sh));

    dsp_shared_deinit(&sh);
}

void test_ring_overflow_drops_newest(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    /* Fill to capacity */
    for (uint32_t i = 0; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = { .raw_value = i };
        dsp_ring_buf_push(&sh, &s);
    }

    /* One more push must be dropped (DROP NEWEST policy) */
    dsp_sample_t overflow = { .raw_value = 0xDEADBEEFu };
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, dsp_ring_buf_push(&sh, &overflow));

    /* Count must still be at capacity */
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, dsp_ring_buf_count(&sh));

    /* Oldest items (0..CAPACITY-1) must still be intact */
    for (uint32_t i = 0; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t out;
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&sh, &out));
        TEST_ASSERT_EQUAL(i, out.raw_value);
    }

    dsp_shared_deinit(&sh);
}

void test_ring_wrap_around(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    /* Fill half, drain half, fill again — exercises the wrap */
    uint32_t half = DSP_RING_BUF_CAPACITY / 2u;

    for (uint32_t i = 0; i < half; i++) {
        dsp_sample_t s = { .raw_value = i };
        dsp_ring_buf_push(&sh, &s);
    }
    for (uint32_t i = 0; i < half; i++) {
        dsp_sample_t out;
        dsp_ring_buf_pop(&sh, &out);
    }
    for (uint32_t i = 0; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = { .raw_value = 100u + i };
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }
    for (uint32_t i = 0; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t out;
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_pop(&sh, &out));
        TEST_ASSERT_EQUAL(100u + i, out.raw_value);
    }

    dsp_shared_deinit(&sh);
}
