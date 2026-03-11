/**
 * @file esp_log.h  (host stub)
 *
 * Maps ESP_LOG* macros to fprintf so DSP components compile and produce
 * readable output on the host without ESP-IDF.
 */
#pragma once

#include <stdio.h>

/* Numeric log level constants (mirror esp_log_level_t values). */
#define ESP_LOG_NONE    0
#define ESP_LOG_ERROR   1
#define ESP_LOG_WARN    2
#define ESP_LOG_INFO    3
#define ESP_LOG_DEBUG   4
#define ESP_LOG_VERBOSE 5

#ifndef DSP_HOST_LOG_LEVEL
#define DSP_HOST_LOG_LEVEL ESP_LOG_INFO
#endif

#define _DSP_LOG(level, letter, tag, fmt, ...)                          \
    do {                                                                 \
        if ((level) <= DSP_HOST_LOG_LEVEL)                              \
            fprintf(stderr, letter " (%s): " fmt "\n", tag, ##__VA_ARGS__); \
    } while (0)

#define ESP_LOGE(tag, fmt, ...) _DSP_LOG(ESP_LOG_ERROR,   "E", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) _DSP_LOG(ESP_LOG_WARN,    "W", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) _DSP_LOG(ESP_LOG_INFO,    "I", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) _DSP_LOG(ESP_LOG_DEBUG,   "D", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) _DSP_LOG(ESP_LOG_VERBOSE, "V", tag, fmt, ##__VA_ARGS__)
