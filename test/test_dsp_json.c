/**
 * @file test_dsp_json.c
 * @brief Host-native Unity tests for dsp_json (DSP-301).
 *
 * 22 tests covering:
 *   - cJSON version guard
 *   - parse / delete
 *   - dsp_json_get_string
 *   - dsp_json_get_type / get_context
 *   - dsp_json_has_mandatory_fields
 *   - builder helpers
 *   - serialisation (buffer + alloc)
 */

#include "unity.h"
#include "dsp_json.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

#define BUF128 128u
#define BUF8   8u

static const char *s_simple_json =
    "{\"@context\":\"https://w3id.org/dspace/v0.8\","
    "\"@type\":\"dspace:CatalogRequestMessage\","
    "\"filter\":\"\"}";

/* -------------------------------------------------------------------------
 * 1. cJSON version guard
 * ------------------------------------------------------------------------- */

void test_json_cjson_version_is_1_7(void)
{
    const char *ver = dsp_json_cjson_version();
    TEST_ASSERT_NOT_NULL(ver);
    /* Require major.minor == "1.7" */
    TEST_ASSERT_EQUAL_STRING_LEN("1.7", ver, 3u);
}

/* -------------------------------------------------------------------------
 * 2. Parse / delete
 * ------------------------------------------------------------------------- */

void test_json_parse_valid_returns_non_null(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    dsp_json_delete(obj);
}

void test_json_parse_invalid_returns_null(void)
{
    cJSON *obj = dsp_json_parse("{bad json");
    TEST_ASSERT_NULL(obj);
}

void test_json_parse_null_returns_null(void)
{
    TEST_ASSERT_NULL(dsp_json_parse(NULL));
}

void test_json_delete_null_is_safe(void)
{
    dsp_json_delete(NULL); /* must not crash */
}

/* -------------------------------------------------------------------------
 * 3. dsp_json_get_string
 * ------------------------------------------------------------------------- */

void test_json_get_string_existing_key(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF128];
    bool ok = dsp_json_get_string(obj, "@type", buf, sizeof(buf));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("dspace:CatalogRequestMessage", buf);
    dsp_json_delete(obj);
}

void test_json_get_string_missing_key_returns_false(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF128] = "unchanged";
    bool ok = dsp_json_get_string(obj, "nonexistent", buf, sizeof(buf));
    TEST_ASSERT_FALSE(ok);
    dsp_json_delete(obj);
}

void test_json_get_string_null_obj_returns_false(void)
{
    char buf[BUF128];
    TEST_ASSERT_FALSE(dsp_json_get_string(NULL, "@type", buf, sizeof(buf)));
}

void test_json_get_string_null_key_returns_false(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF128];
    TEST_ASSERT_FALSE(dsp_json_get_string(obj, NULL, buf, sizeof(buf)));
    dsp_json_delete(obj);
}

void test_json_get_string_buf_too_small_returns_false(void)
{
    /* "@context" value is 27 chars; cap=8 is too small */
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF8];
    bool ok = dsp_json_get_string(obj, "@context", buf, sizeof(buf));
    TEST_ASSERT_FALSE(ok);
    dsp_json_delete(obj);
}

void test_json_get_string_exact_fit(void)
{
    /* "hello" = 5 chars → cap=6 (including NUL) */
    cJSON *obj = dsp_json_parse("{\"k\":\"hello\"}");
    TEST_ASSERT_NOT_NULL(obj);
    char buf[6];
    bool ok = dsp_json_get_string(obj, "k", buf, sizeof(buf));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("hello", buf);
    dsp_json_delete(obj);
}

/* -------------------------------------------------------------------------
 * 4. dsp_json_get_type / get_context
 * ------------------------------------------------------------------------- */

