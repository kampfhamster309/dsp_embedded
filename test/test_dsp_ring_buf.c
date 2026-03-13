/**
 * @file test_dsp_ring_buf.c
 * @brief Ring buffer producer/consumer tests (DSP-504).
 *
 * Complements test_dsp_shared.c (which covers DSP-501 basics: struct layout,
 * push/pop argument validation, 1-item round-trip, and wrap-around).
 *
 * DSP-504 AC coverage:
 *   - Exactly 1 item: produce then consume (extended drain variant).
 *   - Capacity items without loss across push / drain cycles.
 *   - Overflow: item at capacity+1 is dropped (DROP NEWEST policy documented
 *     in dsp_shared.h); buffer retains the original capacity items intact.
 *   - dsp_ring_buf_drain(): argument validation, partial drain, full drain,
 *     drain bounded by capacity, drain over multiple wrap-around cycles.
 */

#include "unity.h"
#include "dsp_shared.h"
#include "esp_err.h"

#include <string.h>

/* setUp / tearDown defined once in test_smoke.c. */

/* Convenience: build a sample with a recognisable raw_value. */
static dsp_sample_t make_sample(uint32_t val)
{
    dsp_sample_t s;
    memset(&s, 0, sizeof(s));
    s.raw_value = val;
    return s;
}

/* -------------------------------------------------------------------------
 * dsp_ring_buf_drain – argument validation
 * ------------------------------------------------------------------------- */

void test_drain_null_sh_returns_invalid_arg(void)
{
    dsp_sample_t buf[1];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_ring_buf_drain(NULL, buf, 1u, &got));
}

void test_drain_null_buf_returns_invalid_arg(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_ring_buf_drain(&sh, NULL, 1u, &got));
    dsp_shared_deinit(&sh);
}

void test_drain_capacity_zero_is_noop(void)
{
    /* capacity=0 with NULL sh and NULL buf must still return OK (no access). */
    uint32_t got = 99u;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(NULL, NULL, 0u, &got));
    TEST_ASSERT_EQUAL(0u, got);
}

void test_drain_null_out_count_accepted(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);
    dsp_sample_t s = make_sample(7u);
    dsp_ring_buf_push(&sh, &s);
    dsp_sample_t buf[1];
    /* Passing NULL for out_count must not crash. */
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 1u, NULL));
    dsp_shared_deinit(&sh);
}

/* -------------------------------------------------------------------------
 * dsp_ring_buf_drain – single-item behaviour (DSP-504 AC: 1-item produce/consume)
 * ------------------------------------------------------------------------- */

void test_drain_one_item(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    dsp_sample_t in = make_sample(42u);
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &in));

    dsp_sample_t buf[1];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 1u, &got));
    TEST_ASSERT_EQUAL(1u, got);
    TEST_ASSERT_EQUAL(42u, buf[0].raw_value);
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&sh));

    dsp_shared_deinit(&sh);
}

void test_drain_empty_returns_zero(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    dsp_sample_t buf[4];
    uint32_t got = 99u;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 4u, &got));
    TEST_ASSERT_EQUAL(0u, got);

    dsp_shared_deinit(&sh);
}

/* -------------------------------------------------------------------------
 * dsp_ring_buf_drain – partial and bounded drains
 * ------------------------------------------------------------------------- */

void test_drain_partial_leaves_remainder(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0u; i < 8u; i++) {
        dsp_sample_t s = make_sample(i);
        dsp_ring_buf_push(&sh, &s);
    }

    dsp_sample_t buf[4];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 4u, &got));
    TEST_ASSERT_EQUAL(4u, got);
    /* First 4 should be 0..3 (FIFO order). */
    for (uint32_t i = 0u; i < 4u; i++) {
        TEST_ASSERT_EQUAL(i, buf[i].raw_value);
    }
    /* 4 items still in buffer. */
    TEST_ASSERT_EQUAL(4u, dsp_ring_buf_count(&sh));

    dsp_shared_deinit(&sh);
}

void test_drain_bounded_by_capacity_param(void)
{
    /* Push 3 items but drain with capacity=2; only 2 should come out. */
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0u; i < 3u; i++) {
        dsp_sample_t s = make_sample(i);
        dsp_ring_buf_push(&sh, &s);
    }

    dsp_sample_t buf[2];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 2u, &got));
    TEST_ASSERT_EQUAL(2u, got);
    TEST_ASSERT_EQUAL(1u, dsp_ring_buf_count(&sh));

    dsp_shared_deinit(&sh);
}

void test_drain_more_capacity_than_items(void)
{
    /* Request more than available; drain returns only what exists. */
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0u; i < 5u; i++) {
        dsp_sample_t s = make_sample(i);
        dsp_ring_buf_push(&sh, &s);
    }

    dsp_sample_t buf[32];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 32u, &got));
    TEST_ASSERT_EQUAL(5u, got);
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&sh));

    dsp_shared_deinit(&sh);
}

