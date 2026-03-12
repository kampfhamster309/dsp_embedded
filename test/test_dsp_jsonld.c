/**
 * @file test_dsp_jsonld.c
 * @brief Host-native Unity tests for dsp_jsonld_ctx.h (DSP-302).
 *
 * 26 tests covering:
 *   - Context URL and namespace prefixes
 *   - Mandatory JSON-LD field names
 *   - DSP common field names
 *   - All message type strings (catalog, negotiation, transfer, error)
 *   - Negotiation and transfer state strings
 *   - Structural invariants (prefix consistency, non-empty, non-NULL)
 */

#include "unity.h"
#include "dsp_jsonld_ctx.h"
#include "dsp_json.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

/** True if @p str starts with @p prefix. */
static bool starts_with(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* -------------------------------------------------------------------------
 * 1. Context URL
 * ------------------------------------------------------------------------- */

void test_jsonld_context_url_is_defined(void)
{
    TEST_ASSERT_NOT_NULL(DSP_JSONLD_CONTEXT_URL);
    TEST_ASSERT_GREATER_THAN(0u, strlen(DSP_JSONLD_CONTEXT_URL));
}

void test_jsonld_context_url_contains_dspace_domain(void)
{
    TEST_ASSERT_NOT_NULL(strstr(DSP_JSONLD_CONTEXT_URL, "w3id.org/dspace"));
}

/* -------------------------------------------------------------------------
 * 2. Namespace prefixes
 * ------------------------------------------------------------------------- */

void test_jsonld_prefix_dspace_ends_with_colon(void)
{
    const char *p = DSP_JSONLD_PREFIX_DSPACE;
    TEST_ASSERT_EQUAL(p[strlen(p) - 1u], ':');
}

void test_jsonld_prefix_dcat_ends_with_colon(void)
{
    const char *p = DSP_JSONLD_PREFIX_DCAT;
    TEST_ASSERT_EQUAL(p[strlen(p) - 1u], ':');
}

void test_jsonld_prefix_odrl_ends_with_colon(void)
{
    const char *p = DSP_JSONLD_PREFIX_ODRL;
    TEST_ASSERT_EQUAL(p[strlen(p) - 1u], ':');
}

/* -------------------------------------------------------------------------
 * 3. Mandatory JSON-LD field names
 * ------------------------------------------------------------------------- */

void test_jsonld_field_context_value(void)
{
    TEST_ASSERT_EQUAL_STRING("@context", DSP_JSONLD_FIELD_CONTEXT);
}

void test_jsonld_field_type_value(void)
{
    TEST_ASSERT_EQUAL_STRING("@type", DSP_JSONLD_FIELD_TYPE);
}

void test_jsonld_field_id_value(void)
{
    TEST_ASSERT_EQUAL_STRING("@id", DSP_JSONLD_FIELD_ID);
}

/* -------------------------------------------------------------------------
 * 4. DSP common field names
 * ------------------------------------------------------------------------- */

void test_jsonld_common_fields_have_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_FIELD_PROCESS_ID,   "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_FIELD_CONSUMER_ID,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_FIELD_PROVIDER_ID,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_FIELD_STATE,        "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_FIELD_CODE,         "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_FIELD_REASON,       "dspace:"));
}

/* -------------------------------------------------------------------------
 * 5. Catalog message types
 * ------------------------------------------------------------------------- */

void test_jsonld_type_catalog_request_has_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_CATALOG_REQUEST, "dspace:"));
}

void test_jsonld_type_catalog_has_dcat_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_CATALOG, "dcat:"));
}

/* -------------------------------------------------------------------------
 * 6. Negotiation message types
 * ------------------------------------------------------------------------- */

void test_jsonld_negotiation_types_have_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_CONTRACT_REQUEST,        "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_CONTRACT_OFFER,          "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_CONTRACT_AGREEMENT,      "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_AGREEMENT_VERIFICATION,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_NEGOTIATION_EVENT,       "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_NEGOTIATION_TERMINATION, "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_NEGOTIATION,             "dspace:"));
}

void test_jsonld_negotiation_types_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_TYPE_CONTRACT_REQUEST,
                                    DSP_JSONLD_TYPE_CONTRACT_OFFER));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_TYPE_CONTRACT_OFFER,
                                    DSP_JSONLD_TYPE_CONTRACT_AGREEMENT));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_TYPE_CONTRACT_AGREEMENT,
                                    DSP_JSONLD_TYPE_AGREEMENT_VERIFICATION));
}

