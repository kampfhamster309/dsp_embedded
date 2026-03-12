/**
 * @file test_dsp_build.c
 * @brief Host-native Unity tests for dsp_build message builders (DSP-304).
 *
 * 36 tests covering all six builders:
 *   - dsp_build_catalog              (6 tests)
 *   - dsp_build_agreement_msg        (6 tests)
 *   - dsp_build_negotiation_event    (6 tests)
 *   - dsp_build_transfer_start       (6 tests)
 *   - dsp_build_transfer_completion  (5 tests)
 *   - dsp_build_error                (6 tests)
 *   - Error code structural checks    (1 test)
 *
 * Output correctness is verified by round-tripping through dsp_json_parse()
 * and inspecting individual fields — more robust than simple string search.
 */

#include "unity.h"
#include "dsp_build.h"
#include "dsp_json.h"
#include "dsp_jsonld_ctx.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

#define OUT_CAP 512u
#define TINY    8u

/** Parse @p json, get string field @p key into static 128-byte buffer. */
static const char *field(const char *json, const char *key)
{
    static char buf[128];
    cJSON *obj = dsp_json_parse(json);
    if (!obj) {
        return NULL;
    }
    bool ok = dsp_json_get_string(obj, key, buf, sizeof(buf));
    dsp_json_delete(obj);
    return ok ? buf : NULL;
}

/** Parse @p json and return true if @p key names a JSON object. */
static bool field_is_object(const char *json, const char *key)
{
    cJSON *obj = dsp_json_parse(json);
    if (!obj) {
        return false;
    }
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    bool result = cJSON_IsObject(item);
    dsp_json_delete(obj);
    return result;
}

/** Parse @p json and return true if @p key names a JSON array. */
static bool field_is_array(const char *json, const char *key)
{
    cJSON *obj = dsp_json_parse(json);
    if (!obj) {
        return false;
    }
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    bool result = cJSON_IsArray(item);
    dsp_json_delete(obj);
    return result;
}

/* -------------------------------------------------------------------------
 * 0. Error code structural checks
 * ------------------------------------------------------------------------- */

void test_build_error_codes_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(DSP_BUILD_OK,           DSP_BUILD_ERR_NULL_ARG);
    TEST_ASSERT_NOT_EQUAL(DSP_BUILD_ERR_NULL_ARG, DSP_BUILD_ERR_BUF_TOO_SMALL);
    TEST_ASSERT_NOT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL, DSP_BUILD_ERR_ALLOC);
}

/* -------------------------------------------------------------------------
 * 1. dsp_build_catalog
 * ------------------------------------------------------------------------- */

void test_build_catalog_null_out_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_catalog(NULL, OUT_CAP, "urn:ds:1", "Test"));
}

void test_build_catalog_null_dataset_id_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_catalog(out, OUT_CAP, NULL, "Test"));
}

void test_build_catalog_null_title_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_catalog(out, OUT_CAP, "urn:ds:1", NULL));
}

void test_build_catalog_buf_too_small_returns_error(void)
{
    char out[TINY];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_build_catalog(out, sizeof(out), "urn:ds:1", "T"));
}

void test_build_catalog_valid_returns_ok(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_build_catalog(out, sizeof(out),
                                        "urn:uuid:dataset-1", "Sensor Catalog"));
}

void test_build_catalog_output_fields(void)
{
    char out[OUT_CAP];
    dsp_build_catalog(out, sizeof(out), "urn:uuid:dataset-1", "Sensor Catalog");

    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             field(out, DSP_JSONLD_FIELD_CONTEXT));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_CATALOG,
                             field(out, DSP_JSONLD_FIELD_TYPE));
    TEST_ASSERT_EQUAL_STRING("Sensor Catalog", field(out, "dct:title"));
    /* dcat:dataset must be an array */
    TEST_ASSERT_TRUE(field_is_array(out, "dcat:dataset"));
}

/* -------------------------------------------------------------------------
 * 2. dsp_build_agreement_msg
 * ------------------------------------------------------------------------- */

void test_build_agreement_null_out_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_agreement_msg(NULL, OUT_CAP,
                                              "pid", "aid"));
}

void test_build_agreement_null_process_id_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_agreement_msg(out, OUT_CAP, NULL, "aid"));
}

void test_build_agreement_null_agreement_id_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_agreement_msg(out, OUT_CAP, "pid", NULL));
}

void test_build_agreement_buf_too_small_returns_error(void)
{
    char out[TINY];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_build_agreement_msg(out, sizeof(out), "p", "a"));
}

void test_build_agreement_valid_returns_ok(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_build_agreement_msg(out, sizeof(out),
                                              "urn:uuid:proc-1",
                                              "urn:uuid:agr-1"));
}

void test_build_agreement_output_fields(void)
{
    char out[OUT_CAP];
    dsp_build_agreement_msg(out, sizeof(out),
                             "urn:uuid:proc-1", "urn:uuid:agr-1");

    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             field(out, DSP_JSONLD_FIELD_CONTEXT));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_CONTRACT_AGREEMENT,
                             field(out, DSP_JSONLD_FIELD_TYPE));
    TEST_ASSERT_EQUAL_STRING("urn:uuid:proc-1",
                             field(out, DSP_JSONLD_FIELD_PROCESS_ID));
    TEST_ASSERT_TRUE(field_is_object(out, DSP_JSONLD_FIELD_AGREEMENT));
}

/* -------------------------------------------------------------------------
 * 3. dsp_build_negotiation_event
 * ------------------------------------------------------------------------- */

