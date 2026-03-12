/**
 * @file dsp_xfer.c
 * @brief DSP transfer state machine and slot table implementation.
 */

#include "dsp_xfer.h"
#include "dsp_build.h"
#include "dsp_http.h"
#include "dsp_json.h"
#include "dsp_jsonld_ctx.h"
#include "dsp_msg.h"
#include "dsp_neg.h"
#include "esp_log.h"
#include <string.h>

#ifdef ESP_PLATFORM
static const char *TAG = "dsp_xfer";
#endif

/* -------------------------------------------------------------------------
 * Internal types
 * ------------------------------------------------------------------------- */

typedef struct {
    dsp_xfer_state_t state;
    char             process_id[DSP_XFER_PID_LEN];
    bool             active;
} dsp_xfer_entry_t;

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */

static dsp_xfer_entry_t  s_table[DSP_XFER_MAX];
static bool              s_initialized = false;
static dsp_xfer_notify_fn_t s_notify   = NULL;

/* -------------------------------------------------------------------------
 * State machine (pure function, always compiled)
 * ------------------------------------------------------------------------- */

dsp_xfer_state_t dsp_xfer_sm_next(dsp_xfer_state_t state, dsp_xfer_event_t event)
{
    switch (state) {
        case DSP_XFER_STATE_INITIAL:
            if (event == DSP_XFER_EVENT_START) {
                return DSP_XFER_STATE_TRANSFERRING;
            }
            return DSP_XFER_STATE_INITIAL;

        case DSP_XFER_STATE_TRANSFERRING:
            switch (event) {
                case DSP_XFER_EVENT_COMPLETE: return DSP_XFER_STATE_COMPLETED;
                case DSP_XFER_EVENT_FAIL:     return DSP_XFER_STATE_FAILED;
                default:                      return DSP_XFER_STATE_TRANSFERRING;
            }

        case DSP_XFER_STATE_COMPLETED:
        case DSP_XFER_STATE_FAILED:
        default:
            return state;
    }
}

/* -------------------------------------------------------------------------
 * Module init / deinit
 * ------------------------------------------------------------------------- */

esp_err_t dsp_xfer_init(void)
{
    if (!s_initialized) {
        memset(s_table, 0, sizeof(s_table));
        s_initialized = true;
    }
    return ESP_OK;
}

void dsp_xfer_deinit(void)
{
    memset(s_table, 0, sizeof(s_table));
    s_initialized = false;
}

bool dsp_xfer_is_initialized(void)
{
    return s_initialized;
}

/* -------------------------------------------------------------------------
 * Notification hook
 * ------------------------------------------------------------------------- */

void dsp_xfer_set_notify(dsp_xfer_notify_fn_t fn)
{
    s_notify = fn;
}

/* -------------------------------------------------------------------------
 * Slot table management
 * ------------------------------------------------------------------------- */

int dsp_xfer_create(const char *process_id)
{
    if (!process_id) {
        return -1;
    }
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        if (!s_table[i].active) {
            s_table[i].active = true;
            s_table[i].state  = DSP_XFER_STATE_INITIAL;
            strncpy(s_table[i].process_id, process_id, DSP_XFER_PID_LEN - 1u);
            s_table[i].process_id[DSP_XFER_PID_LEN - 1u] = '\0';
            return i;
        }
    }
    return -1; /* table full */
}

dsp_xfer_state_t dsp_xfer_apply(int idx, dsp_xfer_event_t event)
{
    if (idx < 0 || idx >= DSP_XFER_MAX || !s_table[idx].active) {
        return DSP_XFER_STATE_INITIAL;
    }
    dsp_xfer_state_t old_state = s_table[idx].state;
    s_table[idx].state = dsp_xfer_sm_next(old_state, event);

    /* Fire notify only on the actual transition into TRANSFERRING. */
    if (old_state != DSP_XFER_STATE_TRANSFERRING &&
        s_table[idx].state == DSP_XFER_STATE_TRANSFERRING &&
        s_notify != NULL) {
        s_notify(idx);
    }
    return s_table[idx].state;
}

