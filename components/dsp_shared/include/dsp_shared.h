/**
 * @file dsp_shared.h
 * @brief Shared memory layout for dual-core DSP Embedded (DSP-501).
 *
 * Defines the data structures exchanged between Core 0 (protocol task) and
 * Core 1 (application task):
 *
 *   dsp_sample_t     – fixed-width sensor sample (16 B, pointer-free)
 *   dsp_ring_buf_t   – ring buffer of samples (pointer-free, verified size)
 *   dsp_shared_t     – full shared state including FreeRTOS handles
 *
 * Layout constraints:
 *   - dsp_sample_t and dsp_ring_buf_t are pointer-free; their sizes are
 *     identical on 32-bit and 64-bit targets and verified by _Static_assert.
 *   - dsp_shared_t contains FreeRTOS handles (pointer-sized):
 *       ESP32-S3 (32-bit): sizeof(dsp_shared_t) == 544 bytes
 *       x86-64   (64-bit): sizeof(dsp_shared_t) == 552 bytes
 *     The ring_buf field is always at offset 0 (verified by _Static_assert).
 *
 * Overflow policy (ring buffer):
 *   DROP NEWEST – dsp_ring_buf_push() discards incoming samples when the
 *   buffer is full and returns ESP_ERR_NO_MEM.  Core 1 is always
 *   non-blocking; Core 0 is responsible for draining the buffer during
 *   active transfers.
 *
 * Used by: dsp_protocol_task (Core 0, DSP-502), dsp_application_task (Core 1, DSP-503)
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Sensor sample
 * =========================================================================
 *
 * Fixed-width, pointer-free: identical layout on 32-bit and 64-bit targets.
 * Total size: 16 bytes.
 */

/** Maximum number of distinct ADC channels. */
#define DSP_SAMPLE_CHANNEL_MAX  4U

/**
 * @brief Single sensor sample produced by Core 1 and consumed by Core 0.
 *
 * All fields use fixed-width types; the 3-byte explicit pad ensures the
 * struct is exactly 16 bytes on every compiler and target.
 */
typedef struct {
    uint64_t timestamp_us; /**< Acquisition time in µs (esp_timer_get_time()). */
    uint32_t raw_value;    /**< Raw ADC reading (12-bit: 0–4095).              */
    uint8_t  channel;      /**< ADC channel index (0 to DSP_SAMPLE_CHANNEL_MAX-1). */
    uint8_t  _pad[3];      /**< Explicit padding — do not use.                 */
} dsp_sample_t;

_Static_assert(sizeof(dsp_sample_t)                    == 16u, "dsp_sample_t must be 16 bytes");
_Static_assert(offsetof(dsp_sample_t, timestamp_us)   ==  0u, "timestamp_us at offset 0");
_Static_assert(offsetof(dsp_sample_t, raw_value)       ==  8u, "raw_value at offset 8");
_Static_assert(offsetof(dsp_sample_t, channel)         == 12u, "channel at offset 12");

/* =========================================================================
 * Ring buffer (data section — no handles, fully pointer-free)
 * =========================================================================
 *
 * Total size: DSP_RING_BUF_CAPACITY * sizeof(dsp_sample_t) + 4 * 4
 *           = 32 * 16 + 16 = 528 bytes.
 *
 * Access must be serialised by the ring_buf_mutex in dsp_shared_t.
 */

/**
 * Ring buffer capacity in samples.  Must be a power of 2.
 *
 * Overflow policy: DROP NEWEST.
 * When full, dsp_ring_buf_push() discards the incoming sample and returns
 * ESP_ERR_NO_MEM.  This keeps Core 1 strictly non-blocking.
 */
#define DSP_RING_BUF_CAPACITY  32U

_Static_assert((DSP_RING_BUF_CAPACITY & (DSP_RING_BUF_CAPACITY - 1u)) == 0u,
               "DSP_RING_BUF_CAPACITY must be a power of 2");

/**
 * @brief Ring buffer of dsp_sample_t values.
 *
 * Pointer-free: identical layout on 32-bit and 64-bit targets.
 * Size: 528 bytes.
 */
typedef struct {
    dsp_sample_t data[DSP_RING_BUF_CAPACITY]; /**< Sample storage (512 B).        */
    uint32_t     head;  /**< Next write slot index; advanced by Core 1 on push.   */
    uint32_t     tail;  /**< Next read  slot index; advanced by Core 0 on pop.    */
    uint32_t     count; /**< Number of valid samples currently in the buffer.     */
    uint32_t     _pad;  /**< Explicit padding to maintain 8-byte alignment.       */
} dsp_ring_buf_t;

