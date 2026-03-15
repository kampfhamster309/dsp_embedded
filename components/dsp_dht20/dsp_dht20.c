/**
 * @file dsp_dht20.c
 * @brief DHT20 I2C temperature and humidity sensor driver.
 *
 * Protocol reference: DHT20 datasheet rev. April 2022.
 *
 * Measurement flow:
 *   1. Power-on: wait 100 ms, check calibration bits (status & 0x18).
 *      If not calibrated, reset registers 0x1B, 0x1C, 0x1E per §8.2.
 *   2. Trigger: I2C write [0xAC, 0x33, 0x00].
 *   3. Wait ≥ 80 ms, then poll status byte until busy bit (bit 7) clears.
 *   4. Read 7 bytes: [status, Hh, Hm, Hl|Th, Tm, Tl, CRC].
 *   5. Verify CRC-8 (poly 0x31, init 0xFF) over bytes 0–5.
 *   6. Extract 20-bit raw values and apply conversion formulas.
 *
 * On host builds (no ESP_PLATFORM) the driver compiles to a lightweight stub:
 *   - dsp_dht20_init()  allocates the handle struct and returns ESP_OK.
 *   - dsp_dht20_read()  returns fixed values: T = 21.5 °C, RH = 55.0 %.
 *   - dsp_dht20_deinit() frees the handle struct.
 */

#include "dsp_dht20.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static const char *TAG = "dsp_dht20";

/* -------------------------------------------------------------------------
 * Protocol constants
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM

/** Trigger measurement command + fixed parameters (datasheet §8.3). */
static const uint8_t DHT20_TRIGGER[3] = {0xAC, 0x33, 0x00};

#define DHT20_STATUS_BUSY       0x80U  /**< Bit 7: measurement in progress. */
#define DHT20_STATUS_CAL_MASK   0x18U  /**< Bits 3+4: must both be 1. */
#define DHT20_STATUS_CAL_OK     0x18U

/** Calibration registers that must be reset when calibration bits are off. */
static const uint8_t DHT20_CAL_REGS[3] = {0x1B, 0x1C, 0x1E};

#define DHT20_I2C_TIMEOUT_MS     100   /**< Per-transaction I2C timeout. */
#define DHT20_MEAS_WAIT_MS        85   /**< Initial wait after trigger (≥80 ms). */
#define DHT20_BUSY_POLL_STEP_MS    5   /**< Polling interval while busy. */
#define DHT20_BUSY_TIMEOUT_MS   1000   /**< Abort if busy bit persists this long. */
#define DHT20_POWERON_WAIT_MS    100   /**< Power-on stabilisation (§8.2). */
#define DHT20_I2C_SCL_HZ      100000  /**< Standard-mode I2C clock. */

/* Datasheet conversion formulas (20-bit raw, 2^20 = 1048576):
 *   RH (%)  = raw_H * 100 / 1048576
 *   T  (°C) = raw_T * 200 / 1048576 − 50   */
#define DHT20_HUM_SCALE   (100.0f  / 1048576.0f)
#define DHT20_TEMP_SCALE  (200.0f  / 1048576.0f)
#define DHT20_TEMP_OFFSET (-50.0f)

/* -------------------------------------------------------------------------
 * CRC-8 (polynomial 0x31, initial value 0xFF, no reflection)
 * Covers bytes 0–5 of the 7-byte response; result compared against byte 6.
 * ------------------------------------------------------------------------- */

static uint8_t dht20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8u; i++) {
            crc = (crc & 0x80u) ? ((uint8_t)((crc << 1u) ^ 0x31u))
                                : (uint8_t)(crc << 1u);
        }
    }
    return crc;
}

#endif /* ESP_PLATFORM – protocol constants and CRC */

/* -------------------------------------------------------------------------
 * Driver context
 * ------------------------------------------------------------------------- */

struct dsp_dht20_s {
#ifdef ESP_PLATFORM
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
#else
    int _placeholder;  /* keep struct non-empty on host */
#endif
};

