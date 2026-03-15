/**
 * @file dsp_dht20.h
 * @brief DHT20 temperature and humidity sensor driver.
 *
 * Implements the DHT20 I2C protocol (address 0x38) using the ESP-IDF v5.x
 * i2c_master API on ESP_PLATFORM.  On host builds a stub is compiled instead
 * so that dsp_application.c compiles and tests pass without hardware.
 *
 * Hardware connection (3.3 V supply):
 *   DHT20 VDD  → 3.3 V
 *   DHT20 GND  → GND
 *   DHT20 SDA  → GPIO CONFIG_DSP_DHT20_SDA_PIN  (4.7 kΩ pull-up to 3.3 V)
 *   DHT20 SCL  → GPIO CONFIG_DSP_DHT20_SCL_PIN  (4.7 kΩ pull-up to 3.3 V)
 *
 * Ring-buffer sample encoding (dsp_sample_t):
 *   channel = DSP_DHT20_CH_TEMP (0):
 *       raw_value = (uint32_t)(int32_t)(temperature_c * 100)
 *       Decode:    temperature_c = (float)(int32_t)raw_value / 100.0f
 *       Range:     -5000 (−50 °C) … +15000 (+150 °C)
 *   channel = DSP_DHT20_CH_HUM (1):
 *       raw_value = (uint32_t)(humidity_pct * 100)
 *       Decode:    humidity_pct = (float)raw_value / 100.0f
 *       Range:     0 … 10000 (0–100 %)
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */

/** DHT20 fixed 7-bit I2C address. */
#define DSP_DHT20_I2C_ADDR   0x38

/** Channel index for temperature samples in dsp_sample_t. */
#define DSP_DHT20_CH_TEMP    0U

/** Channel index for humidity samples in dsp_sample_t. */
#define DSP_DHT20_CH_HUM     1U

/* -------------------------------------------------------------------------
 * Types
 * ------------------------------------------------------------------------- */

/** Opaque driver handle returned by dsp_dht20_init(). */
typedef struct dsp_dht20_s *dsp_dht20_handle_t;

/**
 * @brief Measurement result in physical units.
 */
typedef struct {
    float temperature_c;  /**< Temperature in °C. Range: -50 to +150. */
    float humidity_pct;   /**< Relative humidity in %. Range: 0 to 100. */
} dsp_dht20_reading_t;

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialise the DHT20 driver.
 *
 * Creates an I2C master bus on @p i2c_port, registers the DHT20 device at
 * address 0x38, waits 100 ms for the sensor to stabilise (datasheet §8.2),
 * reads the status byte, and resets calibration registers if the calibration
 * bits (bits 3 and 4) are not set.
 *
 * On host builds this is a lightweight stub — it allocates the handle struct
 * but performs no real I2C communication.
 *
 * @param sda_pin    GPIO number for I2C SDA. Ignored on host builds.
 * @param scl_pin    GPIO number for I2C SCL. Ignored on host builds.
 * @param i2c_port   I2C peripheral index (0 or 1). Ignored on host builds.
 * @param out_handle Set to the newly allocated handle on success.
 *
 * @return ESP_OK              on success.
 * @return ESP_ERR_INVALID_ARG if out_handle is NULL.
 * @return ESP_ERR_NO_MEM      if the handle struct cannot be allocated.
 * @return ESP_FAIL            if the I2C bus/device cannot be created or
 *                             calibration reset fails (device build only).
 */
esp_err_t dsp_dht20_init(int sda_pin, int scl_pin, int i2c_port,
                          dsp_dht20_handle_t *out_handle);

/**
 * @brief Read temperature and humidity from the DHT20.
 *
 * Sends the 0xAC 0x33 0x00 trigger command, blocks for ≥80 ms while the
 * sensor converts, polls the busy bit, reads 7 bytes, verifies the CRC-8
 * checksum (polynomial 0x31, initial value 0xFF), and converts the 20-bit
 * raw values to °C and %.
 *
 * Total blocking time: ~85–100 ms on ESP_PLATFORM.
 * On host builds returns fixed stub values immediately (no I2C).
 *
 * @param handle  Handle from dsp_dht20_init().
 * @param out     Populated with temperature_c and humidity_pct on success.
 *
 * @return ESP_OK               on success.
 * @return ESP_ERR_INVALID_ARG  if handle or out is NULL.
 * @return ESP_ERR_INVALID_CRC  if CRC-8 check fails.
 * @return ESP_ERR_TIMEOUT      if the busy bit does not clear within 1000 ms.
 * @return ESP_FAIL             on I2C bus error.
 */
esp_err_t dsp_dht20_read(dsp_dht20_handle_t handle, dsp_dht20_reading_t *out);

/**
 * @brief Free all resources allocated by dsp_dht20_init().
 *
 * Removes the I2C device and deletes the I2C master bus (ESP_PLATFORM only),
 * then frees the handle struct.  Passing NULL is a safe no-op.
 *
 * @param handle  Handle to destroy.
 */
void dsp_dht20_deinit(dsp_dht20_handle_t handle);

#ifdef __cplusplus
}
#endif