_Static_assert(sizeof(dsp_ring_buf_t)               == 528u, "dsp_ring_buf_t must be 528 bytes");
_Static_assert(offsetof(dsp_ring_buf_t, data)        ==   0u, "data at offset 0");
_Static_assert(offsetof(dsp_ring_buf_t, head)        == 512u, "head at offset 512");
_Static_assert(offsetof(dsp_ring_buf_t, tail)        == 516u, "tail at offset 516");
_Static_assert(offsetof(dsp_ring_buf_t, count)       == 520u, "count at offset 520");

/* =========================================================================
 * Full shared state
 * =========================================================================
 *
 * Instantiate exactly once (e.g. as a file-scope static in main.c) and
 * pass a pointer to both tasks via their pvParameters argument.
 *
 * Sizes of dsp_shared_t:
 *   ESP32-S3 (32-bit pointers): 528 + 4 + 4 + 4 + 4 = 544 bytes
 *   x86-64   (64-bit pointers): 528 + 8 + 8 + 4 + 4 = 552 bytes
 *
 * The ring_buf field is always at offset 0 (verified below).
 */

/**
 * @brief Full shared state between Core 0 (protocol) and Core 1 (application).
 */
typedef struct {
    dsp_ring_buf_t    ring_buf;       /**< Sensor data ring buffer (528 B).         */
    SemaphoreHandle_t ring_buf_mutex; /**< Mutex; guards all ring_buf R/W access.   */
    QueueHandle_t     xfer_notify_q;  /**< Core 0 enqueues transfer-slot index here
                                           to notify Core 1 that a transfer started. */
    volatile uint32_t core0_ready;   /**< Set to 1 when protocol stack is up.       */
    volatile uint32_t core1_ready;   /**< Set to 1 when application task is running.*/
} dsp_shared_t;

_Static_assert(offsetof(dsp_shared_t, ring_buf) == 0u,
               "ring_buf must be at offset 0 in dsp_shared_t");

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * @brief Initialise shared state.
 *
 * Zeroes the ring buffer and flags.  On ESP_PLATFORM, creates the ring buffer
 * mutex (binary semaphore) and the transfer notification queue.
 *
 * @param sh  Caller-allocated shared state struct.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if sh is NULL,
 *         ESP_ERR_NO_MEM if FreeRTOS object creation fails.
 */
esp_err_t dsp_shared_init(dsp_shared_t *sh);

/**
 * @brief Free shared state resources.
 *
 * On ESP_PLATFORM, deletes the mutex and queue.  Safe to call on a partially
 * initialised struct (NULL handles are skipped).
 *
 * @param sh  Shared state previously passed to dsp_shared_init().
 */
void dsp_shared_deinit(dsp_shared_t *sh);

/**
 * @brief Push a sensor sample into the ring buffer (Core 1 side).
 *
 * Thread-safe via ring_buf_mutex.
 * Overflow policy: DROP NEWEST — if full, the sample is discarded and
 * ESP_ERR_NO_MEM is returned.  This function never blocks.
 *
 * @param sh      Initialised shared state.
 * @param sample  Sample to enqueue (copied by value).
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if sh or sample is NULL.
 *         ESP_ERR_NO_MEM if the buffer is full (sample dropped).
 */
esp_err_t dsp_ring_buf_push(dsp_shared_t *sh, const dsp_sample_t *sample);

/**
 * @brief Pop the oldest sensor sample from the ring buffer (Core 0 side).
 *
 * Thread-safe via ring_buf_mutex.
 *
 * @param sh         Initialised shared state.
 * @param[out] out   Populated with the oldest sample on success.
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if sh or out is NULL.
 *         ESP_ERR_NOT_FOUND if the buffer is empty.
 */
esp_err_t dsp_ring_buf_pop(dsp_shared_t *sh, dsp_sample_t *out);

/**
 * @brief Return the current number of samples in the ring buffer.
 *
 * Thread-safe via ring_buf_mutex.
 *
 * @param sh  Initialised shared state.
 * @return Sample count, or 0 if sh is NULL.
 */
uint32_t dsp_ring_buf_count(const dsp_shared_t *sh);

#ifdef __cplusplus
}
#endif
