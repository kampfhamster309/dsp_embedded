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
 * CONFIG_DSP_ENABLE_PSK
 *
 * Enables the Pre-Shared Key (PSK) authentication module (dsp_psk).
 * When disabled, no PSK symbols are compiled in, reducing code and RAM.
 * When enabled, PSK credentials can be stored and applied to TLS connections.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_ENABLE_PSK
#define CONFIG_DSP_ENABLE_PSK 0
#endif

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
 * Enables the dsp_power module: the node enters ESP deep sleep after each
 * completed transfer and wakes on a timer for the next acquisition cycle.
 * Protocol state (negotiations, transfers) is preserved in RTC memory across
 * sleep cycles via dsp_rtc_state_save/restore.
 * When disabled (default), the entire dsp_power module compiles away with
 * zero code/RAM overhead.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX
#define CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX 0
#endif

/**
 * CONFIG_DSP_WATCHDOG_TIMEOUT_MS
 *
 * Timeout in milliseconds for the task watchdog that guards the Core 1
 * acquisition loop. If the application task does not call
 * esp_task_wdt_reset() within this window the TWDT triggers a panic.
 * Must be greater than CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS.
 * Default: 5000 ms.
 */
#ifndef CONFIG_DSP_WATCHDOG_TIMEOUT_MS
#define CONFIG_DSP_WATCHDOG_TIMEOUT_MS 5000
#endif

/**
 * CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP
 *
 * Enables automatic CPU light-sleep via esp_pm between FreeRTOS idle ticks.
 * Reduces active-WiFi current from ~120 mA to ~20–40 mA with minimal
 * latency impact on the HTTP server.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP
#define CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP 0
#endif

/**
 * CONFIG_DSP_TEST_WATCHDOG_HANG
 *
 * DEVICE TEST ONLY – DO NOT ENABLE IN PRODUCTION.
 * Causes the acquisition task to busy-wait after its first sample without
 * feeding the watchdog, triggering a TWDT panic within
 * CONFIG_DSP_WATCHDOG_TIMEOUT_MS milliseconds.
 * Used to verify that the watchdog fires within the expected window.
 * Default: disabled.
 */
#ifndef CONFIG_DSP_TEST_WATCHDOG_HANG
#define CONFIG_DSP_TEST_WATCHDOG_HANG 0
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
 * WiFi connection manager
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_WIFI_MAX_RETRY
 *
 * Number of reconnect attempts before dsp_wifi transitions to FAILED.
 * Default: 5.
 */
#ifndef CONFIG_DSP_WIFI_MAX_RETRY
#define CONFIG_DSP_WIFI_MAX_RETRY 5
#endif

/**
 * CONFIG_DSP_WIFI_RECONNECT_DELAY_MS
 *
 * Milliseconds to wait between reconnect attempts.
 * Default: 1000 ms.
 */
#ifndef CONFIG_DSP_WIFI_RECONNECT_DELAY_MS
#define CONFIG_DSP_WIFI_RECONNECT_DELAY_MS 1000
#endif

/* -------------------------------------------------------------------------
 * Sensor acquisition
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS
 *
 * Milliseconds between successive ADC readings in the Core 1 acquisition loop.
 * Lower values increase sample rate but raise CPU load and ring buffer pressure.
 * Default: 100 ms (10 samples/second per channel).
 */
#ifndef CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS
#define CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS 100
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
 * Static catalog content
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_PROVIDER_ID
 *
 * IRI used as the provider's participant identifier in DSP messages.
 */
#ifndef CONFIG_DSP_PROVIDER_ID
#define CONFIG_DSP_PROVIDER_ID "urn:uuid:dsp-embedded-provider-1"
#endif

/**
 * CONFIG_DSP_CATALOG_DATASET_ID
 *
 * IRI identifying the single dataset entry in the GET /catalog response.
 */
#ifndef CONFIG_DSP_CATALOG_DATASET_ID
#define CONFIG_DSP_CATALOG_DATASET_ID "urn:uuid:dsp-embedded-sensor-dataset-1"
#endif

/**
 * CONFIG_DSP_CATALOG_TITLE
 *
 * dct:title of the static catalog.
 */
#ifndef CONFIG_DSP_CATALOG_TITLE
#define CONFIG_DSP_CATALOG_TITLE "DSP Embedded Sensor Catalog"
#endif

/* -------------------------------------------------------------------------
 * DHT20 sensor pin and port defaults (host build only; ESP builds use Kconfig)
 * ------------------------------------------------------------------------- */

/**
 * CONFIG_DSP_DHT20_SDA_PIN
 * GPIO number for the DHT20 I2C SDA line.  Ignored on host builds.
 */
#ifndef CONFIG_DSP_DHT20_SDA_PIN
#define CONFIG_DSP_DHT20_SDA_PIN 8
#endif

/**
 * CONFIG_DSP_DHT20_SCL_PIN
 * GPIO number for the DHT20 I2C SCL line.  Ignored on host builds.
 */
#ifndef CONFIG_DSP_DHT20_SCL_PIN
#define CONFIG_DSP_DHT20_SCL_PIN 9
#endif

/**
 * CONFIG_DSP_DHT20_I2C_PORT
 * I2C peripheral index (0 or 1).  Ignored on host builds.
 */
#ifndef CONFIG_DSP_DHT20_I2C_PORT
#define CONFIG_DSP_DHT20_I2C_PORT 0
#endif

/* -------------------------------------------------------------------------
 * Static assertions – catch invalid combinations at compile time
 *
 * Note: CONFIG_DSP_DAPS_GATEWAY_URL is a string and cannot be tested in
 * preprocessor expressions. The Kconfig dependency (depends on
 * DSP_ENABLE_DAPS_SHIM) enforces that the URL is set via menuconfig.
 * ------------------------------------------------------------------------- */

#ifdef ESP_PLATFORM
#if CONFIG_DSP_PSRAM_ENABLE && !defined(CONFIG_SPIRAM)
#warning "CONFIG_DSP_PSRAM_ENABLE requires CONFIG_SPIRAM=y in sdkconfig"
#endif
#endif
