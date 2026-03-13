/**
 * @file dsp_application.h
 * @brief DSP Embedded application task – Core 1 setup (DSP-503).
 *
 * Owns the sensor acquisition lifecycle:
 *   - Stub ADC polling loop (channels 0..DSP_SAMPLE_CHANNEL_MAX-1)
 *   - Pushes dsp_sample_t values into the shared ring buffer
 *   - Sets sh->core1_ready when running
 *
 * On ESP_PLATFORM the loop runs inside a dedicated FreeRTOS task pinned to
 * Core 1 via xTaskCreatePinnedToCore().  The task logs its core ID at startup
 * so the AC ("confirmed pinned to Core 1 by xTaskGetCoreID()") is verifiable
 * from the serial monitor.
 *
 * On the host build dsp_application_start() performs the init sequence and
 * one acquisition cycle synchronously so unit tests can verify that the ring
 * buffer contains at least one sample after start.
 *
 * Typical usage (after dsp_shared_init()):
 *   dsp_application_start(&g_shared);
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "dsp_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Stack size for the application task in bytes. */
#define DSP_APPLICATION_TASK_STACK  4096U

/** FreeRTOS priority for the application task. */
#define DSP_APPLICATION_TASK_PRIO   4U

/** CPU core the application task is pinned to. */
#define DSP_APPLICATION_TASK_CORE   1

/**
 * @brief Start the sensor acquisition task.
 *
 * On ESP_PLATFORM: creates a FreeRTOS task pinned to Core 1 that polls stub
 * ADC channels in a round-robin loop, pushes samples into the ring buffer, and
 * sets sh->core1_ready = 1.
 *
 * On host: runs one acquisition cycle synchronously and sets core1_ready so
 * unit tests can verify the ring buffer and ready flag without an RTOS.
 *
 * @param sh  Initialised shared state (from dsp_shared_init()).
 * @return ESP_OK on success.
 *         ESP_ERR_INVALID_ARG if sh is NULL.
 *         ESP_ERR_NO_MEM if FreeRTOS task creation fails (ESP only).
 */
esp_err_t dsp_application_start(dsp_shared_t *sh);

/**
 * @brief Stop the acquisition task and release resources.
 *
 * Clears s_running and core1_ready.  On ESP_PLATFORM the task is also
 * deleted.  Safe to call if not started.
 */
void dsp_application_stop(void);

/**
 * @brief Returns true if dsp_application_start() has been called and the
 *        initialisation sequence has completed.
 */
bool dsp_application_is_running(void);

#ifdef __cplusplus
}
#endif
