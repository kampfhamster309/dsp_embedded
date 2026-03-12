/**
 * @file dsp_mem.c
 * @brief DSP Embedded RAM usage instrumentation implementation.
 *
 * Platform: ESP32-S3 (ESP-IDF / FreeRTOS).
 * The #else block provides host-native stubs for unit testing.
 */

#include "dsp_mem.h"

/* =========================================================================
 * ESP32 implementation
 * ========================================================================= */

#ifdef ESP_PLATFORM

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "dsp_config.h"

static const char *TAG = "dsp_mem";

/* Warn when free internal RAM falls below this threshold. */
#define DSP_MEM_WARN_THRESHOLD_B  (30u * 1024u)

esp_err_t dsp_mem_report(const char *tag)
{
    if (!tag) {
        tag = "(null)";
    }

    uint32_t free_int  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t min_int   = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    uint32_t largest   = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    ESP_LOGI(TAG, "[%s] internal free=%"PRIu32" B (%"PRIu32" KB)  "
             "min_free=%"PRIu32" B  largest_block=%"PRIu32" B",
             tag,
             free_int,  free_int  / 1024u,
             min_int,
             largest);

#if CONFIG_DSP_PSRAM_ENABLE
    uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "[%s] PSRAM free=%"PRIu32" B (%"PRIu32" KB)",
             tag, free_psram, free_psram / 1024u);
#endif

    if (free_int < DSP_MEM_WARN_THRESHOLD_B) {
        ESP_LOGW(TAG, "[%s] WARNING: free internal RAM %"PRIu32" B < %u B threshold",
                 tag, free_int, DSP_MEM_WARN_THRESHOLD_B);
    }

    return ESP_OK;
}

uint32_t dsp_mem_free_internal(void)
{
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t dsp_mem_free_psram(void)
{
#if CONFIG_DSP_PSRAM_ENABLE
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#else
    return 0u;
#endif
}

#else /* !ESP_PLATFORM – host-native stubs */

#include <stdio.h>
#include "dsp_config.h"

esp_err_t dsp_mem_report(const char *tag)
{
    if (!tag) {
        tag = "(null)";
    }
    /* Print to stdout so host test output is visible */
    printf("[dsp_mem][%s] host build: free_internal=%u B (%u KB)  psram=0 B\n",
           tag,
           (unsigned)DSP_MEM_HOST_FREE_B,
           (unsigned)(DSP_MEM_HOST_FREE_B / 1024u));
    return ESP_OK;
}

uint32_t dsp_mem_free_internal(void)
{
    return DSP_MEM_HOST_FREE_B;
}

uint32_t dsp_mem_free_psram(void)
{
    return 0u;
}

#endif /* ESP_PLATFORM */
