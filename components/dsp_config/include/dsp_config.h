#pragma once

/**
 * @file dsp_config.h
 * @brief DSP Embedded compile-time feature flags.
 *
 * This header is the single include point for all DSP Embedded configuration.
 * On ESP-IDF builds, flags are controlled via Kconfig (idf.py menuconfig) and
 * resolved from the generated sdkconfig.h.  On host-native builds (unit tests)
 * the defaults defined below apply unless overridden on the compiler command line.
 *
 * Every CONFIG_DSP_* symbol maps 1-to-1 to a Kconfig option in
 * components/dsp_config/Kconfig.  Do not define these symbols by hand in
 * application code — use menuconfig or sdkconfig.defaults instead.
 */

/* Pull in generated Kconfig symbols when building under ESP-IDF. */
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

/* -------------------------------------------------------------------------
 * Feature flags – functional endpoints and roles
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_ENABLE_CATALOG_REQUEST
 *
 * Enables the POST /catalog/request endpoint (DSP priority: SHOULD).
 * Allows filtered catalog queries from counterparts.
 * Default: disabled (static GET /catalog is sufficient for most deployments).
 */
#ifndef CONFIG_DSP_ENABLE_CATALOG_REQUEST
#define CONFIG_DSP_ENABLE_CATALOG_REQUEST 0
#endif

/**
 * CONFIG_DSP_ENABLE_CONSUMER
 *
 * Activates consumer-side DSP endpoints so the node can initiate
 * negotiations and transfers toward other connectors.
 * Default: disabled (provider-only in initial release).
 */
#ifndef CONFIG_DSP_ENABLE_CONSUMER
#define CONFIG_DSP_ENABLE_CONSUMER 0
#endif

/**
 * CONFIG_DSP_ENABLE_DAPS_SHIM
 *
 * Enables a forwarding shim that proxies token requests to an external
 * DAPS/OAuth2 gateway.  Requires CONFIG_DSP_DAPS_GATEWAY_URL.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_ENABLE_DAPS_SHIM
#define CONFIG_DSP_ENABLE_DAPS_SHIM 0
#endif

/**
 * CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE
 *
 * Enables POST /negotiations/terminate (DSP priority: MAY).
 * Default: disabled.
 */
#ifndef CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE
#define CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE 0
#endif

/* -------------------------------------------------------------------------
 * Feature flags – transport and security
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_TLS_SESSION_TICKETS
 *
 * Enables TLS session ticket resumption to reduce handshake overhead
 * on reconnection after deep sleep or idle periods.
 * Default: enabled.
 */
#ifndef CONFIG_DSP_TLS_SESSION_TICKETS
#define CONFIG_DSP_TLS_SESSION_TICKETS 1
#endif

/* -------------------------------------------------------------------------
 * Feature flags – hardware and power
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_PSRAM_ENABLE
 *
 * Directs large heap allocations (TLS I/O buffers, HTTP body, JSON payloads)
 * to external PSRAM.  Requires ESP32-S3 with 8 MB PSRAM and CONFIG_SPIRAM=y.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_PSRAM_ENABLE
#define CONFIG_DSP_PSRAM_ENABLE 0
#endif

/**
 * CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX
 *
 * Enables deep-sleep power management between data transfers.
 * State is preserved in RTC memory.  Average idle current drops from
 * ~120 mA (WiFi active) to ~20 µA (deep sleep).
 * Default: disabled.
 */
#ifndef CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX
#define CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX 0
#endif

/* -------------------------------------------------------------------------
 * Capacity limits
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS
 *
 * Maximum number of simultaneously tracked contract negotiations.
 * Each entry consumes ~256 bytes of DSP state machine RAM.
 * Default: 4.
 */
#ifndef CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS
#define CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS 4
#endif

/**
 * CONFIG_DSP_MAX_CONCURRENT_TRANSFERS
 *
 * Maximum number of simultaneous active data transfers.
 * Each entry consumes ~512 bytes.
 * Default: 2.
 */
#ifndef CONFIG_DSP_MAX_CONCURRENT_TRANSFERS
#define CONFIG_DSP_MAX_CONCURRENT_TRANSFERS 2
#endif

/* -------------------------------------------------------------------------
 * Network
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_HTTP_PORT
 *
 * TCP port the DSP HTTP/1.1 server listens on.
 * Default: 80.
 */
#ifndef CONFIG_DSP_HTTP_PORT
#define CONFIG_DSP_HTTP_PORT 80
#endif

/* -------------------------------------------------------------------------
 * Diagnostics
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_LOG_LEVEL_VERBOSE
 *
 * When non-zero, logs full request/response bodies and state transitions.
 * Disable in production.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_LOG_LEVEL_VERBOSE
#define CONFIG_DSP_LOG_LEVEL_VERBOSE 0
#endif

/* -------------------------------------------------------------------------
 * DAPS gateway URL (string, only meaningful with CONFIG_DSP_ENABLE_DAPS_SHIM)
 * ------------------------------------------------------------------------- */

#ifndef CONFIG_DSP_DAPS_GATEWAY_URL
#define CONFIG_DSP_DAPS_GATEWAY_URL ""
#endif

/* -------------------------------------------------------------------------
 * Static assertions – catch invalid combinations at compile time
 *
 * Note: CONFIG_DSP_DAPS_GATEWAY_URL is a string and cannot be tested in
 * preprocessor expressions. The Kconfig dependency (depends on
 * DSP_ENABLE_DAPS_SHIM) enforces that the URL is set via menuconfig.
 * ------------------------------------------------------------------------- */

#if CONFIG_DSP_PSRAM_ENABLE && !defined(CONFIG_SPIRAM)
#warning "CONFIG_DSP_PSRAM_ENABLE requires CONFIG_SPIRAM=y in sdkconfig"
#endif
