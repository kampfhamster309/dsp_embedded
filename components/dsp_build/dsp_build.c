/**
 * @file dsp_build.c
 * @brief DSP provider message builder implementation.
 */

#include "dsp_build.h"
#include "dsp_json.h"
#include "dsp_jsonld_ctx.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal helper
 * ------------------------------------------------------------------------- */

/**
 * Stamp the two mandatory JSON-LD fields (@context, @type) onto @p obj.
 * Returns false on allocation failure.
 */
static bool stamp_common(cJSON *obj, const char *type)
{
    return dsp_json_add_string(obj, DSP_JSONLD_FIELD_CONTEXT,
                                DSP_JSONLD_CONTEXT_URL) &&
           dsp_json_add_string(obj, DSP_JSONLD_FIELD_TYPE, type);
}

/**
 * Serialise @p obj into @p out/@p cap.  Deletes @p obj before returning.
 * Returns DSP_BUILD_OK or DSP_BUILD_ERR_BUF_TOO_SMALL.
 */
static dsp_build_err_t finalise(cJSON *obj, char *out, size_t cap)
{
    bool ok = dsp_json_print(obj, out, cap);
    dsp_json_delete(obj);
    return ok ? DSP_BUILD_OK : DSP_BUILD_ERR_BUF_TOO_SMALL;
}

/* -------------------------------------------------------------------------
 * Catalog
 * ------------------------------------------------------------------------- */

dsp_build_err_t dsp_build_catalog(char *out, size_t cap,
                                   const char *dataset_id,
                                   const char *title)
{
    if (!out || !dataset_id || !title) {
        return DSP_BUILD_ERR_NULL_ARG;
    }

    cJSON *obj = dsp_json_new_object();
    if (!obj) {
        return DSP_BUILD_ERR_ALLOC;
    }

    /* Build dataset entry */
    cJSON *ds = dsp_json_new_object();
    if (!ds) {
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }
    if (!dsp_json_add_string(ds, DSP_JSONLD_FIELD_TYPE,  "dcat:Dataset") ||
        !dsp_json_add_string(ds, DSP_JSONLD_FIELD_ID,    dataset_id)) {
        dsp_json_delete(ds);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    cJSON *arr = cJSON_CreateArray();
    if (!arr || !cJSON_AddItemToArray(arr, ds)) {
        if (!arr) {
            dsp_json_delete(ds);
        }
        /* ds is owned by arr if AddItemToArray partially succeeded */
        dsp_json_delete(arr);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    if (!stamp_common(obj, DSP_JSONLD_TYPE_CATALOG) ||
        !dsp_json_add_string(obj, "dct:title", title) ||
        !cJSON_AddItemToObject(obj, "dcat:dataset", arr)) {
        dsp_json_delete(arr);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    return finalise(obj, out, cap);
}

/* -------------------------------------------------------------------------
 * Contract negotiation
 * ------------------------------------------------------------------------- */

dsp_build_err_t dsp_build_agreement_msg(char *out, size_t cap,
                                         const char *process_id,
                                         const char *agreement_id)
{
    if (!out || !process_id || !agreement_id) {
        return DSP_BUILD_ERR_NULL_ARG;
    }

    cJSON *obj = dsp_json_new_object();
    if (!obj) {
        return DSP_BUILD_ERR_ALLOC;
    }

    cJSON *agreement = dsp_json_new_object();
    if (!agreement) {
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }
    if (!dsp_json_add_string(agreement, DSP_JSONLD_FIELD_TYPE, "odrl:Agreement") ||
        !dsp_json_add_string(agreement, DSP_JSONLD_FIELD_ID,   agreement_id)) {
        dsp_json_delete(agreement);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    if (!stamp_common(obj, DSP_JSONLD_TYPE_CONTRACT_AGREEMENT) ||
        !dsp_json_add_string(obj, DSP_JSONLD_FIELD_PROCESS_ID, process_id) ||
        !cJSON_AddItemToObject(obj, DSP_JSONLD_FIELD_AGREEMENT, agreement)) {
        dsp_json_delete(agreement);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    return finalise(obj, out, cap);
}

dsp_build_err_t dsp_build_negotiation_event(char *out, size_t cap,
                                             const char *process_id,
                                             const char *state)
{
    if (!out || !process_id || !state) {
        return DSP_BUILD_ERR_NULL_ARG;
    }

    cJSON *obj = dsp_json_new_object();
    if (!obj) {
        return DSP_BUILD_ERR_ALLOC;
    }

    if (!stamp_common(obj, DSP_JSONLD_TYPE_NEGOTIATION_EVENT) ||
        !dsp_json_add_string(obj, DSP_JSONLD_FIELD_PROCESS_ID, process_id) ||
        !dsp_json_add_string(obj, "dspace:eventType",           state)) {
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    return finalise(obj, out, cap);
}

/* -------------------------------------------------------------------------
 * Transfer
 * ------------------------------------------------------------------------- */

dsp_build_err_t dsp_build_transfer_start(char *out, size_t cap,
                                          const char *process_id,
                                          const char *endpoint_url)
{
    if (!out || !process_id || !endpoint_url) {
        return DSP_BUILD_ERR_NULL_ARG;
    }

    cJSON *obj = dsp_json_new_object();
    if (!obj) {
        return DSP_BUILD_ERR_ALLOC;
    }

    cJSON *addr = dsp_json_new_object();
    if (!addr) {
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }
    if (!dsp_json_add_string(addr, "dspace:endpointUrl", endpoint_url)) {
        dsp_json_delete(addr);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    if (!stamp_common(obj, DSP_JSONLD_TYPE_TRANSFER_START) ||
        !dsp_json_add_string(obj, DSP_JSONLD_FIELD_PROCESS_ID, process_id) ||
        !cJSON_AddItemToObject(obj, DSP_JSONLD_FIELD_DATA_ADDRESS, addr)) {
        dsp_json_delete(addr);
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    return finalise(obj, out, cap);
}

dsp_build_err_t dsp_build_transfer_completion(char *out, size_t cap,
                                               const char *process_id)
{
    if (!out || !process_id) {
        return DSP_BUILD_ERR_NULL_ARG;
    }

    cJSON *obj = dsp_json_new_object();
    if (!obj) {
        return DSP_BUILD_ERR_ALLOC;
    }

    if (!stamp_common(obj, DSP_JSONLD_TYPE_TRANSFER_COMPLETION) ||
        !dsp_json_add_string(obj, DSP_JSONLD_FIELD_PROCESS_ID, process_id)) {
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    return finalise(obj, out, cap);
}

/* -------------------------------------------------------------------------
 * Error
 * ------------------------------------------------------------------------- */

dsp_build_err_t dsp_build_error(char *out, size_t cap,
                                 const char *code,
                                 const char *reason)
{
    if (!out || !code || !reason) {
        return DSP_BUILD_ERR_NULL_ARG;
    }

    cJSON *obj = dsp_json_new_object();
    if (!obj) {
        return DSP_BUILD_ERR_ALLOC;
    }

    if (!stamp_common(obj, DSP_JSONLD_TYPE_ERROR) ||
        !dsp_json_add_string(obj, DSP_JSONLD_FIELD_CODE,   code) ||
        !dsp_json_add_string(obj, DSP_JSONLD_FIELD_REASON, reason)) {
        dsp_json_delete(obj);
        return DSP_BUILD_ERR_ALLOC;
    }

    return finalise(obj, out, cap);
}
