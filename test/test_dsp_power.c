/**
 * @file test_dsp_power.c
 * @brief Host-native tests for dsp_power with CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=0.
 *
 * This file is compiled as part of the default dsp_test_runner binary where
 * the flag is 0.  It verifies:
 *   - The flag defaults to disabled.
 *   - dsp_power.c compiles cleanly with the flag off (no stray symbols).
 *
 * The enabled-path tests live in test_dsp_power_on.c / dsp_test_deep_sleep_on.
 *
 * setUp/tearDown are defined in test_smoke.c.
 */

#include "unity.h"
#include "dsp_config.h"
#include "dsp_power.h"

/* -------------------------------------------------------------------------
 * Flag default
 * ------------------------------------------------------------------------- */

void test_power_flag_is_disabled_by_default(void)
{
    TEST_ASSERT_EQUAL_INT(0, CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX);
}
