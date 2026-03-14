/**
 * @file dsp_rtc_state.c
 * @brief RTC memory persistence of negotiation and transfer state (DSP-505).
 */

#include "dsp_rtc_state.h"
#include "dsp_neg.h"
#include "dsp_xfer.h"
#include "esp_err.h"

#include <string.h>
#include <stdint.h>

/* Magic sentinel: 'D','5','P','5' → 0x44355035 */
#define DSP_RTC_MAGIC  0x44355035U

/* =========================================================================
 * RTC backing store
 *
 * On ESP32-S3: RTC_DATA_ATTR places the variable in RTC slow memory which
 * is retained through deep sleep.
 * On host: plain static variable simulates persistence across save/restore
 * within the same process (sufficient for unit tests).
 * ========================================================================= */

#ifdef ESP_PLATFORM
#include "esp_attr.h"
RTC_DATA_ATTR static dsp_rtc_state_t s_store;
#else
static dsp_rtc_state_t s_store;
#endif

/* =========================================================================
 * CRC32 (IEEE 802.3 polynomial, same as zlib/Ethernet)
 *
 * Computed over the data section (neg[] + xfer[] arrays) only — the magic
 * and crc32 header fields are excluded so the CRC can be verified without
 * a circular dependency.
 * ========================================================================= */

static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, size_t len)
{
    /* Standard reflected CRC32 with polynomial 0xEDB88320 */
    for (size_t i = 0u; i < len; i++) {
        crc ^= (uint32_t)buf[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1u) {
                crc = (crc >> 1u) ^ 0xEDB88320U;
            } else {
                crc >>= 1u;
            }
        }
    }
    return crc;
}

static uint32_t compute_crc32(const dsp_rtc_state_t *s)
{
    uint32_t crc = 0xFFFFFFFFU;
    crc = crc32_update(crc, (const uint8_t *)s->neg,  sizeof(s->neg));
    crc = crc32_update(crc, (const uint8_t *)s->xfer, sizeof(s->xfer));
    return crc ^ 0xFFFFFFFFU;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t dsp_rtc_state_save(void)
{
    /* Clear the data section before writing so stale slots are wiped. */
    memset(s_store.neg,  0, sizeof(s_store.neg));
    memset(s_store.xfer, 0, sizeof(s_store.xfer));

    /* Snapshot negotiation slots. */
    for (int i = 0; i < DSP_NEG_MAX; i++) {
        if (!dsp_neg_is_active(i)) {
            continue;
        }
        s_store.neg[i].active = 1u;
        s_store.neg[i].state  = (uint32_t)dsp_neg_get_state(i);

        const char *cpid = dsp_neg_get_consumer_pid(i);
        const char *ppid = dsp_neg_get_provider_pid(i);
        if (cpid) {
            strncpy(s_store.neg[i].consumer_pid, cpid, DSP_NEG_PID_LEN - 1u);
            s_store.neg[i].consumer_pid[DSP_NEG_PID_LEN - 1u] = '\0';
        }
        if (ppid) {
            strncpy(s_store.neg[i].provider_pid, ppid, DSP_NEG_PID_LEN - 1u);
            s_store.neg[i].provider_pid[DSP_NEG_PID_LEN - 1u] = '\0';
        }
    }

    /* Snapshot transfer slots. */
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        if (!dsp_xfer_is_active(i)) {
            continue;
        }
        s_store.xfer[i].active = 1u;
        s_store.xfer[i].state  = (uint32_t)dsp_xfer_get_state(i);

        const char *pid = dsp_xfer_get_process_id(i);
        if (pid) {
            strncpy(s_store.xfer[i].process_id, pid, DSP_XFER_PID_LEN - 1u);
            s_store.xfer[i].process_id[DSP_XFER_PID_LEN - 1u] = '\0';
        }
    }

    /* Compute CRC32 over the data fields, then seal with magic. */
    s_store.crc32 = compute_crc32(&s_store);
    s_store.magic = DSP_RTC_MAGIC;

    return ESP_OK;
}

esp_err_t dsp_rtc_state_restore(void)
{
    if (s_store.magic != DSP_RTC_MAGIC) {
        return ESP_ERR_INVALID_STATE;
    }

    if (compute_crc32(&s_store) != s_store.crc32) {
        return ESP_ERR_INVALID_CRC;
    }

    /* Restore negotiation slots directly (bypasses state machine). */
    for (int i = 0; i < DSP_NEG_MAX; i++) {
        if (!s_store.neg[i].active) {
            continue;
        }
        dsp_neg_load_slot(i,
                          (dsp_neg_state_t)s_store.neg[i].state,
                          s_store.neg[i].consumer_pid,
                          s_store.neg[i].provider_pid);
    }

    /* Restore transfer slots directly (no notify callback fired). */
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        if (!s_store.xfer[i].active) {
            continue;
        }
        dsp_xfer_load_slot(i,
                           (dsp_xfer_state_t)s_store.xfer[i].state,
                           s_store.xfer[i].process_id);
    }

    return ESP_OK;
}

void dsp_rtc_state_clear(void)
{
    memset(&s_store, 0, sizeof(s_store));
    /* magic == 0 → dsp_rtc_state_is_valid() returns false */
}

bool dsp_rtc_state_is_valid(void)
{
    if (s_store.magic != DSP_RTC_MAGIC) {
        return false;
    }
    return compute_crc32(&s_store) == s_store.crc32;
}

#ifndef ESP_PLATFORM
void dsp_rtc_state_corrupt_crc_for_testing(void)
{
    s_store.crc32 ^= 0xFFFFFFFFU; /* flip all bits → guaranteed mismatch */
}
#endif
