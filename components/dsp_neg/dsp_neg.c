/**
 * @file dsp_neg.c
 * @brief DSP contract negotiation state machine and slot table implementation.
 */

#include "dsp_neg.h"
#include "dsp_build.h"
#include "dsp_http.h"
#include "dsp_msg.h"
#include "esp_log.h"
#include <string.h>

#ifdef ESP_PLATFORM
static const char *TAG = "dsp_neg";
#endif

/* -------------------------------------------------------------------------
 * Internal types
 * ------------------------------------------------------------------------- */

typedef struct {
    dsp_neg_state_t state;
    char            consumer_pid[DSP_NEG_PID_LEN];
    char            provider_pid[DSP_NEG_PID_LEN];
    bool            active;
} dsp_neg_entry_t;

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */

static dsp_neg_entry_t s_table[DSP_NEG_MAX];
static bool            s_initialized = false;

/* -------------------------------------------------------------------------
 * State machine (pure function, always compiled)
 * ------------------------------------------------------------------------- */

dsp_neg_state_t dsp_neg_sm_next(dsp_neg_state_t state, dsp_neg_event_t event)
{
    switch (state) {
        case DSP_NEG_STATE_REQUESTED:
            switch (event) {
                case DSP_NEG_EVENT_OFFER:     return DSP_NEG_STATE_OFFERED;
                case DSP_NEG_EVENT_AGREE:     return DSP_NEG_STATE_AGREED;
                case DSP_NEG_EVENT_TERMINATE: return DSP_NEG_STATE_TERMINATED;
                default:                      return DSP_NEG_STATE_REQUESTED;
            }

        case DSP_NEG_STATE_OFFERED:
            switch (event) {
                case DSP_NEG_EVENT_AGREE:     return DSP_NEG_STATE_AGREED;
                case DSP_NEG_EVENT_TERMINATE: return DSP_NEG_STATE_TERMINATED;
                default:                      return DSP_NEG_STATE_OFFERED;
            }

        case DSP_NEG_STATE_AGREED:
            switch (event) {
                case DSP_NEG_EVENT_VERIFY:    return DSP_NEG_STATE_VERIFIED;
                case DSP_NEG_EVENT_TERMINATE: return DSP_NEG_STATE_TERMINATED;
                default:                      return DSP_NEG_STATE_AGREED;
            }

        case DSP_NEG_STATE_VERIFIED:
            switch (event) {
                case DSP_NEG_EVENT_FINALIZE:  return DSP_NEG_STATE_FINALIZED;
                case DSP_NEG_EVENT_TERMINATE: return DSP_NEG_STATE_TERMINATED;
                default:                      return DSP_NEG_STATE_VERIFIED;
            }

        case DSP_NEG_STATE_FINALIZED:
        case DSP_NEG_STATE_TERMINATED:
        case DSP_NEG_STATE_INITIAL:
        default:
            return state;
    }
}

/* -------------------------------------------------------------------------
 * Slot table management
 * ------------------------------------------------------------------------- */

esp_err_t dsp_neg_init(void)
{
    if (!s_initialized) {
        memset(s_table, 0, sizeof(s_table));
        s_initialized = true;
    }
    return ESP_OK;
}

void dsp_neg_deinit(void)
{
    memset(s_table, 0, sizeof(s_table));
    s_initialized = false;
}

int dsp_neg_create(const char *consumer_pid, const char *provider_pid)
{
    if (!consumer_pid || !provider_pid) {
        return -1;
    }

    for (int i = 0; i < DSP_NEG_MAX; i++) {
        if (!s_table[i].active) {
            s_table[i].active = true;
            s_table[i].state  = DSP_NEG_STATE_REQUESTED;
            strncpy(s_table[i].consumer_pid, consumer_pid, DSP_NEG_PID_LEN - 1u);
            s_table[i].consumer_pid[DSP_NEG_PID_LEN - 1u] = '\0';
            strncpy(s_table[i].provider_pid, provider_pid, DSP_NEG_PID_LEN - 1u);
            s_table[i].provider_pid[DSP_NEG_PID_LEN - 1u] = '\0';
            return i;
        }
    }
    return -1; /* table full */
}

