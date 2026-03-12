/**
 * @file test_dsp_psk.c
 * @brief Disabled-path tests for the dsp_psk component (DSP-204).
 *
 * Compiled into dsp_test_runner with CONFIG_DSP_ENABLE_PSK=0 (default).
 * Verifies that the flag is off by default; the enabled path is covered
 * by the separate dsp_test_psk_on binary.
 */

#include "unity.h"
#include "dsp_config.h"

void test_psk_flag_is_disabled_by_default(void)
{
    TEST_ASSERT_EQUAL(0, CONFIG_DSP_ENABLE_PSK);
}
