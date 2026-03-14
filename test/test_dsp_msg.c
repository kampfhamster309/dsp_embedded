/**
 * @file test_dsp_msg.c
 * @brief Host-native Unity tests for dsp_msg validators (DSP-303).
 *
 * 29 tests covering all four validators:
 *   - dsp_msg_validate_catalog_request  (6 tests)
 *   - dsp_msg_validate_offer            (8 tests)
 *   - dsp_msg_validate_agreement        (8 tests)
 *   - dsp_msg_validate_transfer_request (8 tests)
 *   - Error code structural checks       (1 test)
 *   - Cross-type rejection               (1 test)
 */

#include "unity.h"
#include "dsp_msg.h"
#include "dsp_jsonld_ctx.h"

/* -------------------------------------------------------------------------
 * JSON fixtures
 * ------------------------------------------------------------------------- */

/* --- CatalogRequestMessage --- */
static const char *s_cat_req_valid =
    "{"
    "\"@context\":\"" "https://w3id.org/dspace/2024/1/context.json" "\","
    "\"@type\":\"dspace:CatalogRequestMessage\""
    "}";

static const char *s_cat_req_no_context =
    "{\"@type\":\"dspace:CatalogRequestMessage\"}";

static const char *s_cat_req_wrong_type =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:SomethingElse\""
    "}";

/* --- ContractOfferMessage --- */
static const char *s_offer_valid =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractOfferMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\","
    "\"dspace:offer\":{\"@type\":\"odrl:Offer\",\"@id\":\"urn:uuid:offer-1\"}"
    "}";

static const char *s_offer_no_process_id =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractOfferMessage\","
    "\"dspace:offer\":{\"@type\":\"odrl:Offer\"}"
    "}";

static const char *s_offer_no_offer_obj =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractOfferMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\""
    "}";

static const char *s_offer_offer_is_string =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractOfferMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\","
    "\"dspace:offer\":\"not-an-object\""
    "}";

/* --- ContractAgreementMessage --- */
static const char *s_agree_valid =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractAgreementMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\","
    "\"dspace:agreement\":{\"@type\":\"odrl:Agreement\",\"@id\":\"urn:uuid:agr-1\"}"
    "}";

static const char *s_agree_no_process_id =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractAgreementMessage\","
    "\"dspace:agreement\":{\"@type\":\"odrl:Agreement\"}"
    "}";

static const char *s_agree_no_agreement_obj =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractAgreementMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\""
    "}";

static const char *s_agree_agreement_is_string =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:ContractAgreementMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\","
    "\"dspace:agreement\":\"not-an-object\""
    "}";

/* --- TransferRequestMessage --- */
static const char *s_xfer_req_valid =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:TransferRequestMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\","
    "\"dspace:dataset\":\"urn:uuid:dataset-42\""
    "}";

static const char *s_xfer_req_no_process_id =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:TransferRequestMessage\","
    "\"dspace:dataset\":\"urn:uuid:dataset-42\""
    "}";

static const char *s_xfer_req_no_dataset =
    "{"
    "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
    "\"@type\":\"dspace:TransferRequestMessage\","
    "\"dspace:processId\":\"urn:uuid:a1b2c3d4\""
    "}";

/* -------------------------------------------------------------------------
 * 1. Error code structural checks
 * ------------------------------------------------------------------------- */

void test_msg_error_codes_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(DSP_MSG_OK,              DSP_MSG_ERR_NULL_INPUT);
    TEST_ASSERT_NOT_EQUAL(DSP_MSG_ERR_NULL_INPUT,  DSP_MSG_ERR_PARSE);
    TEST_ASSERT_NOT_EQUAL(DSP_MSG_ERR_PARSE,       DSP_MSG_ERR_MISSING_CONTEXT);
    TEST_ASSERT_NOT_EQUAL(DSP_MSG_ERR_MISSING_CONTEXT, DSP_MSG_ERR_WRONG_TYPE);
    TEST_ASSERT_NOT_EQUAL(DSP_MSG_ERR_WRONG_TYPE,  DSP_MSG_ERR_MISSING_FIELD);
}

/* -------------------------------------------------------------------------
 * 2. CatalogRequestMessage
 * ------------------------------------------------------------------------- */

void test_msg_cat_req_null_returns_null_input(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_NULL_INPUT,
                      dsp_msg_validate_catalog_request(NULL));
}

void test_msg_cat_req_bad_json_returns_parse_error(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_PARSE,
                      dsp_msg_validate_catalog_request("{bad json"));
}

void test_msg_cat_req_missing_context_returns_error(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_CONTEXT,
                      dsp_msg_validate_catalog_request(s_cat_req_no_context));
}

void test_msg_cat_req_wrong_type_returns_error(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_WRONG_TYPE,
                      dsp_msg_validate_catalog_request(s_cat_req_wrong_type));
}

void test_msg_cat_req_valid_returns_ok(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_OK,
                      dsp_msg_validate_catalog_request(s_cat_req_valid));
}

void test_msg_cat_req_with_optional_filter_returns_ok(void)
{
    /* dspace:filter is optional — validator must not reject it */
    static const char *json =
        "{"
        "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
        "\"@type\":\"dspace:CatalogRequestMessage\","
        "\"dspace:filter\":[]"
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_OK,
                      dsp_msg_validate_catalog_request(json));
}

/* -------------------------------------------------------------------------
 * 3. ContractOfferMessage
 * ------------------------------------------------------------------------- */

void test_msg_offer_null_returns_null_input(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_NULL_INPUT,
                      dsp_msg_validate_offer(NULL));
}