dsp_neg_state_t dsp_neg_apply(int idx, dsp_neg_event_t event)
{
    if (idx < 0 || idx >= DSP_NEG_MAX || !s_table[idx].active) {
        return DSP_NEG_STATE_INITIAL;
    }
    s_table[idx].state = dsp_neg_sm_next(s_table[idx].state, event);
    return s_table[idx].state;
}

dsp_neg_state_t dsp_neg_get_state(int idx)
{
    if (idx < 0 || idx >= DSP_NEG_MAX || !s_table[idx].active) {
        return DSP_NEG_STATE_INITIAL;
    }
    return s_table[idx].state;
}

const char *dsp_neg_get_consumer_pid(int idx)
{
    if (idx < 0 || idx >= DSP_NEG_MAX || !s_table[idx].active) {
        return NULL;
    }
    return s_table[idx].consumer_pid;
}

const char *dsp_neg_get_provider_pid(int idx)
{
    if (idx < 0 || idx >= DSP_NEG_MAX || !s_table[idx].active) {
        return NULL;
    }
    return s_table[idx].provider_pid;
}

int dsp_neg_find_by_cpid(const char *cpid)
{
    if (!cpid) {
        return -1;
    }
    for (int i = 0; i < DSP_NEG_MAX; i++) {
        if (s_table[i].active &&
            strncmp(s_table[i].consumer_pid, cpid, DSP_NEG_PID_LEN) == 0) {
            return i;
        }
    }
    return -1;
}

int dsp_neg_count_active(void)
{
    int count = 0;
    for (int i = 0; i < DSP_NEG_MAX; i++) {
        if (s_table[i].active) {
            count++;
        }
    }
    return count;
}

bool dsp_neg_is_active(int idx)
{
    if (idx < 0 || idx >= DSP_NEG_MAX) {
        return false;
    }
    return s_table[idx].active;
}

/* -------------------------------------------------------------------------
 * HTTP handlers
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM

#include "esp_http_server.h"
#include "dsp_json.h"
#include "dsp_jsonld_ctx.h"

/** Buffer size for reading request body and building response. */
#define NEG_BUF_SIZE 1024u

/**
 * Extract the negotiation process-ID segment from a path of the form
 * /negotiations/<id>[/suffix].  Writes at most cap-1 bytes to out.
 */
static void extract_neg_id(const char *uri, char *out, size_t cap)
{
    static const char prefix[] = "/negotiations/";
    const size_t      plen     = sizeof(prefix) - 1u;

    if (!uri || strncmp(uri, prefix, plen) != 0) {
        out[0] = '\0';
        return;
    }

    const char *start = uri + plen;
    const char *end   = strchr(start, '/');
    size_t      len   = (end != NULL) ? (size_t)(end - start) : strlen(start);

    if (len >= cap) {
        len = cap - 1u;
    }
    memcpy(out, start, len);
    out[len] = '\0';
}

/**
 * POST /negotiations/offers
 *
 * Receives a ContractRequestMessage.  Allocates a slot, applies the OFFER
 * event (REQUESTED → OFFERED), and responds with a
 * ContractNegotiationEventMessage carrying state OFFERED.
 */