void test_json_get_type_returns_correct_value(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF128];
    TEST_ASSERT_TRUE(dsp_json_get_type(obj, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("dspace:CatalogRequestMessage", buf);
    dsp_json_delete(obj);
}

void test_json_get_context_returns_correct_value(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF128];
    TEST_ASSERT_TRUE(dsp_json_get_context(obj, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("https://w3id.org/dspace/v0.8", buf);
    dsp_json_delete(obj);
}

/* -------------------------------------------------------------------------
 * 5. dsp_json_has_mandatory_fields
 * ------------------------------------------------------------------------- */

void test_json_has_mandatory_fields_both_present(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_TRUE(dsp_json_has_mandatory_fields(obj));
    dsp_json_delete(obj);
}

void test_json_has_mandatory_fields_missing_type(void)
{
    cJSON *obj = dsp_json_parse("{\"@context\":\"https://w3id.org/dspace/v0.8\"}");
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_FALSE(dsp_json_has_mandatory_fields(obj));
    dsp_json_delete(obj);
}

void test_json_has_mandatory_fields_missing_context(void)
{
    cJSON *obj = dsp_json_parse("{\"@type\":\"dspace:Foo\"}");
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_FALSE(dsp_json_has_mandatory_fields(obj));
    dsp_json_delete(obj);
}

void test_json_has_mandatory_fields_null_returns_false(void)
{
    TEST_ASSERT_FALSE(dsp_json_has_mandatory_fields(NULL));
}

/* -------------------------------------------------------------------------
 * 6. Builder helpers
 * ------------------------------------------------------------------------- */

void test_json_new_object_returns_non_null(void)
{
    cJSON *obj = dsp_json_new_object();
    TEST_ASSERT_NOT_NULL(obj);
    dsp_json_delete(obj);
}

void test_json_add_string_roundtrip(void)
{
    cJSON *obj = dsp_json_new_object();
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_TRUE(dsp_json_add_string(obj, "@context", "https://w3id.org/dspace/v0.8"));
    TEST_ASSERT_TRUE(dsp_json_add_string(obj, "@type", "dspace:AgreementMessage"));

    char buf[BUF128];
    TEST_ASSERT_TRUE(dsp_json_get_type(obj, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("dspace:AgreementMessage", buf);
    TEST_ASSERT_TRUE(dsp_json_has_mandatory_fields(obj));
    dsp_json_delete(obj);
}

void test_json_add_string_null_args_return_false(void)
{
    cJSON *obj = dsp_json_new_object();
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_FALSE(dsp_json_add_string(NULL, "k", "v"));
    TEST_ASSERT_FALSE(dsp_json_add_string(obj,  NULL, "v"));
    TEST_ASSERT_FALSE(dsp_json_add_string(obj,  "k",  NULL));
    dsp_json_delete(obj);
}

/* -------------------------------------------------------------------------
 * 7. Serialisation
 * ------------------------------------------------------------------------- */

void test_json_print_to_buffer(void)
{
    cJSON *obj = dsp_json_parse("{\"@type\":\"dspace:Foo\"}");
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF128];
    TEST_ASSERT_TRUE(dsp_json_print(obj, buf, sizeof(buf)));
    /* Must contain the key–value pair */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"@type\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"dspace:Foo\""));
    dsp_json_delete(obj);
}

void test_json_print_buffer_too_small_returns_false(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char buf[BUF8];
    TEST_ASSERT_FALSE(dsp_json_print(obj, buf, sizeof(buf)));
    dsp_json_delete(obj);
}

void test_json_print_alloc_and_free(void)
{
    cJSON *obj = dsp_json_parse(s_simple_json);
    TEST_ASSERT_NOT_NULL(obj);
    char *s = dsp_json_print_alloc(obj);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_NOT_NULL(strstr(s, "dspace:CatalogRequestMessage"));
    dsp_json_free_str(s);
    dsp_json_delete(obj);
}

void test_json_print_null_returns_false(void)
{
    char buf[BUF128];
    TEST_ASSERT_FALSE(dsp_json_print(NULL, buf, sizeof(buf)));
    TEST_ASSERT_NULL(dsp_json_print_alloc(NULL));
}
