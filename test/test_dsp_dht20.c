/**
 * @file test_dsp_dht20.c
 * @brief Unit tests for dsp_dht20 DHT20 sensor driver (host stub path).
 *
 * All tests run on the host build where dsp_dht20_read() returns fixed
 * stub values (T=21.5 °C, RH=55.0 %) without touching real I2C hardware.
 */

#include "unity.h"
#include "dsp_dht20.h"
#include "esp_err.h"

/* setUp / tearDown defined once in test_smoke.c. */

/* -------------------------------------------------------------------------
 * dsp_dht20_init – argument validation
 * ------------------------------------------------------------------------- */

void test_dht20_init_null_out_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_dht20_init(8, 9, 0, NULL));
}

/* -------------------------------------------------------------------------
 * dsp_dht20_init – success path (host stub)
 * ------------------------------------------------------------------------- */

void test_dht20_init_returns_ok(void)
{
    dsp_dht20_handle_t h = NULL;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_dht20_init(8, 9, 0, &h));
    TEST_ASSERT_NOT_NULL(h);
    dsp_dht20_deinit(h);
}

void test_dht20_init_produces_non_null_handle(void)
{
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);
    TEST_ASSERT_NOT_NULL(h);
    dsp_dht20_deinit(h);
}

/* -------------------------------------------------------------------------
 * dsp_dht20_read – argument validation
 * ------------------------------------------------------------------------- */

void test_dht20_read_null_handle_returns_invalid_arg(void)
{
    dsp_dht20_reading_t r;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_dht20_read(NULL, &r));
}

void test_dht20_read_null_out_returns_invalid_arg(void)
{
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, dsp_dht20_read(h, NULL));
    dsp_dht20_deinit(h);
}

/* -------------------------------------------------------------------------
 * dsp_dht20_read – stub values
 * ------------------------------------------------------------------------- */

void test_dht20_read_returns_ok(void)
{
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    TEST_ASSERT_EQUAL(ESP_OK, dsp_dht20_read(h, &r));

    dsp_dht20_deinit(h);
}

void test_dht20_read_temperature_in_range(void)
{
    /* Stub returns 21.5 °C; valid sensor range is −40 to +85 °C. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    TEST_ASSERT_GREATER_OR_EQUAL(-40.0f, r.temperature_c);
    TEST_ASSERT_LESS_OR_EQUAL(85.0f,     r.temperature_c);

    dsp_dht20_deinit(h);
}

void test_dht20_read_humidity_in_range(void)
{
    /* Stub returns 55.0 %; valid sensor range is 0–100 %. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0f,   r.humidity_pct);
    TEST_ASSERT_LESS_OR_EQUAL(100.0f,    r.humidity_pct);

    dsp_dht20_deinit(h);
}

void test_dht20_read_stub_temperature_value(void)
{
    /* Host stub must return exactly 21.5 °C so ring-buffer raw encoding
     * produces raw_value = (uint32_t)(int32_t)(21.5 * 100) = 2150 > 0. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 21.5f, r.temperature_c);

    dsp_dht20_deinit(h);
}

void test_dht20_read_stub_humidity_value(void)
{
    /* Host stub must return exactly 55.0 % so ring-buffer raw encoding
     * produces raw_value = (uint32_t)(55.0 * 100) = 5500 > 0. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, r.humidity_pct);

    dsp_dht20_deinit(h);
}

/* -------------------------------------------------------------------------
 * dsp_dht20_read – ring-buffer encoding compatibility
 * ------------------------------------------------------------------------- */

void test_dht20_temp_raw_encoding_nonzero(void)
{
    /* Verify that the encoding used in dsp_application.c produces a
     * non-zero raw_value for the stub temperature. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    uint32_t raw = (uint32_t)(int32_t)(r.temperature_c * 100.0f);
    TEST_ASSERT_GREATER_THAN(0u, raw);

    dsp_dht20_deinit(h);
}

void test_dht20_hum_raw_encoding_nonzero(void)
{
    /* Verify that the encoding used in dsp_application.c produces a
     * non-zero raw_value for the stub humidity. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    uint32_t raw = (uint32_t)(r.humidity_pct * 100.0f);
    TEST_ASSERT_GREATER_THAN(0u, raw);

    dsp_dht20_deinit(h);
}

void test_dht20_temp_raw_roundtrip(void)
{
    /* Encode then decode temperature; result must be within 0.01 °C. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    uint32_t raw      = (uint32_t)(int32_t)(r.temperature_c * 100.0f);
    float    decoded  = (float)(int32_t)raw / 100.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, r.temperature_c, decoded);

    dsp_dht20_deinit(h);
}

void test_dht20_hum_raw_roundtrip(void)
{
    /* Encode then decode humidity; result must be within 0.01 %. */
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);

    dsp_dht20_reading_t r;
    dsp_dht20_read(h, &r);
    uint32_t raw     = (uint32_t)(r.humidity_pct * 100.0f);
    float    decoded = (float)raw / 100.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, r.humidity_pct, decoded);

    dsp_dht20_deinit(h);
}

/* -------------------------------------------------------------------------
 * dsp_dht20_deinit – safe usage
 * ------------------------------------------------------------------------- */

void test_dht20_deinit_null_safe(void)
{
    dsp_dht20_deinit(NULL);
    TEST_PASS();
}

void test_dht20_deinit_valid_handle_safe(void)
{
    dsp_dht20_handle_t h = NULL;
    dsp_dht20_init(8, 9, 0, &h);
    dsp_dht20_deinit(h);
    TEST_PASS();
}

/* -------------------------------------------------------------------------
 * Channel constants
 * ------------------------------------------------------------------------- */

void test_dht20_ch_temp_is_zero(void)
{
    TEST_ASSERT_EQUAL(0U, DSP_DHT20_CH_TEMP);
}

void test_dht20_ch_hum_is_one(void)
{
    TEST_ASSERT_EQUAL(1U, DSP_DHT20_CH_HUM);
}

void test_dht20_i2c_addr_is_0x38(void)
{
    TEST_ASSERT_EQUAL(0x38U, DSP_DHT20_I2C_ADDR);
}
