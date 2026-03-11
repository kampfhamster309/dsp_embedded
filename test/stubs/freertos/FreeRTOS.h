/**
 * @file freertos/FreeRTOS.h  (host stub)
 *
 * Minimal type definitions so DSP components that include FreeRTOS headers
 * compile on the host.  No actual RTOS functionality is provided.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint32_t TickType_t;
typedef int      BaseType_t;
typedef uint32_t UBaseType_t;

#define pdTRUE      ((BaseType_t) 1)
#define pdFALSE     ((BaseType_t) 0)
#define pdPASS      pdTRUE
#define pdFAIL      pdFALSE

#define portMAX_DELAY ((TickType_t) 0xFFFFFFFF)

/* Tick rate – matches sdkconfig.defaults CONFIG_FREERTOS_HZ=1000 */
#define configTICK_RATE_HZ 1000U
