# DSP Embedded – Agent Cache

## Current State

**Last completed ticket:** DSP-401 – Catalog state machine (dsp_catalog)
**Next ticket:** DSP-402

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
│   └── dsp_catalog/         # Static catalog module + GET /catalog handler (DSP-401)
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
- **dsp_http**: wraps `esp_http_server`; exposes `dsp_http_start(port)`, `dsp_http_stop()`, `dsp_http_register_handler(uri, method, fn)`, `dsp_http_is_running()`. Handler bridge uses `httpd_req_t::user_ctx` so a single `bridge_handler()` serves all routes. Server config: stack=4096, max_open_sockets=4, lru_purge_enable=true. Host `#else` stub preserves route table in static array (no real server) for test verification.
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
- **dsp_catalog**: `dsp_catalog_init/deinit/is_initialized`, `dsp_catalog_get_json(out,cap)` (delegates to `dsp_build_catalog` with `CONFIG_DSP_CATALOG_DATASET_ID` + `CONFIG_DSP_CATALOG_TITLE`), `dsp_catalog_register_handler()` (always-compiled; on host passes stub fn ptr to dsp_http). New config keys: `CONFIG_DSP_CATALOG_DATASET_ID` (default "urn:uuid:dsp-embedded-sensor-dataset-1"), `CONFIG_DSP_CATALOG_TITLE` (default "DSP Embedded Sensor Catalog") — added to both Kconfig and dsp_config.h. ESP_PLATFORM path has real `httpd_req_t` handler; host path uses `void *req` stub. 18 host-native tests.
- **dsp_build**: Builders: `dsp_build_catalog(out,cap,dataset_id,title)`, `dsp_build_agreement_msg(out,cap,process_id,agreement_id)`, `dsp_build_negotiation_event(out,cap,process_id,state)`, `dsp_build_transfer_start(out,cap,process_id,endpoint_url)`, `dsp_build_transfer_completion(out,cap,process_id)`, `dsp_build_error(out,cap,code,reason)`. All take caller-supplied buffer; use `stamp_common()` + `finalise()` internals. Output verified by round-trip parse in tests. 36 host-native tests.
- **dsp_msg**: Validators: `dsp_msg_validate_catalog_request/offer/agreement/transfer_request`. Each checks: NULL → ERR_NULL_INPUT, parse fail → ERR_PARSE, missing @context → ERR_MISSING_CONTEXT, wrong @type → ERR_WRONG_TYPE, missing required field → ERR_MISSING_FIELD. offer requires `dspace:processId` (string) + `dspace:offer` (object); agreement requires `processId` + `dspace:agreement` (object); transfer_request requires `processId` + `dspace:dataset` (string). catalog_request needs only @context + @type. Depends on dsp_json + dsp_jsonld. 31 host-native tests.
- **dsp_jsonld**: Header-only component (`dsp_jsonld_ctx.h`). Defines: `DSP_JSONLD_CONTEXT_URL`, namespace prefixes (`DSPACE/DCAT/ODRL/DCT`), JSON-LD field names (`@context/@type/@id`), DSP field names (`dspace:processId` etc.), catalog/negotiation/transfer/error message types, negotiation states (`REQUESTED`…`TERMINATED`), transfer states (`REQUESTED/STARTED/COMPLETED/TERMINATED/SUSPENDED`). DEV-001 deviation log updated with static-context rationale. 21 host-native tests. No `.c` source; no entry needed in test executable sources.
- **dsp_json**: cJSON v1.7.19 vendored in `components/dsp_json/cjson/` (copied from ESP-IDF bundled copy). Always compiled; no ESP_PLATFORM split. Wrapper: `dsp_json_parse/delete`, `dsp_json_get_string/type/context`, `dsp_json_has_mandatory_fields`, `dsp_json_new_object`, `dsp_json_add_string`, `dsp_json_print` (static buf), `dsp_json_print_alloc/free_str`. Host build requires `"${COMPONENTS_DIR}/dsp_json/cjson"` added to `target_include_directories` in test/CMakeLists.txt (cjson/ is not under include/). 24 host-native tests.
- Git default branch is `master` on this machine (not `main`).
- The `sdkconfig` file (generated by `idf.py`) is intentionally gitignored — only `sdkconfig.defaults` is tracked.

## Partition Layout (4 MB flash)

| Name       | Type | Offset     | Size    | Purpose                        |
|------------|------|------------|---------|--------------------------------|
| nvs        | data | 0x9000     | 24 KB   | WiFi credentials, runtime NVS  |
| phy_init   | data | 0xF000     | 4 KB    | RF calibration data            |
| factory    | app  | 0x10000    | 1.5 MB  | Main firmware                  |
| dsp_certs  | data | 0x190000   | 64 KB   | X.509 certs and keys (DSP-201) |
| storage    | data | 0x1A0000   | 384 KB  | General sensor/app storage     |