/* -------------------------------------------------------------------------
 * 7. Transfer message types
 * ------------------------------------------------------------------------- */

void test_jsonld_transfer_types_have_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_TRANSFER_REQUEST,     "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_TRANSFER_START,       "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_TRANSFER_COMPLETION,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_TRANSFER_TERMINATION, "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_TRANSFER_SUSPENSION,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_TRANSFER_PROCESS,     "dspace:"));
}

void test_jsonld_transfer_types_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_TYPE_TRANSFER_REQUEST,
                                    DSP_JSONLD_TYPE_TRANSFER_START));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_TYPE_TRANSFER_START,
                                    DSP_JSONLD_TYPE_TRANSFER_COMPLETION));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_TYPE_TRANSFER_COMPLETION,
                                    DSP_JSONLD_TYPE_TRANSFER_TERMINATION));
}

/* -------------------------------------------------------------------------
 * 8. Error type
 * ------------------------------------------------------------------------- */

void test_jsonld_type_error_has_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_TYPE_ERROR, "dspace:"));
}

/* -------------------------------------------------------------------------
 * 9. Negotiation state strings
 * ------------------------------------------------------------------------- */

void test_jsonld_neg_states_have_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_NEG_STATE_REQUESTED,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_NEG_STATE_OFFERED,    "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_NEG_STATE_AGREED,     "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_NEG_STATE_VERIFIED,   "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_NEG_STATE_FINALIZED,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_NEG_STATE_TERMINATED, "dspace:"));
}

void test_jsonld_neg_states_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_NEG_STATE_REQUESTED,  DSP_JSONLD_NEG_STATE_OFFERED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_NEG_STATE_OFFERED,    DSP_JSONLD_NEG_STATE_AGREED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_NEG_STATE_AGREED,     DSP_JSONLD_NEG_STATE_VERIFIED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_NEG_STATE_VERIFIED,   DSP_JSONLD_NEG_STATE_FINALIZED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_NEG_STATE_FINALIZED,  DSP_JSONLD_NEG_STATE_TERMINATED));
}

/* -------------------------------------------------------------------------
 * 10. Transfer state strings
 * ------------------------------------------------------------------------- */

void test_jsonld_xfer_states_have_dspace_prefix(void)
{
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_XFER_STATE_REQUESTED,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_XFER_STATE_STARTED,    "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_XFER_STATE_COMPLETED,  "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_XFER_STATE_TERMINATED, "dspace:"));
    TEST_ASSERT_TRUE(starts_with(DSP_JSONLD_XFER_STATE_SUSPENDED,  "dspace:"));
}

void test_jsonld_xfer_states_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_XFER_STATE_REQUESTED,  DSP_JSONLD_XFER_STATE_STARTED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_XFER_STATE_STARTED,    DSP_JSONLD_XFER_STATE_COMPLETED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_XFER_STATE_COMPLETED,  DSP_JSONLD_XFER_STATE_TERMINATED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(DSP_JSONLD_XFER_STATE_TERMINATED, DSP_JSONLD_XFER_STATE_SUSPENDED));
}

/* -------------------------------------------------------------------------
 * 11. Interop: dsp_json + dsp_jsonld_ctx together
 * ------------------------------------------------------------------------- */

void test_jsonld_constants_work_with_dsp_json(void)
{
    /* Verify the constants compile and match what dsp_json_get_string would
     * expect: just a round-trip through dsp_json helpers. */
    cJSON *obj = dsp_json_new_object();
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_TRUE(dsp_json_add_string(obj,
                                         DSP_JSONLD_FIELD_CONTEXT,
                                         DSP_JSONLD_CONTEXT_URL));
    TEST_ASSERT_TRUE(dsp_json_add_string(obj,
                                         DSP_JSONLD_FIELD_TYPE,
                                         DSP_JSONLD_TYPE_CATALOG_REQUEST));
    TEST_ASSERT_TRUE(dsp_json_has_mandatory_fields(obj));

    char buf[128];
    TEST_ASSERT_TRUE(dsp_json_get_type(obj, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_CATALOG_REQUEST, buf);
    dsp_json_delete(obj);
}