static esp_err_t offers_post_handler(httpd_req_t *req)
{
    char body[NEG_BUF_SIZE];
    int  recv_len = httpd_req_recv(req, body, sizeof(body) - 1u);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
        return ESP_FAIL;
    }
    body[recv_len] = '\0';

    if (dsp_msg_validate_catalog_request(body) == DSP_MSG_OK) {
        /* caller accidentally sent a catalog request — wrong endpoint */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "wrong message type");
        return ESP_FAIL;
    }

    /* Minimal field extraction: get processId as consumer PID.
     * A full implementation would parse the offer policy too; this is
     * deferred until the ODRL module is available. */
    cJSON *msg = dsp_json_parse(body);
    if (!msg) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid JSON");
        return ESP_FAIL;
    }

    char cpid[DSP_NEG_PID_LEN] = "";
    dsp_json_get_string(msg, DSP_JSONLD_FIELD_PROCESS_ID, cpid, sizeof(cpid));
    dsp_json_delete(msg);

    if (cpid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "missing dspace:processId");
        return ESP_FAIL;
    }

    int idx = dsp_neg_create(cpid, CONFIG_DSP_PROVIDER_ID);
    if (idx < 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "negotiation table full");
        return ESP_FAIL;
    }

    dsp_neg_apply(idx, DSP_NEG_EVENT_OFFER);

    char resp[NEG_BUF_SIZE];
    dsp_build_negotiation_event(resp, sizeof(resp),
                                 cpid, DSP_JSONLD_NEG_STATE_OFFERED);

    httpd_resp_set_status(req, "201 Created");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    ESP_LOGI(TAG, "offer accepted: slot=%d cpid=%s", idx, cpid);
    return ESP_OK;
}

/**
 * POST /negotiations/terminate
 *
 * Receives a ContractNegotiationTerminationMessage.  Applies TERMINATE to
 * the named negotiation (any active state → TERMINATED).
 * Only compiled when CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE == 1.
 */
#if CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE
static esp_err_t terminate_post_handler(httpd_req_t *req)
{
    char body[NEG_BUF_SIZE];
    int  recv_len = httpd_req_recv(req, body, sizeof(body) - 1u);
    if (recv_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
        return ESP_FAIL;
    }
    body[recv_len] = '\0';

    cJSON *msg = dsp_json_parse(body);
    if (!msg) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid JSON");
        return ESP_FAIL;
    }

    char cpid[DSP_NEG_PID_LEN] = "";
    dsp_json_get_string(msg, DSP_JSONLD_FIELD_PROCESS_ID, cpid, sizeof(cpid));
    dsp_json_delete(msg);

    if (cpid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing dspace:processId");
        return ESP_FAIL;
    }

    int idx = dsp_neg_find_by_cpid(cpid);
    if (idx < 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "negotiation not found");
        return ESP_FAIL;
    }

    dsp_neg_apply(idx, DSP_NEG_EVENT_TERMINATE);

    char resp[NEG_BUF_SIZE];
    dsp_build_negotiation_event(resp, sizeof(resp), cpid, DSP_JSONLD_NEG_STATE_TERMINATED);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    ESP_LOGI(TAG, "negotiation terminated: slot=%d cpid=%s", idx, cpid);
    return ESP_OK;
}
#endif /* CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE */

/**
 * POST /negotiations/{id}/agree
 *
 * Receives a ContractAgreementMessage.  Transitions OFFERED → AGREED and
 * responds with a ContractNegotiationEventMessage carrying state AGREED.
 */
static esp_err_t agree_post_handler(httpd_req_t *req)
{
    char id[DSP_NEG_PID_LEN];
    extract_neg_id(req->uri, id, sizeof(id));

    if (id[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing negotiation id");
        return ESP_FAIL;
    }

    int idx = dsp_neg_find_by_cpid(id);
    if (idx < 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "negotiation not found");
        return ESP_FAIL;
    }

    dsp_neg_state_t new_state = dsp_neg_apply(idx, DSP_NEG_EVENT_AGREE);
    if (new_state != DSP_NEG_STATE_AGREED) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid state for agree");
        return ESP_FAIL;
    }

    char resp[NEG_BUF_SIZE];
    dsp_build_negotiation_event(resp, sizeof(resp), id, DSP_JSONLD_NEG_STATE_AGREED);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    ESP_LOGI(TAG, "agreement accepted: slot=%d cpid=%s", idx, id);
    return ESP_OK;
}