dsp_xfer_state_t dsp_xfer_get_state(int idx)
{
    if (idx < 0 || idx >= DSP_XFER_MAX || !s_table[idx].active) {
        return DSP_XFER_STATE_INITIAL;
    }
    return s_table[idx].state;
}

const char *dsp_xfer_get_process_id(int idx)
{
    if (idx < 0 || idx >= DSP_XFER_MAX || !s_table[idx].active) {
        return NULL;
    }
    return s_table[idx].process_id;
}

int dsp_xfer_find_by_pid(const char *pid)
{
    if (!pid) {
        return -1;
    }
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        if (s_table[i].active &&
            strncmp(s_table[i].process_id, pid, DSP_XFER_PID_LEN) == 0) {
            return i;
        }
    }
    return -1;
}

int dsp_xfer_count_active(void)
{
    int count = 0;
    for (int i = 0; i < DSP_XFER_MAX; i++) {
        if (s_table[i].active) {
            count++;
        }
    }
    return count;
}

bool dsp_xfer_is_active(int idx)
{
    if (idx < 0 || idx >= DSP_XFER_MAX) {
        return false;
    }
    return s_table[idx].active;
}

/* -------------------------------------------------------------------------
 * HTTP handlers
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM

#include "esp_http_server.h"

/** Buffer size for reading request body and building response. */
#define XFER_BUF_SIZE 1024u

/**
 * POST /transfers/start
 *
 * Receives a TransferRequestMessage.  Validates that the referenced
 * negotiation is in AGREED state, allocates a transfer slot, advances it
 * to TRANSFERRING, fires the notify callback, and responds with a
 * TransferStartMessage.
 *
 * The endpoint URL in the response uses CONFIG_DSP_PROVIDER_ID as a
 * placeholder until DSP-501 defines the actual shared-memory data address.
 */
static esp_err_t xfer_start_post_handler(httpd_req_t *req)
{
    char body[XFER_BUF_SIZE];
    int  recv_len = httpd_req_recv(req, body, sizeof(body) - 1u);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
        return ESP_FAIL;
    }
    body[recv_len] = '\0';

    if (dsp_msg_validate_transfer_request(body) != DSP_MSG_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "invalid TransferRequestMessage");
        return ESP_FAIL;
    }

    cJSON *msg = dsp_json_parse(body);
    if (!msg) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid JSON");
        return ESP_FAIL;
    }

    char pid[DSP_XFER_PID_LEN] = "";
    dsp_json_get_string(msg, DSP_JSONLD_FIELD_PROCESS_ID, pid, sizeof(pid));
    dsp_json_delete(msg);

    if (pid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "missing dspace:processId");
        return ESP_FAIL;
    }

    /* Prerequisite: the referenced negotiation must be in AGREED state. */
    int neg_idx = dsp_neg_find_by_cpid(pid);
    if (neg_idx < 0 || dsp_neg_get_state(neg_idx) != DSP_NEG_STATE_AGREED) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "negotiation not in AGREED state");
        return ESP_FAIL;
    }

    int idx = dsp_xfer_create(pid);
    if (idx < 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "transfer table full");
        return ESP_FAIL;
    }

    dsp_xfer_apply(idx, DSP_XFER_EVENT_START);

    char resp[XFER_BUF_SIZE];
    /* Endpoint URL placeholder – replaced in DSP-501 with the real data address. */
    dsp_build_transfer_start(resp, sizeof(resp), pid, CONFIG_DSP_PROVIDER_ID);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    ESP_LOGI(TAG, "transfer started: slot=%d pid=%s", idx, pid);
    return ESP_OK;
}

esp_err_t dsp_xfer_register_handlers(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return dsp_http_register_handler("/transfers/start",
                                      DSP_HTTP_POST,
                                      xfer_start_post_handler);
}

#else /* host build */

static esp_err_t xfer_start_host_stub(void *req)
{
    (void)req;
    return ESP_OK;
}

esp_err_t dsp_xfer_register_handlers(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return dsp_http_register_handler("/transfers/start",
                                      DSP_HTTP_POST,
                                      xfer_start_host_stub);
}

#endif /* ESP_PLATFORM */
