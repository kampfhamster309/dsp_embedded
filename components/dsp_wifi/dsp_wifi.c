/**
 * @file dsp_wifi.c
 * @brief DSP Embedded WiFi connection manager implementation.
 *
 * The state machine transition function dsp_wifi_sm_next() is compiled
 * unconditionally (no ESP_PLATFORM guard) so it is available in host-native
 * unit tests without any ESP-IDF linkage.
 *
 * The platform-specific driver code (esp_wifi_*, NVS, esp_timer) is guarded
 * by #ifdef ESP_PLATFORM.
 */

#include "dsp_wifi.h"
#include "dsp_config.h"

#include <string.h>

/* =========================================================================
 * State machine – platform-agnostic, always compiled
 * ========================================================================= */

dsp_wifi_state_t dsp_wifi_sm_next(dsp_wifi_state_t  state,
                                   dsp_wifi_input_t   input,
                                   int               *retry_count,
                                   int                max_retry)
{
    /* DISCONNECT from any state → IDLE, reset retries */
    if (input == DSP_WIFI_INPUT_DISCONNECT) {
        *retry_count = 0;
        return DSP_WIFI_STATE_IDLE;
    }

    switch (state) {

    case DSP_WIFI_STATE_IDLE:
        if (input == DSP_WIFI_INPUT_CONNECT) {
            *retry_count = 0;
            return DSP_WIFI_STATE_CONNECTING;
        }
        break;

    case DSP_WIFI_STATE_CONNECTING:
        if (input == DSP_WIFI_INPUT_CONNECTED) {
            *retry_count = 0;
            return DSP_WIFI_STATE_CONNECTED;
        }
        if (input == DSP_WIFI_INPUT_DISCONNECTED) {
            (*retry_count)++;
            if (*retry_count >= max_retry) {
                return DSP_WIFI_STATE_FAILED;
            }
            return DSP_WIFI_STATE_RECONNECTING;
        }
        break;

    case DSP_WIFI_STATE_CONNECTED:
        if (input == DSP_WIFI_INPUT_DISCONNECTED) {
            (*retry_count)++;
            if (*retry_count >= max_retry) {
                return DSP_WIFI_STATE_FAILED;
            }
            return DSP_WIFI_STATE_RECONNECTING;
        }
        break;

    case DSP_WIFI_STATE_RECONNECTING:
        if (input == DSP_WIFI_INPUT_RETRY) {
            return DSP_WIFI_STATE_CONNECTING;
        }
        if (input == DSP_WIFI_INPUT_CONNECTED) {
            /* Rare: connected before retry timer fired */
            *retry_count = 0;
            return DSP_WIFI_STATE_CONNECTED;
        }
        if (input == DSP_WIFI_INPUT_DISCONNECTED) {
            /* Additional disconnect while waiting to retry: count it */
            (*retry_count)++;
            if (*retry_count >= max_retry) {
                return DSP_WIFI_STATE_FAILED;
            }
        }
        break;

    case DSP_WIFI_STATE_FAILED:
        /* Terminal state until explicit DISCONNECT/re-init */
        break;

    case DSP_WIFI_STATE_DISCONNECTED:
        /* Transient state; driver promotes immediately to RECONNECTING */
        if (input == DSP_WIFI_INPUT_RETRY) {
            return DSP_WIFI_STATE_CONNECTING;
        }
        break;
    }

    return state; /* unhandled input – stay in current state */
}

/* =========================================================================
 * ESP32 platform implementation
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"

static const char *TAG = "dsp_wifi";

#define NVS_NAMESPACE "dsp_wifi"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "pass"

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */

