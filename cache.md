# DSP Embedded – Agent Cache

## Current State

**Last completed ticket:** DSP-102 – Implement HTTP/1.1 server skeleton (dsp_http)
**Next ticket:** DSP-103 – Add TLS session ticket support

## Project Structure

```
dsp_embedded/
├── main/main.c               # Minimal app_main entry point
├── main/CMakeLists.txt
├── components/
│   ├── dsp_config/           # Feature flags: Kconfig + dsp_config.h
│   ├── dsp_mbedtls/         # TLS server context (DSP-101)
│   └── dsp_http/            # HTTP/1.1 server skeleton (DSP-102)
├── test/
│   ├── CMakeLists.txt        # Host-native standalone CMake project
│   ├── test_main.c           # Unity runner: UNITY_BEGIN/END + RUN_TEST calls
│   ├── test_smoke.c          # 15 smoke tests (pipeline + dsp_config defaults)
│   ├── test_dsp_tls.c        # 9 host-native tests for dsp_tls context (DSP-101)
│   ├── test_dsp_http.c       # 15 host-native tests for dsp_http (DSP-102)
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
- **dsp_http**: wraps `esp_http_server`; exposes `dsp_http_start(port)`, `dsp_http_stop()`, `dsp_http_register_handler(uri, method, fn)`, `dsp_http_is_running()`. Handler bridge uses `httpd_req_t::user_ctx` so a single `bridge_handler()` serves all routes. Server config: stack=4096, max_open_sockets=4, lru_purge_enable=true. Host `#else` stub preserves route table in static array (no real server) for test verification.
- **dsp_http method mapping**: `DSP_HTTP_{GET,POST,PUT,DELETE}` are DSP-internal enum values (0–3); mapped to ESP-IDF `HTTP_GET=1, HTTP_POST=3, HTTP_PUT=4, HTTP_DELETE=0` via `map_method()` in the ESP_PLATFORM block.
- **dsp_tls host stubs**: `dsp_tls.c` has an `#else` branch (no ESP_PLATFORM) providing `dsp_tls_server_init` (returns ESP_FAIL) and `dsp_tls_server_deinit`. `dsp_tls.h` requires `<stdbool.h>` and `<stddef.h>` (host-compatibility fixes applied in DSP-101).
- `test/build/` is gitignored; `test/test_main.c` is tracked (replaced by Unity runner in DSP-004)
- **Preprocessor string comparisons are invalid in `#if`**: `CONFIG_DSP_DAPS_GATEWAY_URL[0] == '\0'` caused a build error — string literals cannot be used in preprocessor expressions. Removed; Kconfig `depends on` handles the constraint instead.
- Two sdkconfig.defaults keys were renamed in IDF v5.x: `CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240` → `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240` and `CONFIG_ESP32S3_SPIRAM_SUPPORT` → `CONFIG_SPIRAM`. Fixed.
- `human_to_do.md` is excluded from git via `.gitignore` (user preference).
- `cache.md` is tracked in git (it's part of the agent workflow).
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
