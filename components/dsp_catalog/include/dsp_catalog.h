/**
 * @file dsp_catalog.h
 * @brief Static DSP catalog module — load and serve GET /catalog.
 *
 * Maintains a single, compile-time-fixed dcat:Catalog built from the
 * Kconfig options CONFIG_DSP_CATALOG_DATASET_ID and CONFIG_DSP_CATALOG_TITLE.
 * The catalog is serialised to JSON on demand via dsp_catalog_get_json();
 * no persistent allocation is retained between calls.
 *
 * The HTTP handler (dsp_catalog_register_handler) is always compiled and
 * delegates to dsp_http_register_handler(), which on host builds stores the
 * route in a static table without starting a real server.
 *
 * Memory: the serialisation buffer (DSP_CATALOG_JSON_BUF_SIZE) is supplied
 * by the caller; the module itself has only a 1-byte initialized flag in RAM.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "dsp_build.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Recommended minimum buffer size for dsp_catalog_get_json(). */
#define DSP_CATALOG_JSON_BUF_SIZE  512u

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Initialise the catalog module.
 * Safe to call multiple times; subsequent calls are no-ops.
 *
 * @return ESP_OK always.
 */
esp_err_t dsp_catalog_init(void);

/**
 * De-initialise the catalog module.
 * After this call dsp_catalog_is_initialized() returns false.
 */
void dsp_catalog_deinit(void);

/**
 * Return true if dsp_catalog_init() has been called and dsp_catalog_deinit()
 * has not been called since.
 */
bool dsp_catalog_is_initialized(void);

/* -------------------------------------------------------------------------
 * Serialisation
 * ------------------------------------------------------------------------- */

/**
 * Serialise the static catalog to a caller-supplied buffer.
 *
 * Produces a dcat:Catalog JSON object populated from
 * CONFIG_DSP_CATALOG_DATASET_ID and CONFIG_DSP_CATALOG_TITLE.
 * The function does not require the module to be initialised — the catalog
 * data is purely compile-time and always available.
 *
 * @param out  Output buffer (must not be NULL).
 * @param cap  Buffer capacity in bytes.
 * @return     DSP_BUILD_OK, DSP_BUILD_ERR_NULL_ARG, or
 *             DSP_BUILD_ERR_BUF_TOO_SMALL.
 */
dsp_build_err_t dsp_catalog_get_json(char *out, size_t cap);

/* -------------------------------------------------------------------------
 * HTTP handler registration
 * ------------------------------------------------------------------------- */

/**
 * Register the GET /catalog handler with the dsp_http server.
 *
 * Must be called after dsp_catalog_init() and after dsp_http_start().
 * On host builds the route is stored in the dsp_http stub table (no real
 * server is started).
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not initialised,
 *         or an error from dsp_http_register_handler().
 */
esp_err_t dsp_catalog_register_handler(void);

#ifdef __cplusplus
}
#endif
