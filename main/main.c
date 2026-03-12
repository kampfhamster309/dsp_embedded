#include <stdio.h>
#include "esp_log.h"
#include "dsp_config.h"
#include "dsp_mem.h"

static const char *TAG = "dsp_embedded";

void app_main(void)
{
    ESP_LOGI(TAG, "DSP Embedded starting...");
    dsp_mem_report("boot");
    ESP_LOGI(TAG, "Build config: catalog_request=%d consumer=%d daps_shim=%d "
                  "tls_tickets=%d psram=%d deep_sleep=%d terminate=%d "
                  "max_neg=%d max_tx=%d port=%d verbose=%d",
             CONFIG_DSP_ENABLE_CATALOG_REQUEST,
             CONFIG_DSP_ENABLE_CONSUMER,
             CONFIG_DSP_ENABLE_DAPS_SHIM,
             CONFIG_DSP_TLS_SESSION_TICKETS,
             CONFIG_DSP_PSRAM_ENABLE,
             CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX,
             CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE,
             CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS,
             CONFIG_DSP_MAX_CONCURRENT_TRANSFERS,
             CONFIG_DSP_HTTP_PORT,
             CONFIG_DSP_LOG_LEVEL_VERBOSE);
}