/* -------------------------------------------------------------------------
 * Calibration register reset (datasheet §8.2)
 * Reset one internal calibration register:
 *   A. Write [reg, 0x00, 0x00] → wait 5 ms
 *   B. Read 3 bytes             → wait 10 ms
 *   C. Write [0xB0 | reg, b[1], b[2]] → wait 5 ms
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM
static esp_err_t dht20_reset_register(dsp_dht20_handle_t h, uint8_t reg)
{
    uint8_t cmd[3] = {reg, 0x00, 0x00};
    esp_err_t err = i2c_master_transmit(h->dev, cmd, 3, DHT20_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t rx[3] = {0};
    err = i2c_master_receive(h->dev, rx, 3, DHT20_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t fixup[3] = {(uint8_t)(0xB0u | reg), rx[1], rx[2]};
    err = i2c_master_transmit(h->dev, fixup, 3, DHT20_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    return ESP_OK;
}
#endif /* ESP_PLATFORM */

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

esp_err_t dsp_dht20_init(int sda_pin, int scl_pin, int i2c_port,
                          dsp_dht20_handle_t *out_handle)
{
    if (!out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    struct dsp_dht20_s *h = calloc(1, sizeof(*h));
    if (!h) {
        return ESP_ERR_NO_MEM;
    }

#ifdef ESP_PLATFORM
    /* Create I2C master bus */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = i2c_port,
        .sda_io_num        = sda_pin,
        .scl_io_num        = scl_pin,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &h->bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: 0x%x", err);
        free(h);
        return ESP_FAIL;
    }

    /* Register the DHT20 device at address 0x38 */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = DSP_DHT20_I2C_ADDR,
        .scl_speed_hz    = DHT20_I2C_SCL_HZ,
    };
    err = i2c_master_bus_add_device(h->bus, &dev_cfg, &h->dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: 0x%x", err);
        i2c_del_master_bus(h->bus);
        free(h);
        return ESP_FAIL;
    }

    /* Power-on stabilisation: wait 100 ms before first communication */
    vTaskDelay(pdMS_TO_TICKS(DHT20_POWERON_WAIT_MS));

    /* Read status byte (single-byte read, no command prefix) */
    uint8_t status = 0;
    err = i2c_master_receive(h->dev, &status, 1, DHT20_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "initial status read failed: 0x%x", err);
        i2c_master_bus_rm_device(h->dev);
        i2c_del_master_bus(h->bus);
        free(h);
        return ESP_FAIL;
    }

    /* If calibration bits (3 and 4) are not both set, reset the three
     * internal calibration registers per the datasheet §8.2 procedure. */
    if ((status & DHT20_STATUS_CAL_MASK) != DHT20_STATUS_CAL_OK) {
        ESP_LOGW(TAG, "sensor uncalibrated (status=0x%02x), resetting regs",
                 status);
        for (int i = 0; i < 3; i++) {
            err = dht20_reset_register(h, DHT20_CAL_REGS[i]);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "cal reg 0x%02x reset failed: 0x%x",
                         DHT20_CAL_REGS[i], err);
                i2c_master_bus_rm_device(h->dev);
                i2c_del_master_bus(h->bus);
                free(h);
                return ESP_FAIL;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_LOGI(TAG, "calibration registers restored");
    } else {
        ESP_LOGI(TAG, "sensor calibrated (status=0x%02x)", status);
    }

    ESP_LOGI(TAG, "init ok — SDA=%d SCL=%d I2C%d addr=0x38",
             sda_pin, scl_pin, i2c_port);

#else /* host stub */
    (void)sda_pin;
    (void)scl_pin;
    (void)i2c_port;
    ESP_LOGI(TAG, "init ok (host stub — no real I2C)");
#endif /* ESP_PLATFORM */

    *out_handle = h;
    return ESP_OK;
}

