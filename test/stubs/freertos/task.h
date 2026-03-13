/**
 * @file freertos/task.h  (host stub)
 *
 * Minimal type definitions for FreeRTOS task API used by dsp_protocol.
 * No actual task scheduling is provided; host builds gate all task
 * creation behind ESP_PLATFORM.
 */
#pragma once

#include "FreeRTOS.h"

typedef void *TaskHandle_t;

static inline int xTaskGetCoreID(TaskHandle_t h) { (void)h; return 0; }