void test_build_neg_event_null_out_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_negotiation_event(NULL, OUT_CAP,
                                                   "pid",
                                                   DSP_JSONLD_NEG_STATE_FINALIZED));
}

void test_build_neg_event_null_process_id_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_negotiation_event(out, OUT_CAP,
                                                   NULL,
                                                   DSP_JSONLD_NEG_STATE_FINALIZED));
}

void test_build_neg_event_null_state_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_negotiation_event(out, OUT_CAP, "pid", NULL));
}

void test_build_neg_event_buf_too_small_returns_error(void)
{
    char out[TINY];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_build_negotiation_event(out, sizeof(out),
                                                   "p", "s"));
}

void test_build_neg_event_valid_returns_ok(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_build_negotiation_event(out, sizeof(out),
                                                   "urn:uuid:proc-1",
                                                   DSP_JSONLD_NEG_STATE_FINALIZED));
}

void test_build_neg_event_output_fields(void)
{
    char out[OUT_CAP];
    dsp_build_negotiation_event(out, sizeof(out),
                                 "urn:uuid:proc-1",
                                 DSP_JSONLD_NEG_STATE_FINALIZED);

    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             field(out, DSP_JSONLD_FIELD_CONTEXT));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_NEGOTIATION_EVENT,
                             field(out, DSP_JSONLD_FIELD_TYPE));
    TEST_ASSERT_EQUAL_STRING("urn:uuid:proc-1",
                             field(out, DSP_JSONLD_FIELD_PROCESS_ID));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_NEG_STATE_FINALIZED,
                             field(out, "dspace:eventType"));
}

/* -------------------------------------------------------------------------
 * 4. dsp_build_transfer_start
 * ------------------------------------------------------------------------- */

void test_build_xfer_start_null_out_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_transfer_start(NULL, OUT_CAP,
                                               "pid", "http://localhost/data"));
}

void test_build_xfer_start_null_process_id_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_transfer_start(out, OUT_CAP,
                                               NULL, "http://localhost/data"));
}

void test_build_xfer_start_null_endpoint_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_transfer_start(out, OUT_CAP, "pid", NULL));
}

void test_build_xfer_start_buf_too_small_returns_error(void)
{
    char out[TINY];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_build_transfer_start(out, sizeof(out), "p", "u"));
}

void test_build_xfer_start_valid_returns_ok(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_build_transfer_start(out, sizeof(out),
                                               "urn:uuid:xfer-1",
                                               "https://sensor.local/data"));
}

void test_build_xfer_start_output_fields(void)
{
    char out[OUT_CAP];
    dsp_build_transfer_start(out, sizeof(out),
                              "urn:uuid:xfer-1", "https://sensor.local/data");

    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             field(out, DSP_JSONLD_FIELD_CONTEXT));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_TRANSFER_START,
                             field(out, DSP_JSONLD_FIELD_TYPE));
    TEST_ASSERT_EQUAL_STRING("urn:uuid:xfer-1",
                             field(out, DSP_JSONLD_FIELD_PROCESS_ID));
    TEST_ASSERT_TRUE(field_is_object(out, DSP_JSONLD_FIELD_DATA_ADDRESS));
}

/* -------------------------------------------------------------------------
 * 5. dsp_build_transfer_completion
 * ------------------------------------------------------------------------- */

void test_build_xfer_completion_null_out_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_transfer_completion(NULL, OUT_CAP, "pid"));
}

void test_build_xfer_completion_null_process_id_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_transfer_completion(out, OUT_CAP, NULL));
}

void test_build_xfer_completion_buf_too_small_returns_error(void)
{
    char out[TINY];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_build_transfer_completion(out, sizeof(out), "p"));
}

void test_build_xfer_completion_valid_returns_ok(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_build_transfer_completion(out, sizeof(out),
                                                    "urn:uuid:xfer-1"));
}

void test_build_xfer_completion_output_fields(void)
{
    char out[OUT_CAP];
    dsp_build_transfer_completion(out, sizeof(out), "urn:uuid:xfer-1");

    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             field(out, DSP_JSONLD_FIELD_CONTEXT));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_TRANSFER_COMPLETION,
                             field(out, DSP_JSONLD_FIELD_TYPE));
    TEST_ASSERT_EQUAL_STRING("urn:uuid:xfer-1",
                             field(out, DSP_JSONLD_FIELD_PROCESS_ID));
}

/* -------------------------------------------------------------------------
 * 6. dsp_build_error
 * ------------------------------------------------------------------------- */

void test_build_error_null_out_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_error(NULL, OUT_CAP, "400", "Bad Request"));
}

void test_build_error_null_code_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_error(out, OUT_CAP, NULL, "Bad Request"));
}

void test_build_error_null_reason_returns_null_arg(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_build_error(out, OUT_CAP, "400", NULL));
}

void test_build_error_buf_too_small_returns_error(void)
{
    char out[TINY];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_build_error(out, sizeof(out), "400", "Bad"));
}

void test_build_error_valid_returns_ok(void)
{
    char out[OUT_CAP];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_build_error(out, sizeof(out), "403", "Forbidden"));
}

void test_build_error_output_fields(void)
{
    char out[OUT_CAP];
    dsp_build_error(out, sizeof(out), "403", "Forbidden");

    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             field(out, DSP_JSONLD_FIELD_CONTEXT));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_ERROR,
                             field(out, DSP_JSONLD_FIELD_TYPE));
    TEST_ASSERT_EQUAL_STRING("403",       field(out, DSP_JSONLD_FIELD_CODE));
    TEST_ASSERT_EQUAL_STRING("Forbidden", field(out, DSP_JSONLD_FIELD_REASON));
}
