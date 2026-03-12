/**
 * @file dsp_json.c
 * @brief cJSON wrapper with DSP JSON-LD field helpers.
 */

#include "dsp_json.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Parse / cleanup
 * ------------------------------------------------------------------------- */

cJSON *dsp_json_parse(const char *text)
{
    if (!text) {
        return NULL;
    }
    return cJSON_Parse(text);
}

void dsp_json_delete(cJSON *obj)
{
    cJSON_Delete(obj);
}

/* -------------------------------------------------------------------------
 * Field accessors
 * ------------------------------------------------------------------------- */

bool dsp_json_get_string(const cJSON *obj, const char *key,
                          char *buf, size_t cap)
{
    if (!obj || !key || !buf || cap == 0u) {
        if (buf && cap > 0u) {
            buf[0] = '\0';
        }
        return false;
    }

    buf[0] = '\0';

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(item) || !item->valuestring) {
        return false;
    }

    size_t slen = strlen(item->valuestring);
    if (slen >= cap) {
        return false;
    }

    memcpy(buf, item->valuestring, slen + 1u);
    return true;
}

bool dsp_json_get_type(const cJSON *obj, char *buf, size_t cap)
{
    return dsp_json_get_string(obj, DSP_JSON_FIELD_TYPE, buf, cap);
}

bool dsp_json_get_context(const cJSON *obj, char *buf, size_t cap)
{
    return dsp_json_get_string(obj, DSP_JSON_FIELD_CONTEXT, buf, cap);
}

bool dsp_json_has_mandatory_fields(const cJSON *obj)
{
    if (!obj) {
        return false;
    }

    const cJSON *ctx  = cJSON_GetObjectItemCaseSensitive(obj, DSP_JSON_FIELD_CONTEXT);
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(obj, DSP_JSON_FIELD_TYPE);

    return cJSON_IsString(ctx)  && ctx->valuestring  && ctx->valuestring[0]  != '\0' &&
           cJSON_IsString(type) && type->valuestring && type->valuestring[0] != '\0';
}

/* -------------------------------------------------------------------------
 * Builder helpers
 * ------------------------------------------------------------------------- */

cJSON *dsp_json_new_object(void)
{
    return cJSON_CreateObject();
}

bool dsp_json_add_string(cJSON *obj, const char *key, const char *val)
{
    if (!obj || !key || !val) {
        return false;
    }

    /* Remove existing item first to implement replace semantics. */
    cJSON_DeleteItemFromObjectCaseSensitive(obj, key);

    cJSON *item = cJSON_CreateString(val);
    if (!item) {
        return false;
    }

    if (!cJSON_AddItemToObject(obj, key, item)) {
        cJSON_Delete(item);
        return false;
    }

    return true;
}

/* -------------------------------------------------------------------------
 * Serialisation
 * ------------------------------------------------------------------------- */

bool dsp_json_print(const cJSON *obj, char *buf, size_t cap)
{
    if (!obj || !buf || cap == 0u) {
        if (buf && cap > 0u) {
            buf[0] = '\0';
        }
        return false;
    }

    buf[0] = '\0';

    char *tmp = cJSON_PrintUnformatted(obj);
    if (!tmp) {
        return false;
    }

    size_t len = strlen(tmp);
    if (len >= cap) {
        cJSON_free(tmp);
        return false;
    }

    memcpy(buf, tmp, len + 1u);
    cJSON_free(tmp);
    return true;
}

char *dsp_json_print_alloc(const cJSON *obj)
{
    if (!obj) {
        return NULL;
    }
    return cJSON_PrintUnformatted(obj);
}

void dsp_json_free_str(char *str)
{
    cJSON_free(str);
}

/* -------------------------------------------------------------------------
 * Version
 * ------------------------------------------------------------------------- */

const char *dsp_json_cjson_version(void)
{
    return cJSON_Version();
}