/* -------------------------------------------------------------------------
 * dsp_ring_buf_drain – full capacity (DSP-504 AC: capacity items without loss)
 * ------------------------------------------------------------------------- */

void test_drain_full_capacity_no_loss(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = make_sample(i);
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, dsp_ring_buf_count(&sh));

    dsp_sample_t buf[DSP_RING_BUF_CAPACITY];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_ring_buf_drain(&sh, buf, DSP_RING_BUF_CAPACITY, &got));
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, got);
    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&sh));

    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        TEST_ASSERT_EQUAL(i, buf[i].raw_value);
    }

    dsp_shared_deinit(&sh);
}

/* -------------------------------------------------------------------------
 * Overflow: DSP-504 AC – item at capacity+1 is dropped (DROP NEWEST policy)
 * Already tested in test_dsp_shared.c via dsp_ring_buf_pop; here we verify
 * the same invariant when draining.
 * ------------------------------------------------------------------------- */

void test_drain_after_overflow_retains_original_items(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    /* Fill to capacity */
    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = make_sample(i);
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }

    /* Push one more – must be dropped */
    dsp_sample_t overflow = make_sample(0xDEADBEEFu);
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, dsp_ring_buf_push(&sh, &overflow));

    /* Drain everything; original capacity items must be intact */
    dsp_sample_t buf[DSP_RING_BUF_CAPACITY + 1u];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_ring_buf_drain(&sh, buf, DSP_RING_BUF_CAPACITY + 1u, &got));
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, got);
    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        TEST_ASSERT_EQUAL(i, buf[i].raw_value);
    }

    dsp_shared_deinit(&sh);
}

/* -------------------------------------------------------------------------
 * Drain across wrap-around
 * ------------------------------------------------------------------------- */

void test_drain_wrap_around(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    uint32_t half = DSP_RING_BUF_CAPACITY / 2u;

    /* Push and drain half to move the head/tail pointers past the midpoint */
    for (uint32_t i = 0u; i < half; i++) {
        dsp_sample_t s = make_sample(i);
        dsp_ring_buf_push(&sh, &s);
    }
    dsp_sample_t discard[DSP_RING_BUF_CAPACITY / 2u];
    uint32_t discarded = 0u;
    dsp_ring_buf_drain(&sh, discard, half, &discarded);
    TEST_ASSERT_EQUAL(half, discarded);

    /* Now fill to capacity – entries will wrap across the array boundary */
    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = make_sample(200u + i);
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }

    /* Drain all and verify FIFO order is preserved across the wrap */
    dsp_sample_t buf[DSP_RING_BUF_CAPACITY];
    uint32_t got = 0u;
    TEST_ASSERT_EQUAL(ESP_OK,
        dsp_ring_buf_drain(&sh, buf, DSP_RING_BUF_CAPACITY, &got));
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, got);
    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        TEST_ASSERT_EQUAL(200u + i, buf[i].raw_value);
    }

    dsp_shared_deinit(&sh);
}

/* -------------------------------------------------------------------------
 * Producer/consumer integration: interleaved push and drain cycles
 * ------------------------------------------------------------------------- */

void test_producer_consumer_interleaved(void)
{
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    /* Simulate 4 rounds of: produce 8 samples, consume 8 samples */
    for (uint32_t round = 0u; round < 4u; round++) {
        uint32_t base = round * 8u;
        for (uint32_t i = 0u; i < 8u; i++) {
            dsp_sample_t s = make_sample(base + i);
            TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
        }

        dsp_sample_t buf[8];
        uint32_t got = 0u;
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, 8u, &got));
        TEST_ASSERT_EQUAL(8u, got);
        for (uint32_t i = 0u; i < 8u; i++) {
            TEST_ASSERT_EQUAL(base + i, buf[i].raw_value);
        }
    }

    TEST_ASSERT_EQUAL(0u, dsp_ring_buf_count(&sh));
    dsp_shared_deinit(&sh);
}

void test_producer_consumer_bursty_producer(void)
{
    /* Producer bursts to capacity, consumer drains in small batches */
    dsp_shared_t sh;
    dsp_shared_init(&sh);

    for (uint32_t i = 0u; i < DSP_RING_BUF_CAPACITY; i++) {
        dsp_sample_t s = make_sample(i);
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_push(&sh, &s));
    }

    uint32_t total_drained = 0u;
    uint32_t batch = 4u;
    dsp_sample_t buf[4];
    while (dsp_ring_buf_count(&sh) > 0u) {
        uint32_t got = 0u;
        TEST_ASSERT_EQUAL(ESP_OK, dsp_ring_buf_drain(&sh, buf, batch, &got));
        for (uint32_t i = 0u; i < got; i++) {
            TEST_ASSERT_EQUAL(total_drained + i, buf[i].raw_value);
        }
        total_drained += got;
    }
    TEST_ASSERT_EQUAL(DSP_RING_BUF_CAPACITY, total_drained);

    dsp_shared_deinit(&sh);
}
