# DSP Embedded – Agent Cache

## Current State

**Last completed ticket:** DSP-702 – Integration test: negotiation flow (18 tests, 1.65 s)
**Next ticket:** DSP-703 (M7 – Integration test: transfer flow)
**M5 validation status:** All ACs confirmed on ESP32-S3 (2026-03-13)

## Project Structure

```
dsp_embedded/
├── main/main.c               # Minimal app_main entry point
├── main/CMakeLists.txt
├── components/
│   ├── dsp_config/           # Feature flags: Kconfig + dsp_config.h
│   ├── dsp_mbedtls/         # TLS server context (DSP-101)
│   ├── dsp_http/            # HTTP/1.1 server skeleton (DSP-102)
│   ├── dsp_mem/             # RAM instrumentation: dsp_mem_report() (DSP-104)
│   ├── dsp_wifi/            # WiFi connection manager + SM (DSP-105)
│   ├── dsp_identity/        # X.509 cert+key provisioning via EMBED_FILES (DSP-201)
│   ├── dsp_jwt/             # JWT validation ES256+RS256 + helpers (DSP-202/203)
│   ├── dsp_psk/             # PSK credential management + TLS apply (DSP-204)
│   ├── dsp_daps/            # DAPS shim stub: init/request_token API (DSP-205)
│   ├── dsp_json/            # cJSON v1.7.19 wrapper + DSP JSON-LD field helpers (DSP-301)
│   ├── dsp_jsonld/          # Header-only: DSP JSON-LD context/type/field/state constants (DSP-302)
│   ├── dsp_msg/             # DSP message schema validators (DSP-303)
│   ├── dsp_build/           # DSP outgoing message builders (DSP-304)
│   ├── dsp_catalog/         # Static catalog module + GET /catalog handler (DSP-401)
│   ├── dsp_neg/             # Negotiation SM + slot table + all neg HTTP endpoints (DSP-402/403/404)
│   ├── dsp_xfer/            # Transfer SM + slot table + POST /transfers/start + notify (DSP-405)
│   ├── dsp_shared/          # Shared memory layout: ring buffer + FreeRTOS handles (DSP-501/504)
│   ├── dsp_protocol/        # Core 0 task: TLS + HTTP + state machines (DSP-502)
│   ├── dsp_application/     # Core 1 task: ADC stub polling → ring buffer (DSP-503)
│   ├── dsp_rtc_state/       # RTC slow-memory persistence: neg+xfer state across deep sleep (DSP-505)
│   └── dsp_power/           # Deep sleep entry: save RTC state → stop HTTP → esp_deep_sleep_start (DSP-601)
├── test/
│   ├── CMakeLists.txt        # Host-native standalone CMake project
│   ├── test_main.c           # Unity runner: UNITY_BEGIN/END + RUN_TEST calls
│   ├── test_smoke.c          # 15 smoke tests (pipeline + dsp_config defaults)
│   ├── test_dsp_tls.c        # 9 host-native tests for dsp_tls context (DSP-101)
│   ├── test_dsp_http.c       # 15 host-native tests for dsp_http (DSP-102)
│   ├── test_dsp_tls_tickets.c # 6 tests: session tickets ENABLED path (DSP-103)
│   ├── test_tickets_off.c    # 7 tests: session tickets DISABLED path (DSP-103)
│   ├── test_dsp_mem.c        # 16 host-native tests for dsp_mem (DSP-104)
│   ├── test_dsp_wifi.c      # 32 host-native tests for dsp_wifi SM + stubs (DSP-105)
│   ├── test_dsp_identity.c  # 20 host-native tests for dsp_identity (DSP-201)
│   ├── test_dsp_jwt.c       # 34+9 host-native tests for dsp_jwt (DSP-202/203)
│   ├── test_dsp_psk.c       # 27 host-native tests for dsp_psk (DSP-204)
│   ├── test_dsp_daps.c      # 17 host-native tests for dsp_daps (DSP-205)
│   ├── test_dsp_json.c      # 24 host-native tests for dsp_json (DSP-301)
│   ├── test_dsp_jsonld.c    # 21 host-native tests for dsp_jsonld_ctx.h (DSP-302)
│   ├── test_dsp_msg.c       # 31 host-native tests for dsp_msg validators (DSP-303)
│   ├── test_dsp_build.c     # 36 host-native tests for dsp_build builders (DSP-304)
│   ├── test_dsp_catalog.c   # 18 host-native tests for dsp_catalog (DSP-401)
│   ├── test_dsp_neg.c       # 37 host-native tests for dsp_neg SM + slots (DSP-402)
│   ├── test_dsp_protocol.c  # host-native tests for dsp_protocol init/lifecycle (DSP-502)
│   ├── test_dsp_application.c # 15 host-native tests for dsp_application (DSP-503)
│   ├── test_dsp_ring_buf.c  # 14 host-native tests: drain, overflow, wrap-around (DSP-504)
│   ├── test_dsp_rtc_state.c # 23 host-native tests: sizes, round-trip, load_slot (DSP-505)
│   ├── test_dsp_power.c     # 1 host-native test: flag disabled by default (DSP-601)
│   ├── test_dsp_power_on.c  # 19 host-native tests: enter_deep_sleep + handle_wakeup (DSP-601/602)
│   ├── test_dsp_power_on_main.c # Unity runner for dsp_test_deep_sleep_on binary (DSP-601/602)
│   └── test_tickets_off_main.c # Unity runner for dsp_test_no_tickets binary
│   ├── unity/                # git submodule: ThrowTheSwitch/Unity v2.6.0
│   └── stubs/                # ESP-IDF header shims for host builds
│       ├── esp_log.h         # ESP_LOG* → fprintf
│       ├── esp_err.h         # esp_err_t, ESP_OK/FAIL
│       └── freertos/         # FreeRTOS type stubs
├── doc/
│   ├── deviation_log.md      # DEV-001, DEV-002, DEV-003 documented
│   └── compatibility_matrix.md  # Stub, filled in M7
├── docker/
│   ├── docker-compose.yml    # dsp-counterpart service on port 18000
│   ├── dsp-counterpart/      # Python/FastAPI DSP consumer mock
│   │   ├── main.py           # DSP endpoints + /api/test/* control API
│   │   ├── requirements.txt  # fastapi, uvicorn, httpx
│   │   └── Dockerfile        # python:3.12-slim
│   ├── certs/                # Local test certs (gitignored, *.pem)
│   └── seed/
│       └── seed-test-data.sh # Health check + catalog probe
├── tools/                    # Empty – provisioning scripts added in M8
├── CMakeLists.txt            # Top-level ESP-IDF project file
├── partitions.csv            # NVS, phy, factory, dsp_certs, storage (4 MB)
├── sdkconfig.defaults        # ESP32-S3 defaults, mbedTLS reduction stubs
├── .gitignore                # Excludes build/, sdkconfig, *.bin, human_to_do.md
└── README.md
```