static dsp_wifi_state_t   s_state      = DSP_WIFI_STATE_IDLE;
static int                s_retry      = 0;
static int                s_max_retry  = CONFIG_DSP_WIFI_MAX_RETRY;
static uint32_t           s_delay_ms   = CONFIG_DSP_WIFI_RECONNECT_DELAY_MS;
static dsp_wifi_event_cb_t s_cb        = NULL;
static void              *s_cb_data    = NULL;
static esp_timer_handle_t s_retry_timer = NULL;
static esp_netif_t       *s_netif      = NULL;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static void emit(dsp_wifi_event_t evt)
{
    if (s_cb) {
        s_cb(evt, s_cb_data);
    }
}

static void apply_transition(dsp_wifi_input_t input)
{
    dsp_wifi_state_t next = dsp_wifi_sm_next(s_state, input,
                                              &s_retry, s_max_retry);
    if (next == s_state && input != DSP_WIFI_INPUT_DISCONNECT) {
        return;
    }

    ESP_LOGD(TAG, "SM %d -[%d]-> %d (retry=%d)", s_state, input, next, s_retry);
    s_state = next;

    switch (next) {
    case DSP_WIFI_STATE_CONNECTED:
        ESP_LOGI(TAG, "connected (retries reset)");
        emit(DSP_WIFI_EVT_CONNECTED);
        break;
    case DSP_WIFI_STATE_RECONNECTING:
        ESP_LOGW(TAG, "disconnected, reconnecting (attempt %d/%d)",
                 s_retry, s_max_retry);
        emit(DSP_WIFI_EVT_RECONNECTING);
        esp_timer_start_once(s_retry_timer,
                             (uint64_t)s_delay_ms * 1000ULL);
        break;
    case DSP_WIFI_STATE_FAILED:
        ESP_LOGE(TAG, "max retries (%d) exceeded, giving up", s_max_retry);
        emit(DSP_WIFI_EVT_FAILED);
        break;
    case DSP_WIFI_STATE_IDLE:
        ESP_LOGI(TAG, "disconnected by request");
        break;
    default:
        break;
    }
}

static void retry_timer_cb(void *arg)
{
    (void)arg;
    ESP_LOGD(TAG, "retry timer fired");
    esp_wifi_connect();
    apply_transition(DSP_WIFI_INPUT_RETRY);
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        emit(DSP_WIFI_EVT_DISCONNECTED);
        apply_transition(DSP_WIFI_INPUT_DISCONNECTED);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        apply_transition(DSP_WIFI_INPUT_CONNECTED);
    }
}

/* -------------------------------------------------------------------------
 * NVS credential helpers
 * ------------------------------------------------------------------------- */

static esp_err_t nvs_load_credentials(char *ssid, size_t ssid_len,
                                       char *pass, size_t pass_len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_str(h, NVS_KEY_SSID, ssid, &ssid_len);
    if (err == ESP_OK) {
        err = nvs_get_str(h, NVS_KEY_PASS, pass, &pass_len);
    }
    nvs_close(h);
    return err;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

esp_err_t dsp_wifi_init(const dsp_wifi_config_t *cfg)
{
    esp_err_t err;

    /* NVS must be initialised before anything else */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init: 0x%x", err);
        return err;
    }

    /* TCP/IP stack and default event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_netif = esp_netif_create_default_wifi_sta();

    /* WiFi driver */
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    /* Credentials */
    wifi_config_t sta_cfg = {0};

    if (cfg && cfg->ssid[0] != '\0') {
        strncpy((char *)sta_cfg.sta.ssid,
                cfg->ssid, sizeof(sta_cfg.sta.ssid) - 1);
        strncpy((char *)sta_cfg.sta.password,
                cfg->password, sizeof(sta_cfg.sta.password) - 1);
        if (cfg->max_retry > 0) {
            s_max_retry = cfg->max_retry;
        }
        if (cfg->reconnect_delay_ms > 0) {
            s_delay_ms = cfg->reconnect_delay_ms;
        }
    } else {
        char ssid[DSP_WIFI_SSID_MAX_LEN] = {0};
        char pass[DSP_WIFI_PASS_MAX_LEN] = {0};
        err = nvs_load_credentials(ssid, sizeof(ssid), pass, sizeof(pass));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "no WiFi credentials in NVS (err 0x%x)", err);
            return ESP_ERR_NVS_NOT_FOUND;
        }
        strncpy((char *)sta_cfg.sta.ssid,
                ssid, sizeof(sta_cfg.sta.ssid) - 1);
        strncpy((char *)sta_cfg.sta.password,
                pass, sizeof(sta_cfg.sta.password) - 1);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    /* Event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    /* Reconnect timer (one-shot, restarted on each disconnect) */
    esp_timer_create_args_t timer_args = {
        .callback = retry_timer_cb,
        .name     = "dsp_wifi_retry",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_retry_timer));

    s_state = DSP_WIFI_STATE_IDLE;
    s_retry = 0;

    ESP_LOGI(TAG, "initialised (max_retry=%d delay=%"PRIu32"ms)",
             s_max_retry, s_delay_ms);
    return ESP_OK;
}

