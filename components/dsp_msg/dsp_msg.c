/**
 * @file dsp_msg.c
 * @brief DSP message schema validator implementation.
 */

#include "dsp_msg.h"
#include "dsp_json.h"
#include "dsp_jsonld_ctx.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/**
 * Parse @p json, verify @context is present, verify @type equals
 * @p expected_type, and return the parsed tree in @p *out_obj.
 *
 * The caller must call dsp_json_delete(*out_obj) on success.
 * On failure *out_obj is NULL.
 */
static dsp_msg_err_t check_common(const char    *json,
                                   const char    *expected_type,
                                   cJSON        **out_obj)
{
    *out_obj = NULL;

    if (!json) {
        return DSP_MSG_ERR_NULL_INPUT;
    }

    cJSON *obj = dsp_json_parse(json);
    if (!obj) {
        return DSP_MSG_ERR_PARSE;
    }

    /* @context – must be a non-empty string */
    char ctx_buf[8]; /* just need to know it exists and is non-empty */
    if (!dsp_json_get_context(obj, ctx_buf, sizeof(ctx_buf)) &&
        /* buf too small is also a hit — check the raw item */
        !cJSON_IsString(cJSON_GetObjectItemCaseSensitive(obj,
                                                          DSP_JSONLD_FIELD_CONTEXT))) {
        dsp_json_delete(obj);
        return DSP_MSG_ERR_MISSING_CONTEXT;
    }

    /* Confirm @context item exists and is a string (even if truncated above) */
    const cJSON *ctx_item = cJSON_GetObjectItemCaseSensitive(obj,
                                                              DSP_JSONLD_FIELD_CONTEXT);
    if (!cJSON_IsString(ctx_item) || !ctx_item->valuestring ||
        ctx_item->valuestring[0] == '\0') {
        dsp_json_delete(obj);
        return DSP_MSG_ERR_MISSING_CONTEXT;
    }

    /* @type – must match expected_type */
    const cJSON *type_item = cJSON_GetObjectItemCaseSensitive(obj,
                                                               DSP_JSONLD_FIELD_TYPE);
    if (!cJSON_IsString(type_item) || !type_item->valuestring ||
        strcmp(type_item->valuestring, expected_type) != 0) {
        dsp_json_delete(obj);
        return DSP_MSG_ERR_WRONG_TYPE;
    }

    *out_obj = obj;
    return DSP_MSG_OK;
}

/** Return true if @p obj has a non-empty string field named @p key. */
static bool has_string_field(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsString(item) &&
           item->valuestring != NULL &&
           item->valuestring[0] != '\0';
}

/** Return true if @p obj has an object field named @p key. */
static bool has_object_field(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsObject(item);
}

/* -------------------------------------------------------------------------
 * Validators
 * ------------------------------------------------------------------------- */

dsp_msg_err_t dsp_msg_validate_catalog_request(const char *json)
{
    cJSON *obj = NULL;
    dsp_msg_err_t rc = check_common(json,
                                     DSP_JSONLD_TYPE_CATALOG_REQUEST,
                                     &obj);
    if (rc != DSP_MSG_OK) {
        return rc;
    }
    /* CatalogRequestMessage has no additional mandatory fields beyond
     * @context and @type; dspace:filter is optional. */
    dsp_json_delete(obj);
    return DSP_MSG_OK;
}

dsp_msg_err_t dsp_msg_validate_offer(const char *json)
{
    cJSON *obj = NULL;
    dsp_msg_err_t rc = check_common(json,
                                     DSP_JSONLD_TYPE_CONTRACT_OFFER,
                                     &obj);
    if (rc != DSP_MSG_OK) {
        return rc;
    }

    if (!has_string_field(obj, DSP_JSONLD_FIELD_PROCESS_ID) ||
        !has_object_field(obj, DSP_JSONLD_FIELD_OFFER)) {
        dsp_json_delete(obj);
        return DSP_MSG_ERR_MISSING_FIELD;
    }

    dsp_json_delete(obj);
    return DSP_MSG_OK;
}

dsp_msg_err_t dsp_msg_validate_agreement(const char *json)
{
    cJSON *obj = NULL;
    dsp_msg_err_t rc = check_common(json,
                                     DSP_JSONLD_TYPE_CONTRACT_AGREEMENT,
                                     &obj);
    if (rc != DSP_MSG_OK) {
        return rc;
    }

    if (!has_string_field(obj, DSP_JSONLD_FIELD_PROCESS_ID) ||
        !has_object_field(obj, DSP_JSONLD_FIELD_AGREEMENT)) {
        dsp_json_delete(obj);
        return DSP_MSG_ERR_MISSING_FIELD;
    }

    dsp_json_delete(obj);
    return DSP_MSG_OK;
}

dsp_msg_err_t dsp_msg_validate_transfer_request(const char *json)
{
    cJSON *obj = NULL;
    dsp_msg_err_t rc = check_common(json,
                                     DSP_JSONLD_TYPE_TRANSFER_REQUEST,
                                     &obj);
    if (rc != DSP_MSG_OK) {
        return rc;
    }

    if (!has_string_field(obj, DSP_JSONLD_FIELD_PROCESS_ID) ||
        !has_string_field(obj, DSP_JSONLD_FIELD_DATASET)) {
        dsp_json_delete(obj);
        return DSP_MSG_ERR_MISSING_FIELD;
    }

    dsp_json_delete(obj);
    return DSP_MSG_OK;
}
