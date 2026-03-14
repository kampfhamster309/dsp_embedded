# Static Analysis Report – DSP-802

**Tool:** cppcheck 2.13.0
**Date:** 2026-03-14
**Scope:** `components/*/dsp_*.c` (19 source files) + `test/test_*.c` (34 source files)
**Standard:** C17, platform unix32

## Result

**Zero findings at severity ≥ medium** (error / warning / performance / portability).

```
cppcheck --enable=warning,performance,portability \
         --suppress=missingIncludeSystem \
         --std=c17 --error-exitcode=1 \
         -DDSP_HOST_BUILD=1 --platform=unix32 \
         components/*/dsp_*.c test/test_*.c
Exit code: 0  (no medium+ findings)
```

## Style-Level False Positives (not fixed)

Two `style`-severity findings were identified; both are **false positives** caused by
cppcheck analysing only the host `#else` stub branches:

| File | Line | Finding | Why it is a false positive |
|---|---|---|---|
| `dsp_daps/dsp_daps.c` | 54–55 | `constParameterPointer` for `out_buf` / `out_len` | The ESP_PLATFORM branch writes into `*out_buf` and `*out_len`; the host stub hits an early return before those writes, so cppcheck never sees them. Making the params `const` would break the device build. |
| `dsp_mbedtls/dsp_tls.c` | 231 | `constParameterPointer` for `ctx` | The ESP_PLATFORM `dsp_tls_server_init()` initialises mbedTLS state inside `*ctx`; the host `#else` stub only performs a NULL check and immediately returns `ESP_FAIL`. Same cause. |

Fixing these by adding `const` would be semantically incorrect for the device build and
is therefore deliberately left as-is.