esp_err_t dsp_wifi_connect(void)
{
    if (s_state != DSP_WIFI_STATE_IDLE) {
        return ESP_ERR_INVALID_STATE;
    }
    apply_transition(DSP_WIFI_INPUT_CONNECT);
    return esp_wifi_start();
}

void dsp_wifi_disconnect(void)
{
    esp_timer_stop(s_retry_timer);
    esp_wifi_stop();
    apply_transition(DSP_WIFI_INPUT_DISCONNECT);
}

void dsp_wifi_deinit(void)
{
    dsp_wifi_disconnect();
    if (s_retry_timer) {
        esp_timer_delete(s_retry_timer);
        s_retry_timer = NULL;
    }
    esp_wifi_deinit();
    if (s_netif) {
        esp_netif_destroy_default_wifi(s_netif);
        s_netif = NULL;
    }
}

dsp_wifi_state_t dsp_wifi_get_state(void)
{
    return s_state;
}

void dsp_wifi_set_event_cb(dsp_wifi_event_cb_t cb, void *user_data)
{
    s_cb      = cb;
    s_cb_data = user_data;
}

esp_err_t dsp_wifi_store_credentials(const char *ssid, const char *password)
{
    if (!ssid || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (!password) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_str(h, NVS_KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(h, NVS_KEY_PASS, password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "credentials stored (ssid='%s')", ssid);
    }
    return err;
}

#else /* !ESP_PLATFORM – host-native stubs */

#include "esp_err.h"
#include <stdbool.h>

static dsp_wifi_state_t    s_state   = DSP_WIFI_STATE_IDLE;
static dsp_wifi_event_cb_t s_cb      = NULL;
static void               *s_cb_data = NULL;

esp_err_t dsp_wifi_init(const dsp_wifi_config_t *cfg)
{
    (void)cfg;
    s_state = DSP_WIFI_STATE_IDLE;
    return ESP_OK;     /* Init stub succeeds so tests can exercise post-init paths */
}

esp_err_t dsp_wifi_connect(void)
{
    /* Real WiFi not available on host */
    return ESP_FAIL;
}

void dsp_wifi_disconnect(void)
{
    s_state = DSP_WIFI_STATE_IDLE;
}

void dsp_wifi_deinit(void)
{
    dsp_wifi_disconnect();
    s_cb      = NULL;
    s_cb_data = NULL;
}

dsp_wifi_state_t dsp_wifi_get_state(void)
{
    return s_state;
}

void dsp_wifi_set_event_cb(dsp_wifi_event_cb_t cb, void *user_data)
{
    s_cb      = cb;
    s_cb_data = user_data;
}

esp_err_t dsp_wifi_store_credentials(const char *ssid, const char *password)
{
    if (!ssid || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (!password) {
        return ESP_ERR_INVALID_ARG;
    }
    /* NVS not available on host */
    return ESP_FAIL;
}

#endif /* ESP_PLATFORM */
