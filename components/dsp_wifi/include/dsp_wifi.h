/**
 * @file dsp_wifi.h
 * @brief DSP Embedded WiFi connection manager.
 *
 * Manages the STA-mode WiFi lifecycle:
 *   connect → reconnect on drop → report IP-ready event → fail after retries.
 *
 * Credentials are stored in NVS (namespace "dsp_wifi", keys "ssid"/"pass").
 * Pass a non-NULL dsp_wifi_config_t to dsp_wifi_init() to override NVS values.
 *
 * The state machine transition function dsp_wifi_sm_next() is platform-
 * agnostic and can be unit-tested on the host without any ESP-IDF linkage.
 *
 * Used by: main (DSP-105), dsp_http start trigger (DSP-102)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "dsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * State machine types  (platform-agnostic, testable on host)
 * ------------------------------------------------------------------------- */

/** Connection manager states. */
typedef enum {
    DSP_WIFI_STATE_IDLE         = 0, /**< Not started.                       */
    DSP_WIFI_STATE_CONNECTING   = 1, /**< esp_wifi_connect() issued.         */
    DSP_WIFI_STATE_CONNECTED    = 2, /**< IP obtained, ready to use.         */
    DSP_WIFI_STATE_DISCONNECTED = 3, /**< Link lost, retry pending.          */
    DSP_WIFI_STATE_RECONNECTING = 4, /**< Waiting for reconnect timer.       */
    DSP_WIFI_STATE_FAILED       = 5, /**< Max retries exceeded.              */
} dsp_wifi_state_t;

/** Inputs that drive state transitions. */
typedef enum {
    DSP_WIFI_INPUT_CONNECT      = 0, /**< User calls dsp_wifi_connect().     */
    DSP_WIFI_INPUT_CONNECTED    = 1, /**< IP_EVENT_STA_GOT_IP fired.         */
    DSP_WIFI_INPUT_DISCONNECTED = 2, /**< WIFI_EVENT_STA_DISCONNECTED fired. */
    DSP_WIFI_INPUT_RETRY        = 3, /**< Reconnect timer expired.           */
    DSP_WIFI_INPUT_DISCONNECT   = 4, /**< User calls dsp_wifi_disconnect().  */
} dsp_wifi_input_t;

/** Events emitted to the application callback. */
typedef enum {
    DSP_WIFI_EVT_CONNECTED    = 0, /**< IP ready, server can start.          */
    DSP_WIFI_EVT_DISCONNECTED = 1, /**< Link lost, will retry.              */
    DSP_WIFI_EVT_RECONNECTING = 2, /**< Retry attempt N of max_retry.       */
    DSP_WIFI_EVT_FAILED       = 3, /**< Max retries exceeded, give up.      */
} dsp_wifi_event_t;

/**
 * @brief Pure state-machine transition function.
 *
 * Computes the next state given the current state and an input signal.
 * Updates *retry_count when a disconnect is processed; resets it on
 * reconnection.  If retry_count reaches max_retry on a disconnect the
 * function returns DSP_WIFI_STATE_FAILED.
 *
 * This function has NO side effects beyond modifying *retry_count and is
 * fully testable on the host without any ESP-IDF dependency.
 *
 * @param state        Current state.
 * @param input        Input signal.
 * @param retry_count  Pointer to the caller-managed retry counter.
 * @param max_retry    Maximum retries before FAILED (e.g. CONFIG_DSP_WIFI_MAX_RETRY).
 * @return Next state.
 */
dsp_wifi_state_t dsp_wifi_sm_next(dsp_wifi_state_t  state,
                                   dsp_wifi_input_t   input,
                                   int               *retry_count,
                                   int                max_retry);

/* -------------------------------------------------------------------------
 * Application configuration
 * ------------------------------------------------------------------------- */

/** Maximum SSID length (IEEE 802.11, null-terminated). */
#define DSP_WIFI_SSID_MAX_LEN  32
/** Maximum WPA2 passphrase length (null-terminated). */
#define DSP_WIFI_PASS_MAX_LEN  64

/**
 * @brief WiFi configuration passed to dsp_wifi_init().
 *
 * If ssid[0] == '\\0', credentials are loaded from NVS instead.
 */
typedef struct {
    char     ssid[DSP_WIFI_SSID_MAX_LEN]; /**< Network SSID.                */
    char     password[DSP_WIFI_PASS_MAX_LEN]; /**< WPA2 passphrase.         */
    int      max_retry;          /**< 0 → use CONFIG_DSP_WIFI_MAX_RETRY.    */
    uint32_t reconnect_delay_ms; /**< 0 → use CONFIG_DSP_WIFI_RECONNECT_DELAY_MS. */
} dsp_wifi_config_t;

/** Application event callback type. */
typedef void (*dsp_wifi_event_cb_t)(dsp_wifi_event_t event, void *user_data);

/* -------------------------------------------------------------------------
 * Platform API
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialise the WiFi manager.
 *
 * Initialises NVS, the TCP/IP stack, and the esp_event loop.  If cfg is
 * non-NULL and cfg->ssid is non-empty, those credentials are used directly.
 * Otherwise credentials are loaded from NVS ("dsp_wifi"/"ssid"+"pass").
 *
 * @param cfg  Configuration, or NULL to load all settings from NVS/Kconfig.
 * @return ESP_OK, ESP_ERR_NVS_NOT_FOUND if NVS has no stored credentials,
 *         or an IDF error code.
 */
esp_err_t dsp_wifi_init(const dsp_wifi_config_t *cfg);

/**
 * @brief Start the WiFi driver and attempt to connect.
 *
 * Transitions state machine to CONNECTING.  Events are delivered via the
 * registered callback.  Must be called after dsp_wifi_init().
 *
 * @return ESP_OK or an IDF error code.
 */
esp_err_t dsp_wifi_connect(void);

/**
 * @brief Disconnect and stop the WiFi driver.
 *
 * Transitions state machine to IDLE.  Safe to call in any state.
 */
void dsp_wifi_disconnect(void);

/**
 * @brief Deinitialise the WiFi manager and free resources.
 *
 * Calls dsp_wifi_disconnect() if connected.
 */
void dsp_wifi_deinit(void);

/**
 * @brief Query the current connection state.
 */
dsp_wifi_state_t dsp_wifi_get_state(void);

/**
 * @brief Register an application event callback.
 *
 * Only one callback is supported.  Pass NULL to clear.
 *
 * @param cb        Callback function, or NULL to deregister.
 * @param user_data Opaque pointer passed back in every callback invocation.
 */
void dsp_wifi_set_event_cb(dsp_wifi_event_cb_t cb, void *user_data);

/**
 * @brief Persist WiFi credentials to NVS.
 *
 * Stores ssid and password in the "dsp_wifi" NVS namespace so they are
 * available across resets without recompiling.
 *
 * @param ssid      Network SSID (max DSP_WIFI_SSID_MAX_LEN-1 chars).
 * @param password  WPA2 passphrase (max DSP_WIFI_PASS_MAX_LEN-1 chars).
 * @return ESP_OK, ESP_ERR_INVALID_ARG if ssid is NULL or empty, or an
 *         NVS error code.
 */
esp_err_t dsp_wifi_store_credentials(const char *ssid, const char *password);

#ifdef __cplusplus
}
#endif
