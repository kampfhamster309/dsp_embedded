/**
 * @file freertos/queue.h  (host stub)
 *
 * Stub type definitions for FreeRTOS queue handles used in DSP shared-memory
 * structures.  Actual queue operations are not implemented here — host-native
 * unit tests mock the interfaces they need.
 */
#pragma once

#include "FreeRTOS.h"

typedef void *QueueHandle_t;
typedef void *QueueSetHandle_t;
