/**
 * @file test_dsp_catalog.c
 * @brief Host-native Unity tests for dsp_catalog (DSP-401, DSP-407).
 *
 * 21 tests covering:
 *   - Compile-time config constants
 *   - Lifecycle (init/deinit/is_initialized)
 *   - dsp_catalog_get_json serialization
 *   - dsp_catalog_register_handler
 *   - dsp_catalog_register_request_handler – disabled path (DSP-407)
 */

#include "unity.h"
#include "dsp_catalog.h"
#include "dsp_json.h"
#include "dsp_jsonld_ctx.h"
#include "dsp_config.h"
#include "esp_err.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

/** Parse out[512] buffer and return string field value in static buf. */
static const char *get_field(const char *json, const char *key)
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
 * 1. Compile-time config constants
 * ------------------------------------------------------------------------- */

void test_catalog_dataset_id_is_nonempty(void)
{
    TEST_ASSERT_GREATER_THAN(0u, strlen(CONFIG_DSP_CATALOG_DATASET_ID));
}

void test_catalog_title_is_nonempty(void)
{
    TEST_ASSERT_GREATER_THAN(0u, strlen(CONFIG_DSP_CATALOG_TITLE));
}

void test_catalog_json_buf_size_sufficient(void)
{
    /* DSP_CATALOG_JSON_BUF_SIZE must comfortably hold the serialised catalog */
    TEST_ASSERT_GREATER_OR_EQUAL(256u, DSP_CATALOG_JSON_BUF_SIZE);
}

/* -------------------------------------------------------------------------
 * 2. Lifecycle
 * ------------------------------------------------------------------------- */

void test_catalog_is_initialized_false_before_init(void)
{
    dsp_catalog_deinit(); /* ensure clean state */
    TEST_ASSERT_FALSE(dsp_catalog_is_initialized());
}

void test_catalog_init_returns_ok(void)
{
    dsp_catalog_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_catalog_init());
    dsp_catalog_deinit();
}

void test_catalog_is_initialized_true_after_init(void)
{
    dsp_catalog_deinit();
    dsp_catalog_init();
    TEST_ASSERT_TRUE(dsp_catalog_is_initialized());
    dsp_catalog_deinit();
}

void test_catalog_is_initialized_false_after_deinit(void)
{
    dsp_catalog_init();
    dsp_catalog_deinit();
    TEST_ASSERT_FALSE(dsp_catalog_is_initialized());
}

void test_catalog_double_init_safe(void)
{
    dsp_catalog_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_catalog_init());
    TEST_ASSERT_EQUAL(ESP_OK, dsp_catalog_init());
    TEST_ASSERT_TRUE(dsp_catalog_is_initialized());
    dsp_catalog_deinit();
}

void test_catalog_double_deinit_safe(void)
{
    dsp_catalog_init();
    dsp_catalog_deinit();
    dsp_catalog_deinit(); /* must not crash */
    TEST_ASSERT_FALSE(dsp_catalog_is_initialized());
}

/* -------------------------------------------------------------------------
 * 3. dsp_catalog_get_json – serialisation
 * ------------------------------------------------------------------------- */

void test_catalog_get_json_null_buf_returns_null_arg(void)
{
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_NULL_ARG,
                      dsp_catalog_get_json(NULL, DSP_CATALOG_JSON_BUF_SIZE));
}

void test_catalog_get_json_buf_too_small_returns_error(void)
{
    char tiny[8];
    TEST_ASSERT_EQUAL(DSP_BUILD_ERR_BUF_TOO_SMALL,
                      dsp_catalog_get_json(tiny, sizeof(tiny)));
}

void test_catalog_get_json_returns_ok(void)
{
    char out[DSP_CATALOG_JSON_BUF_SIZE];
    TEST_ASSERT_EQUAL(DSP_BUILD_OK,
                      dsp_catalog_get_json(out, sizeof(out)));
}

void test_catalog_get_json_context_field(void)
{
    char out[DSP_CATALOG_JSON_BUF_SIZE];
    dsp_catalog_get_json(out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_CONTEXT_URL,
                             get_field(out, DSP_JSONLD_FIELD_CONTEXT));
}

void test_catalog_get_json_type_is_catalog(void)
{
    char out[DSP_CATALOG_JSON_BUF_SIZE];
    dsp_catalog_get_json(out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING(DSP_JSONLD_TYPE_CATALOG,
                             get_field(out, DSP_JSONLD_FIELD_TYPE));
}

void test_catalog_get_json_title_matches_config(void)
{
    char out[DSP_CATALOG_JSON_BUF_SIZE];
    dsp_catalog_get_json(out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING(CONFIG_DSP_CATALOG_TITLE,
                             get_field(out, "dct:title"));
}

void test_catalog_get_json_dataset_is_array(void)
{
    char out[DSP_CATALOG_JSON_BUF_SIZE];
    dsp_catalog_get_json(out, sizeof(out));
    TEST_ASSERT_TRUE(field_is_array(out, "dcat:dataset"));
}

/* -------------------------------------------------------------------------
 * 4. dsp_catalog_register_handler
 * ------------------------------------------------------------------------- */

void test_catalog_register_handler_before_init_returns_invalid_state(void)
{
    dsp_catalog_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
                      dsp_catalog_register_handler());
}

void test_catalog_register_handler_after_init_returns_ok(void)
{
    dsp_catalog_deinit();
    dsp_catalog_init();
    TEST_ASSERT_EQUAL(ESP_OK, dsp_catalog_register_handler());
    dsp_catalog_deinit();
}

/* -------------------------------------------------------------------------
 * 5. dsp_catalog_register_request_handler – disabled path (DSP-407)
 * ------------------------------------------------------------------------- */

void test_catalog_request_flag_is_disabled_by_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_CATALOG_REQUEST);
}

void test_catalog_request_register_before_init_returns_invalid_state(void)
{
    dsp_catalog_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
                      dsp_catalog_register_request_handler());
}

void test_catalog_request_register_disabled_returns_not_supported(void)
{
    dsp_catalog_deinit();
    dsp_catalog_init();
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED,
                      dsp_catalog_register_request_handler());
    dsp_catalog_deinit();
}
