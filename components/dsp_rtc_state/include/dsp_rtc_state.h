/**
 * @file dsp_rtc_state.h
 * @brief RTC memory persistence of negotiation and transfer state (DSP-505).
 *
 * Serializes the active dsp_neg and dsp_xfer slot tables into a fixed-size
 * struct held in RTC slow memory (preserved across deep sleep on ESP32-S3).
 * On host builds the backing store is a static variable so unit tests can
 * exercise the full save/restore round-trip without hardware.
 *
 * Layout (all fields fixed-width, pointer-free):
 *
 *   dsp_rtc_neg_slot_t  – one per DSP_NEG_MAX negotiation slot  (136 B each)
 *   dsp_rtc_xfer_slot_t – one per DSP_XFER_MAX transfer slot    ( 72 B each)
 *   dsp_rtc_state_t     – header (magic + CRC32) + both slot arrays
 *
 * Integrity is verified by a CRC32 of the data section before any restore.
 * If the magic or CRC is wrong, dsp_rtc_state_restore() returns
 * ESP_ERR_INVALID_STATE without touching the module tables.
 *
 * Typical usage before deep sleep:
 *   dsp_rtc_state_save();
 *   esp_deep_sleep_start();
 *
 * Typical usage after wake-up:
 *   dsp_neg_init();
 *   dsp_xfer_init();
 *   if (dsp_rtc_state_is_valid()) {
 *       dsp_rtc_state_restore();
 *   }
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "dsp_neg.h"
#include "dsp_xfer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Serialised slot types (pointer-free, fixed-width)
 * ========================================================================= */

/**
 * @brief Serialised negotiation slot as stored in RTC memory.
 *
 * Size: 1 + 3 (pad) + 4 (state) + 64 (cpid) + 64 (ppid) = 136 bytes.
 */
typedef struct {
    uint8_t  active;                       /**< 1 if slot is occupied.            */
    uint8_t  _pad[3];                      /**< Alignment padding — do not use.   */
    uint32_t state;                        /**< dsp_neg_state_t cast to uint32.   */
    char     consumer_pid[DSP_NEG_PID_LEN]; /**< Consumer process ID string.      */
    char     provider_pid[DSP_NEG_PID_LEN]; /**< Provider process ID string.      */
} dsp_rtc_neg_slot_t;

_Static_assert(sizeof(dsp_rtc_neg_slot_t) == 136u,
               "dsp_rtc_neg_slot_t must be 136 bytes");

/**
 * @brief Serialised transfer slot as stored in RTC memory.
 *
 * Size: 1 + 3 (pad) + 4 (state) + 64 (pid) = 72 bytes.
 */
typedef struct {
    uint8_t  active;                        /**< 1 if slot is occupied.           */
    uint8_t  _pad[3];                       /**< Alignment padding — do not use.  */
    uint32_t state;                         /**< dsp_xfer_state_t cast to uint32. */
    char     process_id[DSP_XFER_PID_LEN];  /**< Transfer process ID string.      */
} dsp_rtc_xfer_slot_t;

_Static_assert(sizeof(dsp_rtc_xfer_slot_t) == 72u,
               "dsp_rtc_xfer_slot_t must be 72 bytes");

/**
 * @brief Complete RTC state image.
 *
 * Size: 4 (magic) + 4 (crc32) + DSP_NEG_MAX*136 + DSP_XFER_MAX*72
 *     = 8 + 544 + 144 = 696 bytes (with defaults DSP_NEG_MAX=4, DSP_XFER_MAX=2).
 *
 * On ESP32-S3 this struct is placed in RTC slow memory via RTC_DATA_ATTR
 * and survives deep sleep.  On host it is a plain static variable.
 */
typedef struct {
    uint32_t magic;                       /**< DSP_RTC_MAGIC when valid.          */
    uint32_t crc32;                       /**< CRC32 of the neg[] + xfer[] data.  */
    dsp_rtc_neg_slot_t  neg[DSP_NEG_MAX];   /**< Serialised negotiation slots.   */
    dsp_rtc_xfer_slot_t xfer[DSP_XFER_MAX]; /**< Serialised transfer slots.      */
} dsp_rtc_state_t;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * @brief Snapshot the current dsp_neg and dsp_xfer slot tables to RTC memory.
 *
 * Copies all active slots into the RTC backing store, computes CRC32 over the
 * data, and writes the magic sentinel.  Safe to call from any context.
 *
 * @return ESP_OK always.
 */
esp_err_t dsp_rtc_state_save(void);

/**
 * @brief Restore dsp_neg and dsp_xfer slot tables from RTC memory.
 *
 * Verifies magic and CRC32, then calls dsp_neg_load_slot() and
 * dsp_xfer_load_slot() for each active serialised slot.
 *
 * dsp_neg_init() and dsp_xfer_init() must be called before this function.
 *
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_STATE if magic is wrong (no saved state).
 *         ESP_ERR_INVALID_CRC   if CRC32 check fails (data corrupted).
 */
esp_err_t dsp_rtc_state_restore(void);

/**
 * @brief Clear the RTC backing store (invalidates magic).
 *
 * Safe to call at any time.  After this call dsp_rtc_state_is_valid() returns
 * false and dsp_rtc_state_restore() returns ESP_ERR_INVALID_STATE.
 */
void dsp_rtc_state_clear(void);

/**
 * @brief Return true if the RTC backing store contains valid saved state.
 *
 * Checks magic sentinel and CRC32.
 */
bool dsp_rtc_state_is_valid(void);

#ifdef __cplusplus
}
#endif