void test_msg_offer_bad_json_returns_parse_error(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_PARSE,
                      dsp_msg_validate_offer("{bad"));
}

void test_msg_offer_missing_context_returns_error(void)
{
    static const char *json =
        "{"
        "\"@type\":\"dspace:ContractOfferMessage\","
        "\"dspace:processId\":\"urn:uuid:a1\","
        "\"dspace:offer\":{}"
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_CONTEXT,
                      dsp_msg_validate_offer(json));
}

void test_msg_offer_wrong_type_returns_error(void)
{
    static const char *json =
        "{"
        "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
        "\"@type\":\"dspace:CatalogRequestMessage\","
        "\"dspace:processId\":\"urn:uuid:a1\","
        "\"dspace:offer\":{}"
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_WRONG_TYPE,
                      dsp_msg_validate_offer(json));
}

void test_msg_offer_missing_process_id_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_offer(s_offer_no_process_id));
}

void test_msg_offer_missing_offer_obj_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_offer(s_offer_no_offer_obj));
}

void test_msg_offer_offer_field_is_string_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_offer(s_offer_offer_is_string));
}

void test_msg_offer_valid_returns_ok(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_OK,
                      dsp_msg_validate_offer(s_offer_valid));
}

/* -------------------------------------------------------------------------
 * 4. ContractAgreementMessage
 * ------------------------------------------------------------------------- */

void test_msg_agreement_null_returns_null_input(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_NULL_INPUT,
                      dsp_msg_validate_agreement(NULL));
}

void test_msg_agreement_bad_json_returns_parse_error(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_PARSE,
                      dsp_msg_validate_agreement("{bad"));
}

void test_msg_agreement_missing_context_returns_error(void)
{
    static const char *json =
        "{"
        "\"@type\":\"dspace:ContractAgreementMessage\","
        "\"dspace:processId\":\"urn:uuid:a1\","
        "\"dspace:agreement\":{}"
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_CONTEXT,
                      dsp_msg_validate_agreement(json));
}

void test_msg_agreement_wrong_type_returns_error(void)
{
    static const char *json =
        "{"
        "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
        "\"@type\":\"dspace:ContractOfferMessage\","
        "\"dspace:processId\":\"urn:uuid:a1\","
        "\"dspace:agreement\":{}"
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_WRONG_TYPE,
                      dsp_msg_validate_agreement(json));
}

void test_msg_agreement_missing_process_id_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_agreement(s_agree_no_process_id));
}

void test_msg_agreement_missing_agreement_obj_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_agreement(s_agree_no_agreement_obj));
}

void test_msg_agreement_agreement_field_is_string_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_agreement(s_agree_agreement_is_string));
}

void test_msg_agreement_valid_returns_ok(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_OK,
                      dsp_msg_validate_agreement(s_agree_valid));
}

/* -------------------------------------------------------------------------
 * 5. TransferRequestMessage
 * ------------------------------------------------------------------------- */

void test_msg_xfer_null_returns_null_input(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_NULL_INPUT,
                      dsp_msg_validate_transfer_request(NULL));
}

void test_msg_xfer_bad_json_returns_parse_error(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_PARSE,
                      dsp_msg_validate_transfer_request("{bad"));
}

void test_msg_xfer_missing_context_returns_error(void)
{
    static const char *json =
        "{"
        "\"@type\":\"dspace:TransferRequestMessage\","
        "\"dspace:processId\":\"urn:uuid:a1\","
        "\"dspace:dataset\":\"urn:uuid:ds-1\""
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_CONTEXT,
                      dsp_msg_validate_transfer_request(json));
}

void test_msg_xfer_wrong_type_returns_error(void)
{
    static const char *json =
        "{"
        "\"@context\":\"https://w3id.org/dspace/2024/1/context.json\","
        "\"@type\":\"dspace:CatalogRequestMessage\","
        "\"dspace:processId\":\"urn:uuid:a1\","
        "\"dspace:dataset\":\"urn:uuid:ds-1\""
        "}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_WRONG_TYPE,
                      dsp_msg_validate_transfer_request(json));
}

void test_msg_xfer_missing_process_id_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_transfer_request(s_xfer_req_no_process_id));
}

void test_msg_xfer_missing_dataset_returns_missing_field(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_FIELD,
                      dsp_msg_validate_transfer_request(s_xfer_req_no_dataset));
}

void test_msg_xfer_valid_returns_ok(void)
{
    TEST_ASSERT_EQUAL(DSP_MSG_OK,
                      dsp_msg_validate_transfer_request(s_xfer_req_valid));
}

/* -------------------------------------------------------------------------
 * 6. Cross-type rejection
 * ------------------------------------------------------------------------- */

void test_msg_catalog_request_rejected_by_offer_validator(void)
{
    /* A valid CatalogRequestMessage must fail offer validation (wrong @type). */
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_WRONG_TYPE,
                      dsp_msg_validate_offer(s_cat_req_valid));
}

/* -------------------------------------------------------------------------
 * DSP-801: coverage addition – empty-string @context (Phase 2 check)
 * ------------------------------------------------------------------------- */

void test_msg_common_context_empty_string_returns_missing_context(void)
{
    /* @context present as "" passes cJSON_IsString() (Phase 1) but
     * fails the valuestring[0] == '\0' guard (Phase 2) → MISSING_CONTEXT */
    const char *body =
        "{\"@context\":\"\",\"@type\":\"dspace:CatalogRequestMessage\"}";
    TEST_ASSERT_EQUAL(DSP_MSG_ERR_MISSING_CONTEXT,
                      dsp_msg_validate_catalog_request(body));
}
