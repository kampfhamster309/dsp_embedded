/**
 * @file dsp_protocol.h
 * @brief DSP Embedded protocol task – Core 0 setup (DSP-502).
 *
 * Owns the full DSP protocol stack lifecycle:
 *   - TLS server context (dsp_tls)
 *   - HTTP server (dsp_http)
 *   - Catalog state machine (dsp_catalog)
 *   - Negotiation state machine (dsp_neg)
 *   - Transfer state machine (dsp_xfer)
 *
 * On ESP_PLATFORM the stack runs inside a dedicated FreeRTOS task pinned to
 * Core 0 via xTaskCreatePinnedToCore().  The task logs its core ID at startup
 * so the AC ("confirmed pinned to Core 0 by xTaskGetCoreID()") is verifiable
 * from the serial monitor.
 *
 * On the host build the initialisation runs synchronously inside
 * dsp_protocol_start() so the init logic can be unit-tested without RTOS.
 *
 * Typical usage (after WiFi is up):
 *   dsp_protocol_start(&g_shared);
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "dsp_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Stack size for the protocol task in bytes. */
#define DSP_PROTOCOL_TASK_STACK  8192U

/** FreeRTOS priority for the protocol task (higher than idle, lower than WiFi). */
#define DSP_PROTOCOL_TASK_PRIO   5U

/** CPU core the protocol task is pinned to. */
#define DSP_PROTOCOL_TASK_CORE   0

/**
 * @brief Start the DSP protocol stack.
 *
 * On ESP_PLATFORM: creates a FreeRTOS task pinned to Core 0 that initialises
 * TLS, HTTP, catalog, negotiation, and transfer modules, registers all HTTP
 * handlers, starts the HTTP server, sets sh->core0_ready = 1, and then loops
 * handling incoming requests.
 *
 * On host: runs the same initialisation sequence synchronously in the calling
 * context (no background task created).  dsp_http_start() returns ESP_FAIL on
 * host — this is logged as a warning and execution continues so that all other
 * components can be initialised and verified by unit tests.
 *
 * @param sh  Initialised shared state (from dsp_shared_init()).
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if sh is NULL.
 *         ESP_ERR_NO_MEM if FreeRTOS task creation fails (ESP only).
 */
esp_err_t dsp_protocol_start(dsp_shared_t *sh);

/**
 * @brief Stop the protocol stack and release all resources.
 *
 * Stops the HTTP server, deinitialises all state machines and TLS context.
 * On ESP_PLATFORM the task is also deleted.  Safe to call if not started.
 */
void dsp_protocol_stop(void);

/**
 * @brief Returns true if dsp_protocol_start() has been called and the
 *        initialisation sequence has completed.
 */
bool dsp_protocol_is_running(void);

#ifdef __cplusplus
}
#endif