/**
 * GET /negotiations/{id}
 *
 * Returns the current negotiation state as a ContractNegotiationEventMessage.
 */
static esp_err_t neg_get_handler(httpd_req_t *req)
{
    char id[DSP_NEG_PID_LEN];
    extract_neg_id(req->uri, id, sizeof(id));

    if (id[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing negotiation id");
        return ESP_FAIL;
    }

    int idx = dsp_neg_find_by_cpid(id);
    if (idx < 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "negotiation not found");
        return ESP_FAIL;
    }

    const char *state_str;
    switch (dsp_neg_get_state(idx)) {
        case DSP_NEG_STATE_REQUESTED:  state_str = DSP_JSONLD_NEG_STATE_REQUESTED;  break;
        case DSP_NEG_STATE_OFFERED:    state_str = DSP_JSONLD_NEG_STATE_OFFERED;    break;
        case DSP_NEG_STATE_AGREED:     state_str = DSP_JSONLD_NEG_STATE_AGREED;     break;
        case DSP_NEG_STATE_VERIFIED:   state_str = DSP_JSONLD_NEG_STATE_VERIFIED;   break;
        case DSP_NEG_STATE_FINALIZED:  state_str = DSP_JSONLD_NEG_STATE_FINALIZED;  break;
        case DSP_NEG_STATE_TERMINATED: state_str = DSP_JSONLD_NEG_STATE_TERMINATED; break;
        default:                       state_str = DSP_JSONLD_NEG_STATE_REQUESTED;  break;
    }

    char resp[NEG_BUF_SIZE];
    dsp_build_negotiation_event(resp, sizeof(resp), id, state_str);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

esp_err_t dsp_neg_register_handlers(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;

    err = dsp_http_register_handler("/negotiations/offers",
                                     DSP_HTTP_POST,
                                     offers_post_handler);
    if (err != ESP_OK) { return err; }

    err = dsp_http_register_handler("/negotiations/*/agree",
                                     DSP_HTTP_POST,
                                     agree_post_handler);
    if (err != ESP_OK) { return err; }

#if CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE
    err = dsp_http_register_handler("/negotiations/terminate",
                                     DSP_HTTP_POST,
                                     terminate_post_handler);
    if (err != ESP_OK) { return err; }
#endif

    err = dsp_http_register_handler("/negotiations/*",
                                     DSP_HTTP_GET,
                                     neg_get_handler);
    return err;
}

#else /* host build */

static esp_err_t neg_offers_host_stub(void *req)
{
    (void)req;
    return ESP_OK;
}

static esp_err_t neg_agree_host_stub(void *req)
{
    (void)req;
    return ESP_OK;
}

static esp_err_t neg_get_host_stub(void *req)
{
    (void)req;
    return ESP_OK;
}

#if CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE
static esp_err_t neg_terminate_host_stub(void *req)
{
    (void)req;
    return ESP_OK;
}
#endif

esp_err_t dsp_neg_register_handlers(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;

    err = dsp_http_register_handler("/negotiations/offers",
                                     DSP_HTTP_POST,
                                     neg_offers_host_stub);
    if (err != ESP_OK) { return err; }

    err = dsp_http_register_handler("/negotiations/*/agree",
                                     DSP_HTTP_POST,
                                     neg_agree_host_stub);
    if (err != ESP_OK) { return err; }

#if CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE
    err = dsp_http_register_handler("/negotiations/terminate",
                                     DSP_HTTP_POST,
                                     neg_terminate_host_stub);
    if (err != ESP_OK) { return err; }
#endif

    err = dsp_http_register_handler("/negotiations/*",
                                     DSP_HTTP_GET,
                                     neg_get_host_stub);
    return err;
}

#endif /* ESP_PLATFORM */
