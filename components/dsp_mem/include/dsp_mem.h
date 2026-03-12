/**
 * @file dsp_mem.h
 * @brief DSP Embedded RAM usage instrumentation.
 *
 * Provides a snapshot logger and query helpers so budget compliance can be
 * verified at any point during development without a separate profiler.
 *
 * Memory budget (from target.md):
 *   Total internal RAM       ≤ 130 KB
 *   TLS (dsp_mbedtls)        ≤  50 KB
 *   HTTP (dsp_http)          ≤  20 KB
 *   JSON parser              ≤  15 KB
 *   JWT validation           ≤  25 KB
 *   DSP state machines       ≤  20 KB
 *
 * Usage pattern:
 *   dsp_mem_report("boot");          // before any component init
 *   dsp_tls_server_init(...);
 *   dsp_mem_report("after-tls");     // measure TLS cost
 *
 * Used by: main (DSP-104), integration tests (M7)
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Budget constants (bytes) – mirrors target.md
 * ------------------------------------------------------------------------- */

#define DSP_MEM_BUDGET_TOTAL_B   (130u * 1024u)
#define DSP_MEM_BUDGET_TLS_B     ( 50u * 1024u)
#define DSP_MEM_BUDGET_HTTP_B    ( 20u * 1024u)
#define DSP_MEM_BUDGET_JSON_B    ( 15u * 1024u)
#define DSP_MEM_BUDGET_JWT_B     ( 25u * 1024u)
#define DSP_MEM_BUDGET_DSP_SM_B  ( 20u * 1024u)

/**
 * @brief Log a heap snapshot to the ESP-IDF log output.
 *
 * Logs, under the "dsp_mem" tag:
 *   - Free internal RAM (bytes and KB)
 *   - Minimum free internal RAM since boot
 *   - Largest contiguous free block in internal RAM
 *   - Free PSRAM (if CONFIG_DSP_PSRAM_ENABLE && SPIRAM present)
 *   - A WARNING if free internal RAM < 30 KB (approaching budget limit)
 *
 * @param tag  Caller label printed with the snapshot, e.g. "after-tls".
 *             Must not be NULL.
 * @return ESP_OK always (logging failures are non-fatal).
 */
esp_err_t dsp_mem_report(const char *tag);

/**
 * @brief Return the current free internal RAM in bytes.
 *
 * On ESP_PLATFORM: wraps heap_caps_get_free_size(MALLOC_CAP_INTERNAL).
 * On host builds: returns a fixed sentinel value (DSP_MEM_HOST_FREE_B).
 */
uint32_t dsp_mem_free_internal(void);

/**
 * @brief Return the current free PSRAM in bytes, or 0 if unavailable.
 *
 * On ESP_PLATFORM with SPIRAM: wraps heap_caps_get_free_size(MALLOC_CAP_SPIRAM).
 * On ESP_PLATFORM without SPIRAM, or on host builds: returns 0.
 */
uint32_t dsp_mem_free_psram(void);

/**
 * @brief Sentinel returned by dsp_mem_free_internal() in host builds.
 *
 * Chosen to be above DSP_MEM_BUDGET_TOTAL_B so budget-check tests can
 * distinguish host stubs from a real constrained device.
 */
#define DSP_MEM_HOST_FREE_B  (512u * 1024u)

#ifdef __cplusplus
}
#endif