## Gotchas and Learnings

- **ESP-IDF v5.5.3** installed at `~/esp/esp-idf`. Source with: `source ~/esp/esp-idf/export.sh`
- `idf.py` is not on PATH by default — invoke via `python /home/cyphus309/esp/esp-idf/tools/idf.py` or source export.sh first.
- **Tractus-X EDC (all versions 0.7.7–0.12.0) requires a full Catena-X DCP/STS/BDRS stack** to boot — not suitable for minimal integration testing. Use the Python mock instead.
- **DSP counterpart**: `cd docker && docker compose up -d` → healthy in <10 s on port 18000. Control API at `/api/test/*`.
- **Private keys must never be committed** — even test keys. Use `docker/certs/` (gitignored for *.pem) and regenerate locally.
- **Unity submodule**: `test/unity` pinned to v2.6.0. Init with `git submodule update --init test/unity`. CMakeLists.txt emits FATAL_ERROR if missing.
- **Adding tests**: add `test_<component>.c`, declare externs in `test_main.c`, add `RUN_TEST()` calls, add source to `add_executable(dsp_test_runner ...)` in CMakeLists.txt.
- **Host build is in `test/`**, run with: `cd test && cmake -B build && cmake --build build && ctest --test-dir build`
- `DSP_HOST_BUILD=1` is defined for host builds; `ESP_PLATFORM` is intentionally absent so `dsp_config.h` skips `sdkconfig.h`
- `test/CMakeLists.txt` auto-discovers `components/*/include` — new component headers need no edits to the host build file; but component `.c` sources must be listed explicitly in `add_executable(dsp_test_runner ...)`
- **dsp_identity**: `EMBED_FILES certs/device_cert.der certs/device_key.der`. Both DER files are gitignored; generated by `bash tools/gen_dev_certs.sh` (ECDSA P-256, self-signed). Host stubs return 2-byte placeholder `{0x30, 0x00}` DER SEQUENCE. `dsp_identity_get_cert/key(NULL)` returns NULL. `get_*` returns NULL before `init()` and after `deinit()`. On ESP_PLATFORM validates cert with `mbedtls_x509_crt_parse_der` and logs subject + validity dates.
- **CMakeLists issue**: `if(NOT EXISTS ...)` in component CMakeLists runs during ESP-IDF requirements-scan phase where `CMAKE_CURRENT_SOURCE_DIR` differs — removed the check; ESP-IDF fails naturally if EMBED_FILES are absent.
- **dsp_wifi SM**: `dsp_wifi_sm_next(state, input, &retry, max_retry)` is platform-agnostic (always compiled). States: IDLE→CONNECTING→CONNECTED→RECONNECTING→FAILED. DISCONNECT from any state → IDLE + retry reset. Retry increments on DISCONNECTED; at max_retry → FAILED.
- **dsp_wifi NVS**: namespace "dsp_wifi", keys "ssid"/"pass" (max 15-char NVS key). `dsp_wifi_store_credentials()` validates SSID non-empty/non-null.
- **dsp_wifi reconnect**: `esp_timer` one-shot fires after `CONFIG_DSP_WIFI_RECONNECT_DELAY_MS` ms, calls `esp_wifi_connect()` + `DSP_WIFI_INPUT_RETRY`. Host stub: `init()` returns ESP_OK, `connect()` returns ESP_FAIL.
- **dsp_mem_report(tag)**: call before/after component init to measure per-component RAM impact. On ESP_PLATFORM uses `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)`, warns if < 30 KB free. On host returns fixed sentinel `DSP_MEM_HOST_FREE_B = 512 KB`. Called at "boot" in `app_main()`.
- **DSP-103 two-binary strategy**: compile-time flags can't be tested in one binary. `dsp_test_runner` has `CONFIG_DSP_TLS_SESSION_TICKETS=1` (default). `dsp_test_no_tickets` compiles with `-DCONFIG_DSP_TLS_SESSION_TICKETS=0` and verifies the disabled path. Both registered in CTest. Run with: `ctest --test-dir test/build`
- **dsp_http**: wraps `esp_http_server`; exposes `dsp_http_start(port)`, `dsp_http_stop()`, `dsp_http_register_handler(uri, method, fn)`, `dsp_http_is_running()`. Handler bridge uses `httpd_req_t::user_ctx` so a single `bridge_handler()` serves all routes. Server config: **stack=8192** (must be ≥8192 — POST handlers with two 1 KB stack buffers + cJSON overhead overflow at 4096), max_open_sockets=4, lru_purge_enable=true. Host `#else` stub preserves route table in static array (no real server) for test verification.
- **dsp_http method mapping**: `DSP_HTTP_{GET,POST,PUT,DELETE}` are DSP-internal enum values (0–3); mapped to ESP-IDF `HTTP_GET=1, HTTP_POST=3, HTTP_PUT=4, HTTP_DELETE=0` via `map_method()` in the ESP_PLATFORM block.
- **dsp_tls host stubs**: `dsp_tls.c` has an `#else` branch (no ESP_PLATFORM) providing `dsp_tls_server_init` (returns ESP_FAIL) and `dsp_tls_server_deinit`. `dsp_tls.h` requires `<stdbool.h>` and `<stddef.h>` (host-compatibility fixes applied in DSP-101).
- `test/build/` is gitignored; `test/test_main.c` is tracked (replaced by Unity runner in DSP-004)
- **Preprocessor string comparisons are invalid in `#if`**: `CONFIG_DSP_DAPS_GATEWAY_URL[0] == '\0'` caused a build error — string literals cannot be used in preprocessor expressions. Removed; Kconfig `depends on` handles the constraint instead.
- Two sdkconfig.defaults keys were renamed in IDF v5.x: `CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240` → `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240` and `CONFIG_ESP32S3_SPIRAM_SUPPORT` → `CONFIG_SPIRAM`. Fixed.
- `human_to_do.md` is excluded from git via `.gitignore` (user preference).
- `cache.md` is tracked in git (it's part of the agent workflow).
- **dsp_jwt**: `dsp_jwt_validate_es256()` and `dsp_jwt_validate_rs256()`. Shared `jwt_check_structure(jwt, key, len, expected_alg, parts)` validates args → format → alg → expiry for both. Always-compiled helpers: `dsp_jwt_split()`, `dsp_jwt_base64url_decode()`, `dsp_jwt_parse_alg()`, `dsp_jwt_parse_exp()`. ES256 ESP_PLATFORM: SHA-256 + ECDSA P-256; raw R||S (64 bytes) → DER via `rs_to_der()`. RS256 ESP_PLATFORM: SHA-256 + RSA-PKCS1v15; raw sig bytes (≤512) passed directly to `mbedtls_pk_verify()`. Host `#else`: both return `DSP_JWT_ERR_CRYPTO` after pre-crypto checks. Error codes: OK=0, INVALID_ARG=-1, INVALID_FORMAT=-2, INVALID_ALG=-3, EXPIRED=-4, NO_EXP=-5, DECODE=-6, CRYPTO=-7.
- **dsp_jwt exp field**: uses `uint64_t` to handle `exp=9999999999` (year 2286) without uint32 overflow. `parse_exp` does simple string search for `"exp":` and parses decimal integer.
- **dsp_jwt base64url decode**: RFC 4648 §5; no padding; processes 4-char groups → 3 bytes; trailing 2 chars → 1 byte, 3 chars → 2 bytes; 1 trailing char is invalid (-1). Returns decoded byte count or -1.
- **dsp_daps**: `dsp_daps_init/deinit/is_initialized/request_token`. Error precedence: INVALID_ARG → NOT_INIT → DISABLED (CONFIG_DSP_ENABLE_DAPS_SHIM=0) → NO_URL (empty CONFIG_DSP_DAPS_GATEWAY_URL) → HTTP (stub). ESP_PLATFORM + shim enabled: HTTP path is a TODO stub returning ERR_HTTP. Host with default config always returns ERR_DISABLED. `dsp_daps.c` depends only on `dsp_config`; no mbedtls/http client yet.
- **dsp_psk**: `dsp_psk_init/deinit/set/is_configured/get_identity/get_key`. Static buffers: `s_identity[64]`, `s_key[32]`. Constraints: identity 1–64 B, key 16–32 B (min 128-bit). `dsp_psk_apply(mbedtls_ssl_config *)` is ESP_PLATFORM-only; calls `mbedtls_ssl_conf_psk()` + restricts to PSK_WITH_AES_128_GCM_SHA256 / PSK_WITH_AES_256_GCM_SHA384. `deinit()` zero-wipes key material. 27 host-native tests.
- **dsp_neg**: Full provider SM (`dsp_neg_sm_next` — pure fn), slot table (`dsp_neg_init/deinit`, `dsp_neg_create/apply/get_state/get_consumer_pid/get_provider_pid/find_by_cpid/count_active/is_active`). `DSP_NEG_MAX=CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS`, `DSP_NEG_PID_LEN=64`. `dsp_neg_register_handlers()` registers POST /negotiations/offers. ESP_PLATFORM handler: reads body, extracts processId, creates slot, applies OFFER event, returns 201 + ContractNegotiationEventMessage OFFERED. Host stub fn avoids NULL handler rejection. TAG guarded by #ifdef ESP_PLATFORM. New config: `CONFIG_DSP_PROVIDER_ID` (default "urn:uuid:dsp-embedded-provider-1") in Kconfig + dsp_config.h. 37 host-native tests.
- **dsp_catalog**: `dsp_catalog_init/deinit/is_initialized`, `dsp_catalog_get_json(out,cap)` (delegates to `dsp_build_catalog` with `CONFIG_DSP_CATALOG_DATASET_ID` + `CONFIG_DSP_CATALOG_TITLE`), `dsp_catalog_register_handler()` (always-compiled; on host passes stub fn ptr to dsp_http). New config keys: `CONFIG_DSP_CATALOG_DATASET_ID` (default "urn:uuid:dsp-embedded-sensor-dataset-1"), `CONFIG_DSP_CATALOG_TITLE` (default "DSP Embedded Sensor Catalog") — added to both Kconfig and dsp_config.h. ESP_PLATFORM path has real `httpd_req_t` handler; host path uses `void *req` stub. 18 host-native tests.
- **dsp_build**: Builders: `dsp_build_catalog(out,cap,dataset_id,title)`, `dsp_build_agreement_msg(out,cap,process_id,agreement_id)`, `dsp_build_negotiation_event(out,cap,process_id,state)`, `dsp_build_transfer_start(out,cap,process_id,endpoint_url)`, `dsp_build_transfer_completion(out,cap,process_id)`, `dsp_build_error(out,cap,code,reason)`. All take caller-supplied buffer; use `stamp_common()` + `finalise()` internals. Output verified by round-trip parse in tests. 36 host-native tests.
- **dsp_msg**: Validators: `dsp_msg_validate_catalog_request/offer/agreement/transfer_request`. Each checks: NULL → ERR_NULL_INPUT, parse fail → ERR_PARSE, missing @context → ERR_MISSING_CONTEXT, wrong @type → ERR_WRONG_TYPE, missing required field → ERR_MISSING_FIELD. offer requires `dspace:processId` (string) + `dspace:offer` (object); agreement requires `processId` + `dspace:agreement` (object); transfer_request requires `processId` + `dspace:dataset` (string). catalog_request needs only @context + @type. Depends on dsp_json + dsp_jsonld. 31 host-native tests.
- **dsp_jsonld**: Header-only component (`dsp_jsonld_ctx.h`). Defines: `DSP_JSONLD_CONTEXT_URL`, namespace prefixes (`DSPACE/DCAT/ODRL/DCT`), JSON-LD field names (`@context/@type/@id`), DSP field names (`dspace:processId` etc.), catalog/negotiation/transfer/error message types, negotiation states (`REQUESTED`…`TERMINATED`), transfer states (`REQUESTED/STARTED/COMPLETED/TERMINATED/SUSPENDED`). DEV-001 deviation log updated with static-context rationale. 21 host-native tests. No `.c` source; no entry needed in test executable sources.
- **dsp_json**: cJSON v1.7.19 vendored in `components/dsp_json/cjson/` (copied from ESP-IDF bundled copy). Always compiled; no ESP_PLATFORM split. Wrapper: `dsp_json_parse/delete`, `dsp_json_get_string/type/context`, `dsp_json_has_mandatory_fields`, `dsp_json_new_object`, `dsp_json_add_string`, `dsp_json_print` (static buf), `dsp_json_print_alloc/free_str`. Host build requires `"${COMPONENTS_DIR}/dsp_json/cjson"` added to `target_include_directories` in test/CMakeLists.txt (cjson/ is not under include/). 24 host-native tests.
- Git default branch is `main` (renamed from `master`).
- The `sdkconfig` file (generated by `idf.py`) is intentionally gitignored — only `sdkconfig.defaults` is tracked.

### M5 / Dual-Core (DSP-501–505)

- **dsp_shared**: `dsp_shared_t` holds ring buffer (`dsp_sample_t ring[DSP_RING_BUF_SIZE]`, head/tail/count uint32), mutex (`SemaphoreHandle_t mutex`), notify queues (`data_notify_q`, `xfer_notify_q`), and ready flags (`core0_ready`, `core1_ready`). Size: ~548 B on ESP32-S3, ~560 B on x86-64 (pointer width difference). All sizes asserted with `_Static_assert`.
- **Ring buffer overflow policy: DROP NEWEST**. `dsp_ring_buf_push()` returns `ESP_ERR_NO_MEM` when full; caller (Core 1) discards the sample and logs a warning. Not configurable — documented in a code comment. The buffer fills in ~3.2 s at 100 ms / 4 channels with no consumer; this is the expected steady-state on device without an active transfer.
- **`dsp_ring_buf_drain()`**: acquires mutex once, pops `min(count, capacity)` samples, releases mutex. More efficient than calling `pop()` in a loop. Used by Core 0 when consuming samples for transfer.
- **`data_notify_q`**: depth-1 `QueueHandle_t` in `dsp_shared_t`; Core 1 posts to it non-blocking after each successful push (coalescing: multiple rapid pushes → at most 1 notification pending). Allows Core 0 to block instead of spin-poll. Only signalled under `#ifdef ESP_PLATFORM`.
- **Host stub for tasks**: `#ifdef ESP_PLATFORM` guards all `xTaskCreatePinnedToCore` calls. On host, `dsp_protocol_start()` and `dsp_application_start()` run synchronously (one cycle): `application_init(sh)` + `acquire_one_sample(0)`. This is sufficient for unit test coverage without FreeRTOS.
- **TLS host stub non-fatal pattern**: `dsp_tls_server_init` always returns `ESP_FAIL` on host (no mbedTLS backend). Protocol init must treat this as a warning, not a fatal error, to allow host tests to pass the full init sequence. Pattern: `ESP_LOGW(TAG, "TLS init returned 0x%x (expected on host)", err)` — do not `return err`. Same applies to `dsp_http_start`.
- **`esp_timer.h` in component CMakeLists**: If a component uses `esp_timer_get_time()` inside `#ifdef ESP_PLATFORM`, the ESP-IDF build still requires `esp_timer` in `REQUIRES`. Missing it causes `fatal error: esp_timer.h: No such file or directory` at build time even though the call is guarded.
- **WiFi init non-fatal for M5 validation**: `dsp_wifi_init()` returns an error if no NVS credentials are stored (`0x1102`). For M5 validation, WiFi failure must be non-fatal so `dsp_protocol_start()` and `dsp_application_start()` still run. Pattern in `app_main`: `ESP_LOGW + continue` instead of `return`.
- **dsp_rtc_state**: `RTC_DATA_ATTR static dsp_rtc_state_t s_store` — survives deep sleep on ESP. On host: plain `static`. Magic sentinel: `0x44355035U` ("D5P5"). CRC32 (IEEE 802.3 reflected, poly `0xEDB88320U`) over `neg[]` + `xfer[]` arrays only. Struct sizes: `dsp_rtc_neg_slot_t` = 136 B, `dsp_rtc_xfer_slot_t` = 72 B — both asserted with `_Static_assert`.
- **`dsp_neg_load_slot()` / `dsp_xfer_load_slot()`**: Direct slot-injection APIs added for `dsp_rtc_state_restore()`. Bypass the state machine and notify callbacks to avoid side effects during restore. Not the same as the normal `apply(event)` path.
- **`esp_err.h` stub**: Host stubs must define `ESP_ERR_INVALID_CRC ((esp_err_t) 0x109)` — it's used by `dsp_rtc_state` but not present in the minimal host stub by default.

### M6 / Power Management (DSP-601–)

- **dsp_power**: Entire component compiles away when `CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=0` — all code is inside `#if CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX`. Verified with `nm` on device ELF: zero `dsp_power` symbols when flag=0.
- **`dsp_power_enter_deep_sleep()` sequence**: (1) `dsp_rtc_state_save()`, (2) `dsp_http_stop()`, (3) `esp_deep_sleep_start()` on ESP / mock sleep fn on host. Never returns on ESP hardware.
- **`dsp_power_handle_wakeup()` sequence**: (1) get wakeup cause (real or mock), (2) if not TIMER → return `ESP_ERR_INVALID_STATE` (cold boot path), (3) `dsp_rtc_state_restore()`, (4) `dsp_wifi_connect()` non-fatal, (5) `dsp_http_start()` non-fatal → return `ESP_OK`.
- **`dsp_power_wakeup_cause_t` enum**: `UNDEFINED=0, TIMER=1, OTHER=2`. Independent of `esp_sleep_wakeup_cause_t` so tests work on host. On ESP, `get_wakeup_cause()` maps from `esp_sleep_get_wakeup_cause()`. On host, injected via `dsp_power_set_wakeup_cause()`.
- **Host mock wakeup cause**: `dsp_power_set_wakeup_cause(cause)` — host only (`#ifndef ESP_PLATFORM`). Default is `UNDEFINED`. Reset in setUp/tearDown.
- **Host mock sleep fn**: `dsp_power_set_sleep_fn(fn, arg)` — host only. Used in tests to intercept sleep entry and assert state was already saved before callback fires.
- **`dsp_http_start()` host stub always returns `ESP_FAIL`** — never sets `is_running=true`. Do not assert `is_running()` is true after calling `dsp_http_start()` on host. Only assert it's false after `enter_deep_sleep` (stop was called).
- **`dsp_wifi_connect()` host stub**: returns `ESP_FAIL` unconditionally, no state check, safe to call without prior `dsp_wifi_init()`. `dsp_power_handle_wakeup()` treats a FAIL from `dsp_wifi_connect()` as a warning (non-fatal).
- **`dsp_test_deep_sleep_on` binary**: compiled with `-DCONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=1`. Sources: `dsp_power.c`, `dsp_rtc_state.c`, `dsp_neg.c`, `dsp_xfer.c`, `dsp_http.c`, `dsp_json.c`, `cjson/cJSON.c`, `dsp_wifi.c`. Needs `dsp_json/cjson` in `target_include_directories` because `dsp_xfer.c` → `dsp_json.h` → `cJSON.h`.
- **ESP-IDF CMakeLists for dsp_power**: `REQUIRES dsp_config dsp_rtc_state dsp_http dsp_wifi esp_hw_support esp_pm esp_system`. `esp_deep_sleep_start()` is in `esp_hw_support`. `esp_task_wdt.h` is in `esp_system` (not a separate component). `esp_pm_configure()` is in `esp_pm`.
- **`dsp_power_pm_init()` (DSP-603)**: Always compiled on `ESP_PLATFORM` (NOT behind deep-sleep flag). Call from `app_main` before starting tasks. (1) `esp_task_wdt_reconfigure(&wdt_cfg)` with `timeout_ms=CONFIG_DSP_WATCHDOG_TIMEOUT_MS, trigger_panic=true`; falls back to `esp_task_wdt_init()` if `ESP_ERR_INVALID_STATE`. (2) Optionally `esp_pm_configure()` if `CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP=1`.
- **`esp_task_wdt` subscription in `dsp_application.c`**: `esp_task_wdt_add(NULL)` in `application_init()` (inside the task). `esp_task_wdt_reset()` called once per acquisition loop iteration. `esp_task_wdt_delete(s_task_handle)` in `dsp_application_stop()` before `vTaskDelete()`. All within `#ifdef ESP_PLATFORM`. `esp_task_wdt.h` header lives in `esp_system` component.
- **`CONFIG_DSP_TEST_WATCHDOG_HANG` (device test flag)**: Build with `sdkconfig.hang_test` containing `CONFIG_DSP_TEST_WATCHDOG_HANG=y`. After first sample, acquisition task logs "DSP-603 hang test: blocking..." and enters `for(;;)` without feeding WDT. TWDT fires at ~5000ms with `E (task_wdt) Task watchdog got triggered... - dsp_application (CPU 1)`. Default=0; never enable in production.
- **TWDT timeout window observed**: TWDT fires at ~5000 ms (`task_wdt` log at `I(8786)` = 5 s after hang message at `I(3787)`). Ring buffer fills in ~3.2 s at 100 ms / 4 channels; normal acquisition feeds WDT each 100 ms so TWDT never fires in normal operation.
- **`esp_task_wdt.h` component location gotcha**: `esp_task_wdt` is NOT a standalone ESP-IDF component name. The header lives in `esp_system`. Use `REQUIRES ... esp_system` not `esp_task_wdt` — the latter causes `Failed to resolve component 'esp_task_wdt'` at cmake time.
- **DSP-604 power measurement**: `doc/power_measurement.md` has full procedure + expected ranges. Physical measurements pending (requires ammeter; added to `human_to_do.md`). Datasheet figures: Active WiFi 105–130 mA avg; light-sleep 20–45 mA avg (DTIM-dependent); deep-sleep 7–15 µA (RTC fast+slow on).

### M7 / Integration Testing (DSP-701–)

- **Integration test layout**: `integration/` dir with `conftest.py` (fixtures), `test_701_catalog.py`, `test_702_negotiation.py`, `requirements.txt`, `pytest.ini`. Run with: `integration/.venv/bin/pytest integration/ --provider-url http://<device-ip> --counterpart-url http://localhost:18000 -v`
- **Provider URL**: Device IP 192.168.178.107 (may change on DHCP reassignment). Check with boot log or router DHCP table. Env var: `PROVIDER_DSP_URL`.
- **DSP counterpart**: `cd docker && docker compose up -d`. Healthy at `http://localhost:18000/health`. Control API at `/api/test/*`. Endpoints: `POST /api/test/catalog`, `POST /api/test/negotiate`, `POST /api/test/agree`, `POST /api/test/transfer`, `GET /api/test/negotiations`, `GET /api/test/transfers`.
- **WiFi provisioning fix (main.c)**: `dsp_wifi_store_credentials()` requires NVS to be initialized first (done by `dsp_wifi_init()`). When `CONFIG_DSP_WIFI_PROVISION=y`, pass credentials directly via `dsp_wifi_config_t` to `dsp_wifi_init()`, then call `store_credentials()` after init to persist for subsequent boots. The old ordering (store before init) produced `0x1101` (NVS_NOT_INITIALIZED) and credentials were never written.
- **Integration venv**: `python3 -m venv integration/.venv && integration/.venv/bin/pip install pytest httpx`. Venv is gitignored.
- **DSP-701 result (2026-03-14)**: 14 tests passed in 1.86 s. GET /catalog validated `@type=dcat:Catalog`, `@context`, `dcat:dataset`, `dct:title`, `@id` fields.
- **DSP-702 result (2026-03-14)**: 18 tests passed in 1.65 s. Full offer→agree state machine verified directly and via counterpart. Primary AC: final `GET /negotiations/{id}` returns `dspace:eventType=dspace:AGREED`.
- **`httpd_uri_match_wildcard` limitation**: ONLY supports trailing wildcards (`*` at end of pattern). `/negotiations/*/agree` with `*` in the middle NEVER matches anything. Fix: register agree handler as `POST /negotiations/*` (trailing wildcard) — `extract_neg_id()` already strips any `/agree` suffix from `req->uri`. Registration order ensures `POST /negotiations/offers` (exact) matches first for that specific path.
- **Negotiation slot exhaustion**: `DSP_NEG_MAX=4` and slots are never freed during a session. Integration tests MUST use module-scoped fixtures that run the flow once and cache results. Never create one negotiation per test (10 tests × 1 slot = out of table).
- **DSP negotiation message field**: Device's `offers_post_handler` reads `dspace:processId` (not `dspace:consumerPid`) from the incoming `ContractOfferMessage`. Counterpart's make_offer_body must send `"dspace:processId": "urn:uuid:{consumer_pid}"`.
- **DSP negotiation state field**: `GET /negotiations/{id}` and all negotiation event responses use `dspace:eventType` (NOT `dsp:state`) for the state value. Values: `dspace:OFFERED`, `dspace:AGREED`, etc.
- **Device does NOT send callbacks after agree**: When `POST /negotiations/{id}/agree` succeeds, the device does NOT call the consumer's `callbackAddress`. The counterpart's `/api/test/agree` endpoint manually sets `negotiations[consumer_pid]["state"] = "AGREED"` after confirming the device responded with `dspace:eventType=dspace:AGREED`.

### ESP32-S3 Board / Serial Capture

- **Board**: ESP32-S3 (QFN56, revision v0.2), 8 MB PSRAM (AP_3v3), 40 MHz crystal. MAC: `80:b5:4e:d9:bd:38`. Port: `/dev/ttyACM1`. Baud: 115200. USB mode: USB-Serial/JTAG (no external USB-UART chip needed).
- **Flash/monitor workflow**:
  - Build: `source ~/esp/esp-idf/export.sh && idf.py build`
  - Flash: `idf.py -p /dev/ttyACM1 flash`
  - Flash+monitor: `idf.py -p /dev/ttyACM1 flash monitor` (interactive; not usable from script)
  - Script reset+capture: open `/dev/ttyACM1` first, then toggle RTS low→high to trigger reset, then read. Use the Python snippet below — do NOT use `esptool run` in a subshell before starting the capture; it completes the boot before `cat` starts.
- **Capturing the full boot log from a script** (the reliable pattern):
  ```python
  import serial, time, sys
  with serial.Serial("/dev/ttyACM1", 115200, timeout=0.1) as ser:
      ser.dtr = False; ser.rts = True; time.sleep(0.1); ser.rts = False
      start = time.time()
      while time.time() - start < 20:
          d = ser.read(256)
          if d: sys.stdout.buffer.write(d); sys.stdout.flush()
  ```
  The key is: **open the port before asserting RTS**. If `esptool run` runs in a subshell beforehand, the device finishes booting before the capture begins.
- **USB-CDC re-enumeration delay**: `app_main` has a 3-second `vTaskDelay` at the top to allow the USB-CDC host to re-enumerate after a firmware reset. This is intentional — do not remove it.
- **Boot timestamps**: The ESP32-S3 starts counting from 0 ms at reset. `I (3501) dsp_protocol: ...` means 3501 ms from reset. The 3-second USB delay is visible as the gap between reset and first `dsp_embedded` log line.
- **Heap at boot** (observed, 2026-03-13): `[boot] 313 KB free`. After full dual-core init: `[after-application] min_free=240 KB`. Protocol stack (TLS+HTTP+WiFi driver): ~73 KB consumed total.
- **WiFi credentials**: `sdkconfig.wifi` exists in the project root with real WiFi credentials. Use it when a ticket requires actual WiFi connectivity on the board: `idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi" build` (or merge into sdkconfig.defaults temporarily). The file is gitignored (contains credentials). Without this file, `dsp_wifi_init` returns `0x1102` (`ESP_ERR_NVS_NOT_FOUND`).
- **`idf.py` not on PATH**: always `source ~/esp/esp-idf/export.sh` first, or invoke as `python /home/cyphus309/esp/esp-idf/tools/idf.py`.
- **`python` vs `python3`**: The system has `python3` but not `python`. Use `python3 -m esptool` not `python -m esptool`.

## Partition Layout (4 MB flash)

| Name       | Type | Offset     | Size    | Purpose                        |
|------------|------|------------|---------|--------------------------------|
| nvs        | data | 0x9000     | 24 KB   | WiFi credentials, runtime NVS  |
| phy_init   | data | 0xF000     | 4 KB    | RF calibration data            |
| factory    | app  | 0x10000    | 1.5 MB  | Main firmware                  |
| dsp_certs  | data | 0x190000   | 64 KB   | X.509 certs and keys (DSP-201) |
| storage    | data | 0x1A0000   | 384 KB  | General sensor/app storage     |