esp_err_t dsp_dht20_read(dsp_dht20_handle_t handle, dsp_dht20_reading_t *out)
{
    if (!handle || !out) {
        return ESP_ERR_INVALID_ARG;
    }

#ifdef ESP_PLATFORM
    /* 1. Send trigger command: 0xAC 0x33 0x00 */
    esp_err_t err = i2c_master_transmit(handle->dev, DHT20_TRIGGER,
                                         sizeof(DHT20_TRIGGER),
                                         DHT20_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "trigger write failed: 0x%x", err);
        return ESP_FAIL;
    }

    /* 2. Wait ≥ 80 ms for the sensor to complete the conversion */
    vTaskDelay(pdMS_TO_TICKS(DHT20_MEAS_WAIT_MS));

    /* 3. Poll busy bit; most reads succeed on the first attempt */
    int waited_ms = DHT20_MEAS_WAIT_MS;
    for (;;) {
        uint8_t status = 0;
        err = i2c_master_receive(handle->dev, &status, 1, DHT20_I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "busy-poll read failed: 0x%x", err);
            return ESP_FAIL;
        }
        if ((status & DHT20_STATUS_BUSY) == 0u) {
            break;
        }
        if (waited_ms >= DHT20_BUSY_TIMEOUT_MS) {
            ESP_LOGE(TAG, "sensor busy timeout after %d ms", waited_ms);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(DHT20_BUSY_POLL_STEP_MS));
        waited_ms += DHT20_BUSY_POLL_STEP_MS;
    }

    /* 4. Read 7 bytes: status | Hh | Hm | Hl/Th | Tm | Tl | CRC */
    uint8_t buf[7] = {0};
    err = i2c_master_receive(handle->dev, buf, sizeof(buf), DHT20_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "data read failed: 0x%x", err);
        return ESP_FAIL;
    }

    /* 5. Verify CRC-8 over bytes 0–5; expected result in byte 6 */
    uint8_t computed_crc = dht20_crc8(buf, 6);
    if (computed_crc != buf[6]) {
        ESP_LOGE(TAG, "CRC mismatch: computed=0x%02x received=0x%02x",
                 computed_crc, buf[6]);
        return ESP_ERR_INVALID_CRC;
    }

    /* 6. Extract 20-bit humidity and temperature raw values
     *
     *   buf[1][7:0]  = raw_H[19:12]
     *   buf[2][7:0]  = raw_H[11:4]
     *   buf[3][7:4]  = raw_H[3:0]
     *   buf[3][3:0]  = raw_T[19:16]
     *   buf[4][7:0]  = raw_T[15:8]
     *   buf[5][7:0]  = raw_T[7:0]
     */
    uint32_t raw_H = ((uint32_t)buf[1] << 12u)
                   | ((uint32_t)buf[2] <<  4u)
                   | ((uint32_t)buf[3] >>  4u);

    uint32_t raw_T = ((uint32_t)(buf[3] & 0x0Fu) << 16u)
                   | ((uint32_t)buf[4]            <<  8u)
                   |  (uint32_t)buf[5];

    /* 7. Apply datasheet conversion formulas */
    out->humidity_pct  = (float)raw_H * DHT20_HUM_SCALE;
    out->temperature_c = (float)raw_T * DHT20_TEMP_SCALE + DHT20_TEMP_OFFSET;

    ESP_LOGD(TAG, "T=%.2f°C  RH=%.2f%%  (raw_T=%"PRIu32" raw_H=%"PRIu32")",
             out->temperature_c, out->humidity_pct, raw_T, raw_H);

#else /* host stub */
    /* Return representative fixed values so host tests can verify the
     * acquisition path without hardware. */
    out->temperature_c = 21.5f;
    out->humidity_pct  = 55.0f;
#endif /* ESP_PLATFORM */

    return ESP_OK;
}

void dsp_dht20_deinit(dsp_dht20_handle_t handle)
{
    if (!handle) {
        return;
    }

#ifdef ESP_PLATFORM
    i2c_master_bus_rm_device(handle->dev);
    i2c_del_master_bus(handle->bus);
#endif

    free(handle);
}
