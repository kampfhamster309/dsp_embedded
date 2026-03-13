/**
 * @file dsp_shared.c
 * @brief Shared memory implementation for dual-core DSP Embedded (DSP-501).
 */

#include "dsp_shared.h"
#include <string.h>

/* =========================================================================
 * Platform-specific locking helpers
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

/** Depth of the transfer notification queue (one slot per pending transfer). */
#define XFER_NOTIFY_Q_DEPTH  4U

/** Depth of the data-ready notification queue.
 *  Depth=1: signals coalesce — multiple back-to-back pushes produce at most
 *  one pending notification.  Core 0 drains all available samples on wake. */
#define DATA_NOTIFY_Q_DEPTH  1U

static inline void ring_lock(dsp_shared_t *sh)
{
    xSemaphoreTake(sh->ring_buf_mutex, portMAX_DELAY);
}

static inline void ring_unlock(dsp_shared_t *sh)
{
    xSemaphoreGive(sh->ring_buf_mutex);
}

static inline void ring_lock_const(const dsp_shared_t *sh)
{
    xSemaphoreTake(((dsp_shared_t *)sh)->ring_buf_mutex, portMAX_DELAY);
}

static inline void ring_unlock_const(const dsp_shared_t *sh)
{
    xSemaphoreGive(((dsp_shared_t *)sh)->ring_buf_mutex);
}

esp_err_t dsp_shared_init(dsp_shared_t *sh)
{
    if (!sh) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(sh, 0, sizeof(*sh));

    sh->ring_buf_mutex = xSemaphoreCreateMutex();
    if (!sh->ring_buf_mutex) {
        return ESP_ERR_NO_MEM;
    }

    sh->xfer_notify_q = xQueueCreate(XFER_NOTIFY_Q_DEPTH, sizeof(uint32_t));
    if (!sh->xfer_notify_q) {
        vSemaphoreDelete(sh->ring_buf_mutex);
        sh->ring_buf_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    sh->data_notify_q = xQueueCreate(DATA_NOTIFY_Q_DEPTH, sizeof(uint32_t));
    if (!sh->data_notify_q) {
        vQueueDelete(sh->xfer_notify_q);
        sh->xfer_notify_q = NULL;
        vSemaphoreDelete(sh->ring_buf_mutex);
        sh->ring_buf_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void dsp_shared_deinit(dsp_shared_t *sh)
{
    if (!sh) {
        return;
    }
    if (sh->data_notify_q) {
        vQueueDelete(sh->data_notify_q);
        sh->data_notify_q = NULL;
    }
    if (sh->xfer_notify_q) {
        vQueueDelete(sh->xfer_notify_q);
        sh->xfer_notify_q = NULL;
    }
    if (sh->ring_buf_mutex) {
        vSemaphoreDelete(sh->ring_buf_mutex);
        sh->ring_buf_mutex = NULL;
    }
    memset(&sh->ring_buf, 0, sizeof(sh->ring_buf));
    sh->core0_ready = 0;
    sh->core1_ready = 0;
}

#else /* Host-native stub */

static inline void ring_lock(dsp_shared_t *sh)       { (void)sh; }
static inline void ring_unlock(dsp_shared_t *sh)     { (void)sh; }
static inline void ring_lock_const(const dsp_shared_t *sh)   { (void)sh; }
static inline void ring_unlock_const(const dsp_shared_t *sh) { (void)sh; }

esp_err_t dsp_shared_init(dsp_shared_t *sh)
{
    if (!sh) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(sh, 0, sizeof(*sh));
    return ESP_OK;
}

void dsp_shared_deinit(dsp_shared_t *sh)
{
    if (!sh) {
        return;
    }
    memset(sh, 0, sizeof(*sh));
}

#endif /* ESP_PLATFORM */

/* =========================================================================
 * Ring buffer operations (platform-agnostic, locking via helpers above)
 * ========================================================================= */

esp_err_t dsp_ring_buf_push(dsp_shared_t *sh, const dsp_sample_t *sample)
{
    if (!sh || !sample) {
        return ESP_ERR_INVALID_ARG;
    }

    ring_lock(sh);

    if (sh->ring_buf.count >= DSP_RING_BUF_CAPACITY) {
        ring_unlock(sh);
        return ESP_ERR_NO_MEM; /* DROP NEWEST: caller logs and continues */
    }

    sh->ring_buf.data[sh->ring_buf.head] = *sample;
    sh->ring_buf.head = (sh->ring_buf.head + 1u) % DSP_RING_BUF_CAPACITY;
    sh->ring_buf.count++;

    ring_unlock(sh);

#ifdef ESP_PLATFORM
    /* Notify Core 0 that data is available.  Non-blocking: if the queue is
     * already full a pending notification exists, so nothing is lost. */
    if (sh->data_notify_q) {
        uint32_t dummy = 1u;
        xQueueSend(sh->data_notify_q, &dummy, 0);
    }
#endif

    return ESP_OK;
}

esp_err_t dsp_ring_buf_pop(dsp_shared_t *sh, dsp_sample_t *out)
{
    if (!sh || !out) {
        return ESP_ERR_INVALID_ARG;
    }

    ring_lock(sh);

    if (sh->ring_buf.count == 0u) {
        ring_unlock(sh);
        return ESP_ERR_NOT_FOUND;
    }

    *out = sh->ring_buf.data[sh->ring_buf.tail];
    sh->ring_buf.tail = (sh->ring_buf.tail + 1u) % DSP_RING_BUF_CAPACITY;
    sh->ring_buf.count--;

    ring_unlock(sh);
    return ESP_OK;
}

uint32_t dsp_ring_buf_count(const dsp_shared_t *sh)
{
    if (!sh) {
        return 0u;
    }
    ring_lock_const(sh);
    uint32_t n = sh->ring_buf.count;
    ring_unlock_const(sh);
    return n;
}

esp_err_t dsp_ring_buf_drain(dsp_shared_t *sh, dsp_sample_t *buf,
                              uint32_t capacity, uint32_t *out_count)
{
    /* No-op path: capacity==0 needs no buffer and is always valid. */
    if (capacity == 0u) {
        if (out_count) {
            *out_count = 0u;
        }
        return ESP_OK;
    }

    if (!sh || !buf) {
        return ESP_ERR_INVALID_ARG;
    }

    ring_lock(sh);

    uint32_t n = sh->ring_buf.count < capacity ? sh->ring_buf.count : capacity;
    for (uint32_t i = 0u; i < n; i++) {
        buf[i] = sh->ring_buf.data[sh->ring_buf.tail];
        sh->ring_buf.tail = (sh->ring_buf.tail + 1u) % DSP_RING_BUF_CAPACITY;
        sh->ring_buf.count--;
    }

    ring_unlock(sh);

    if (out_count) {
        *out_count = n;
    }
    return ESP_OK;
}
