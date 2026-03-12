/**
 * @file dsp_neg.h
 * @brief DSP contract negotiation state machine and slot table.
 *
 * Implements the provider-side negotiation state machine defined in the
 * Dataspace Protocol specification (v2024/1).  The node is a provider-only
 * participant (DEV-003), so consumer-initiated states are included but
 * consumer-initiated events that do not apply in provider role are silently
 * ignored (state is preserved).
 *
 * State machine (provider view):
 *
 *   REQUESTED ──OFFER──► OFFERED ──AGREE──► AGREED ──VERIFY──► VERIFIED
 *       │                   │                 │                    │
 *       └──AGREE──► AGREED  └──TERMINATE──┐  └──TERMINATE──┐    FINALIZE
 *       └──TERMINATE──┐                   │                │      │
 *                     ▼                   ▼                ▼      ▼
 *                 TERMINATED ◄────────────────────── TERMINATED FINALIZED
 *
 * FINALIZED and TERMINATED are terminal states: all further events are
 * absorbed without transition.
 *
 * HTTP endpoints (registered by dsp_neg_register_handlers):
 *   POST /negotiations/offers        – receive ContractRequestMessage → OFFERED
 *   POST /negotiations/{id}/agree    – receive AgreementVerification  → VERIFIED
 *                                      (DSP-403)
 *   POST /negotiations/terminate     – receive TerminationMessage → TERMINATED
 *                                      (DSP-404, behind CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE)
 *
 * The platform-agnostic SM and slot table are always compiled and are
 * suitable for host-native unit testing.  HTTP handler bodies are guarded
 * by #ifdef ESP_PLATFORM; host builds register stub handlers.
 *
 * Memory: DSP_NEG_MAX × sizeof(dsp_neg_entry_t) ≤ 1 KB (4 entries ×
 * ~136 bytes), well within the 20 KB DSP_SM budget.
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "dsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Limits
 * ------------------------------------------------------------------------- */

/** Maximum number of concurrently tracked negotiations. */
#define DSP_NEG_MAX     CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS

/** Maximum length of a PID string including NUL terminator. */
#define DSP_NEG_PID_LEN 64u

/* -------------------------------------------------------------------------
 * State and event enumerations
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_NEG_STATE_INITIAL    = 0, /**< Slot is free / not yet allocated       */
    DSP_NEG_STATE_REQUESTED  = 1, /**< Consumer sent ContractRequestMessage    */
    DSP_NEG_STATE_OFFERED    = 2, /**< Provider sent ContractOfferMessage      */
    DSP_NEG_STATE_AGREED     = 3, /**< Provider sent ContractAgreementMessage  */
    DSP_NEG_STATE_VERIFIED   = 4, /**< Consumer sent AgreementVerification     */
    DSP_NEG_STATE_FINALIZED  = 5, /**< Provider sent FINALIZED event (terminal)*/
    DSP_NEG_STATE_TERMINATED = 6, /**< Either party terminated (terminal)      */
} dsp_neg_state_t;

typedef enum {
    DSP_NEG_EVENT_OFFER      = 0, /**< Provider sends ContractOfferMessage     */
    DSP_NEG_EVENT_AGREE      = 1, /**< Provider sends ContractAgreementMessage */
    DSP_NEG_EVENT_VERIFY     = 2, /**< Consumer sends AgreementVerification    */
    DSP_NEG_EVENT_FINALIZE   = 3, /**< Provider sends FINALIZED event message  */
    DSP_NEG_EVENT_TERMINATE  = 4, /**< Either sends TerminationMessage         */
} dsp_neg_event_t;

/* -------------------------------------------------------------------------
 * Platform-agnostic state machine (always compiled)
 * ------------------------------------------------------------------------- */

/**
 * Compute the next negotiation state given the current state and event.
 *
 * Pure function — no side effects, no global state.  Safe to call from
 * any context.  Unhandled events in a given state leave the state unchanged.
 *
 * @param state  Current negotiation state.
 * @param event  Incoming event.
 * @return       Next state (may equal @p state if event is not handled).
 */
dsp_neg_state_t dsp_neg_sm_next(dsp_neg_state_t state, dsp_neg_event_t event);

/* -------------------------------------------------------------------------
 * Negotiation slot table
 * ------------------------------------------------------------------------- */

/**
 * Initialise the negotiation table.  Clears all slots.
 * Safe to call multiple times.
 *
 * @return ESP_OK always.
 */
esp_err_t dsp_neg_init(void);

/**
 * Clear all negotiation slots and reset to uninitialised state.
 */
void dsp_neg_deinit(void);

/**
 * Allocate a new negotiation slot in the REQUESTED state.
 *
 * @param consumer_pid  NUL-terminated consumer PID string.
 * @param provider_pid  NUL-terminated provider PID string.
 * @return              Non-negative slot index on success, -1 if the table
 *                      is full or either PID argument is NULL.
 */
int dsp_neg_create(const char *consumer_pid, const char *provider_pid);

/**
 * Apply an event to the negotiation at slot @p idx.
 *
 * @return  New state, or DSP_NEG_STATE_INITIAL if @p idx is invalid /
 *          the slot is inactive.
 */
dsp_neg_state_t dsp_neg_apply(int idx, dsp_neg_event_t event);

/**
 * Get the current state of the negotiation at slot @p idx.
 *
 * @return  Current state, or DSP_NEG_STATE_INITIAL for an invalid slot.
 */
dsp_neg_state_t dsp_neg_get_state(int idx);

/**
 * Retrieve the consumer PID stored in slot @p idx.
 *
 * @return  NUL-terminated consumer PID string, or NULL for an invalid slot.
 */
const char *dsp_neg_get_consumer_pid(int idx);

/**
 * Retrieve the provider PID stored in slot @p idx.
 *
 * @return  NUL-terminated provider PID string, or NULL for an invalid slot.
 */
const char *dsp_neg_get_provider_pid(int idx);

/**
 * Find the slot index for the given consumer PID.
 *
 * @return  Non-negative slot index, or -1 if not found or @p cpid is NULL.
 */
int dsp_neg_find_by_cpid(const char *cpid);

/**
 * Return the number of currently active (allocated) negotiation slots.
 */
int dsp_neg_count_active(void);

/**
 * Return true if slot @p idx is allocated and active.
 */
bool dsp_neg_is_active(int idx);

/* -------------------------------------------------------------------------
 * HTTP handler registration
 * ------------------------------------------------------------------------- */

/**
 * Register all negotiation HTTP handlers with the dsp_http server.
 *
 * DSP-402: POST /negotiations/offers
 * DSP-403: POST /negotiations/{id}/agree   (added later)
 * DSP-404: POST /negotiations/terminate    (added later)
 *
 * Must be called after dsp_neg_init().
 *
 * @return ESP_OK, ESP_ERR_INVALID_STATE if not initialised, or an error
 *         from dsp_http_register_handler().
 */
esp_err_t dsp_neg_register_handlers(void);

#ifdef __cplusplus
}
#endif
