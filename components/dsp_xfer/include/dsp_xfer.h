/**
 * @file dsp_xfer.h
 * @brief DSP transfer state machine and slot table.
 *
 * Implements the provider-side transfer flow:
 *   POST /transfers/start  →  INITIAL → TRANSFERRING          (DSP-405)
 *   GET  /transfers/{id}   →  current state response           (DSP-406)
 *
 * States:
 *   INITIAL      – slot allocated, not yet started
 *   TRANSFERRING – data delivery in progress (Core 1 queue signalled)
 *   COMPLETED    – transfer completed successfully   (DSP-406)
 *   FAILED       – transfer aborted with error       (DSP-406)
 *
 * A notify callback (dsp_xfer_set_notify) is invoked whenever a slot
 * transitions to TRANSFERRING.  On ESP-IDF builds the application registers
 * a FreeRTOS queue sender here to trigger Core 1 data delivery.  The default
 * is NULL (no notification), which is the behaviour used in host unit tests.
 *
 * Capacity is controlled by CONFIG_DSP_MAX_CONCURRENT_TRANSFERS (default 2).
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "dsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */

/** Maximum simultaneous transfers tracked by the slot table. */
#define DSP_XFER_MAX     CONFIG_DSP_MAX_CONCURRENT_TRANSFERS

/** Maximum length (including NUL) of a process-ID string. */
#define DSP_XFER_PID_LEN 64u

/* -------------------------------------------------------------------------
 * Types
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_XFER_STATE_INITIAL      = 0, /**< Slot allocated, transfer not started. */
    DSP_XFER_STATE_TRANSFERRING = 1, /**< Data delivery in progress.            */
    DSP_XFER_STATE_COMPLETED    = 2, /**< Transfer finished successfully.       */
    DSP_XFER_STATE_FAILED       = 3, /**< Transfer aborted with error.          */
} dsp_xfer_state_t;

typedef enum {
    DSP_XFER_EVENT_START    = 0, /**< Start transfer (INITIAL → TRANSFERRING). */
    DSP_XFER_EVENT_COMPLETE = 1, /**< Complete transfer (TRANSFERRING → COMPLETED). */
    DSP_XFER_EVENT_FAIL     = 2, /**< Fail transfer (TRANSFERRING → FAILED).   */
} dsp_xfer_event_t;

/**
 * Callback invoked when a slot transitions to TRANSFERRING via
 * dsp_xfer_apply(idx, DSP_XFER_EVENT_START).
 *
 * @param idx  Slot index of the newly started transfer.
 */
typedef void (*dsp_xfer_notify_fn_t)(int idx);

/* -------------------------------------------------------------------------
 * State machine (pure function, always compiled)
 * ------------------------------------------------------------------------- */

/**
 * @brief Compute the next state given current state and event.
 *
 * Terminal states (COMPLETED, FAILED) absorb all events.
 */
dsp_xfer_state_t dsp_xfer_sm_next(dsp_xfer_state_t state, dsp_xfer_event_t event);

/* -------------------------------------------------------------------------
 * Module init / deinit
 * ------------------------------------------------------------------------- */

esp_err_t dsp_xfer_init(void);
void      dsp_xfer_deinit(void);
bool      dsp_xfer_is_initialized(void);

/* -------------------------------------------------------------------------
 * Notification hook
 * ------------------------------------------------------------------------- */

/**
 * @brief Register a callback to be called when a transfer starts.
 *
 * Pass NULL to clear the callback.  The callback is invoked from within
 * dsp_xfer_apply() when the DSP_XFER_EVENT_START event results in the
 * TRANSFERRING state.
 */
void dsp_xfer_set_notify(dsp_xfer_notify_fn_t fn);

/* -------------------------------------------------------------------------
 * Slot table management
 * ------------------------------------------------------------------------- */

/**
 * @brief Allocate a new transfer slot for the given process ID.
 * @return Slot index [0, DSP_XFER_MAX), or -1 if the table is full or
 *         process_id is NULL.
 */
int dsp_xfer_create(const char *process_id);

/**
 * @brief Apply an event to a slot, advancing its state.
 *
 * Also fires the notify callback when the transition is INITIAL → TRANSFERRING.
 *
 * @return New state, or DSP_XFER_STATE_INITIAL if idx is invalid.
 */
dsp_xfer_state_t dsp_xfer_apply(int idx, dsp_xfer_event_t event);

dsp_xfer_state_t dsp_xfer_get_state(int idx);
const char      *dsp_xfer_get_process_id(int idx);
int              dsp_xfer_find_by_pid(const char *pid);
int              dsp_xfer_count_active(void);
bool             dsp_xfer_is_active(int idx);

/**
 * @brief Directly load a transfer slot at index @p idx with the given state
 *        and process ID, bypassing the state machine and notify callback.
 *
 * Used exclusively by dsp_rtc_state_restore() to reconstruct the slot table
 * after a deep-sleep wake-up without replaying events or triggering Core 1.
 * Must be called after dsp_xfer_init() on an empty slot.
 *
 * @param idx         Slot index [0, DSP_XFER_MAX).
 * @param state       State to set directly.
 * @param process_id  NUL-terminated process ID (copied; max DSP_XFER_PID_LEN-1).
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if idx out of range or process_id is NULL.
 *         ESP_ERR_INVALID_STATE if dsp_xfer_init() has not been called.
 */
esp_err_t dsp_xfer_load_slot(int idx, dsp_xfer_state_t state, const char *process_id);

/* -------------------------------------------------------------------------
 * HTTP handler registration
 * ------------------------------------------------------------------------- */

/**
 * @brief Register transfer HTTP handlers with the dsp_http server.
 *
 * Registers:
 *   POST /transfers/start        – start a new transfer
 *   GET  /transfers/{id} (wild)  – query transfer state by process ID
 *
 * @return ESP_ERR_INVALID_STATE if dsp_xfer_init() has not been called.
 */
esp_err_t dsp_xfer_register_handlers(void);

#ifdef __cplusplus
}
#endif
