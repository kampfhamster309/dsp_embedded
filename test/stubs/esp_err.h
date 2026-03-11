/**
 * @file esp_err.h  (host stub)
 *
 * Minimal esp_err_t definition and common error codes used by DSP components.
 */
#pragma once

#include <stdint.h>

typedef int32_t esp_err_t;

#define ESP_OK          ((esp_err_t)  0)
#define ESP_FAIL        ((esp_err_t) -1)
#define ESP_ERR_NO_MEM  ((esp_err_t) 0x101)
#define ESP_ERR_INVALID_ARG ((esp_err_t) 0x102)
#define ESP_ERR_INVALID_STATE ((esp_err_t) 0x103)
#define ESP_ERR_NOT_FOUND ((esp_err_t) 0x105)
#define ESP_ERR_TIMEOUT ((esp_err_t) 0x107)

#define ESP_ERROR_CHECK(x) do { \
    esp_err_t _rc = (x); \
    if (_rc != ESP_OK) { \
        fprintf(stderr, "ESP_ERROR_CHECK failed: %d at %s:%d\n", _rc, __FILE__, __LINE__); \
        abort(); \
    } \
} while (0)
