/**
 * @file dsp_json.h
 * @brief cJSON wrapper with helpers for mandatory DSP JSON-LD fields.
 *
 * Wraps cJSON v1.7.19 (vendored in components/dsp_json/cjson/).  The wrapper
 * adds:
 *   - Null-safe parse/delete helpers.
 *   - Field accessors for the two mandatory DSP JSON-LD fields: \@context and
 *     \@type.
 *   - A generic string-field accessor and mandatory-fields presence check.
 *   - Simple object builder helpers (new object, add string field).
 *   - Serialisation to caller-supplied buffer (no dynamic allocation) and an
 *     alloc variant for cases where dynamic memory is acceptable.
 *
 * Memory: cJSON itself uses dynamic allocation via malloc/free.  The wrapper
 * does not add any static state; its RAM contribution is only the ~1 KB
 * cJSON code-size overhead.
 *
 * All functions are always compiled and have no platform-specific code; they
 * are suitable for both ESP32-S3 and host-native unit testing.
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * DSP JSON-LD field names
 * ------------------------------------------------------------------------- */

/** JSON-LD context field name. */
#define DSP_JSON_FIELD_CONTEXT  "@context"

/** JSON-LD type field name. */
#define DSP_JSON_FIELD_TYPE     "@type"

/* -------------------------------------------------------------------------
 * Parse / cleanup
 * ------------------------------------------------------------------------- */

/**
 * Parse a NUL-terminated JSON string.
 *
 * @param text  Input JSON string (may be NULL → returns NULL).
 * @return      Heap-allocated cJSON tree, or NULL on parse failure.
 *              Caller must pass the result to dsp_json_delete().
 */
cJSON *dsp_json_parse(const char *text);

/**
 * Free a cJSON tree returned by dsp_json_parse() or dsp_json_new_object().
 * Safe to call with NULL.
 */
void dsp_json_delete(cJSON *obj);

/* -------------------------------------------------------------------------
 * Field accessors
 * ------------------------------------------------------------------------- */

/**
 * Copy the string value of a named field into @p buf.
 *
 * @param obj  Root JSON object (may be NULL → returns false).
 * @param key  Field name (may be NULL → returns false).
 * @param buf  Destination buffer.
 * @param cap  Capacity of @p buf in bytes (must be ≥ 1).
 * @return     true if the field exists, is a string, and fits in @p buf.
 *             false otherwise; @p buf is NUL-terminated but may be empty.
 */
bool dsp_json_get_string(const cJSON *obj, const char *key,
                          char *buf, size_t cap);

/**
 * Read the \@type field from a DSP JSON object.
 *
 * Convenience wrapper around dsp_json_get_string(obj, "@type", buf, cap).
 */
bool dsp_json_get_type(const cJSON *obj, char *buf, size_t cap);

/**
 * Read the \@context field from a DSP JSON object.
 *
 * Convenience wrapper around dsp_json_get_string(obj, "@context", buf, cap).
 */
bool dsp_json_get_context(const cJSON *obj, char *buf, size_t cap);

/**
 * Return true if @p obj contains both \@context and \@type string fields.
 */
bool dsp_json_has_mandatory_fields(const cJSON *obj);

/* -------------------------------------------------------------------------
 * Builder helpers
 * ------------------------------------------------------------------------- */

/**
 * Allocate a new empty JSON object.
 *
 * @return  Heap-allocated cJSON object, or NULL on allocation failure.
 *          Caller must pass the result to dsp_json_delete().
 */
cJSON *dsp_json_new_object(void);

/**
 * Add (or replace) a string field in @p obj.
 *
 * @param obj  Target JSON object (must not be NULL).
 * @param key  Field name (must not be NULL).
 * @param val  String value (must not be NULL).
 * @return     true on success, false on allocation failure or NULL arguments.
 */
bool dsp_json_add_string(cJSON *obj, const char *key, const char *val);

/* -------------------------------------------------------------------------
 * Serialisation
 * ------------------------------------------------------------------------- */

/**
 * Serialise @p obj to a caller-supplied buffer (no dynamic allocation).
 *
 * @param obj  JSON tree to serialise (may be NULL → returns false).
 * @param buf  Destination buffer.
 * @param cap  Capacity of @p buf in bytes (must be ≥ 1).
 * @return     true if the serialised representation fits in @p buf.
 *             false if @p obj is NULL, @p buf is NULL, or the output is
 *             too large; @p buf is NUL-terminated but may be incomplete.
 */
bool dsp_json_print(const cJSON *obj, char *buf, size_t cap);

/**
 * Serialise @p obj to a freshly allocated string.
 *
 * @param obj  JSON tree to serialise.
 * @return     Heap-allocated NUL-terminated JSON string, or NULL on failure.
 *             Caller must free the string with dsp_json_free_str().
 */
char *dsp_json_print_alloc(const cJSON *obj);

/**
 * Free a string returned by dsp_json_print_alloc().
 * Safe to call with NULL.
 */
void dsp_json_free_str(char *str);

/* -------------------------------------------------------------------------
 * Version query (for compile-time verification in tests)
 * ------------------------------------------------------------------------- */

/**
 * Return the vendored cJSON version string ("major.minor.patch").
 */
const char *dsp_json_cjson_version(void);

#ifdef __cplusplus
}
#endif
