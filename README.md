# DSP Embedded

A lean [Dataspace Protocol (DSP)](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol) reference implementation in C for resource-constrained embedded systems, targeting the ESP32-S3.

The node acts as a **data provider** only: it serves a static asset catalog, drives a contract negotiation state machine, and streams data from the onboard sensor pipeline to a DSP consumer — all on a microcontroller with 512 KB SRAM.

See [target.md](target.md) for full design constraints and [development_plan.md](development_plan.md) for the implementation roadmap.

---

## Status

All Milestone 0–8 tickets implemented and verified (2026-03-14).

| Component | Status |
|---|---|
| Host-native unit tests | 250+ tests, 0 failures |
| Integration tests (Python mock counterpart) | 52/52 pass |
| Static analysis (cppcheck) | 0 findings at severity ≥ medium |
| RAM budget | All 5 components within budget (see [doc/ram_budget.md](doc/ram_budget.md)) |
| PSRAM (8 MB OPI) | Verified on device — `CONFIG_DSP_PSRAM_ENABLE=y` |

---

## Requirements

| Tool | Minimum version | Purpose |
|------|----------------|---------|
| [ESP-IDF](https://github.com/espressif/esp-idf) | v5.5 | Firmware build + flash |
| CMake | 3.16 | Host-native test build |
| GCC | 13 | Host-native test build |
| Python | 3.10 | Integration tests, provisioning tool |
| OpenSSL | any recent | Device certificate generation |
| Docker + Compose | any recent | DSP counterpart for integration tests |

**Hardware:** ESP32-S3 development board (4 MB flash minimum). Optional: 8 MB OPI PSRAM.

---

## Clone and Initialise

```sh
git clone <repo-url> dsp_embedded
cd dsp_embedded

# Unity test framework is a git submodule — must be initialised before building tests
git submodule update --init test/unity
```

---

## Host-Native Unit Tests (no hardware required)

The entire library compiles for the host (x86-64 / arm64) using CMake. No ESP-IDF or connected hardware is needed.

```sh
cd test
cmake -B build
cmake --build build

# Run all test binaries
ctest --test-dir build --output-on-failure
```

Expected output (abbreviated):

```
Test #1: dsp_test_runner           Passed    0.02 sec
Test #2: dsp_test_no_tickets       Passed    0.01 sec
Test #3: dsp_test_deep_sleep_on    Passed    0.01 sec
Test #4: dsp_test_psk_on           Passed    0.01 sec
Test #5: dsp_test_terminate_on     Passed    0.01 sec
Test #6: dsp_test_catalog_req_on   Passed    0.01 sec

100% tests passed, 0 tests failed
```

To run a single binary directly:

```sh
./test/build/dsp_test_runner
```

---

## Firmware Build and Flash

### 1. Set up ESP-IDF

```sh
source ~/esp/esp-idf/export.sh    # once per shell session
```

If ESP-IDF is not installed, see the [ESP-IDF Get Started guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/).

### 2. Generate device certificate and key

The firmware embeds a device certificate and private key at build time. These files are gitignored and must be generated locally before building.

```sh
bash tools/gen_dev_certs.sh
```

This creates (gitignored):
- `components/dsp_identity/certs/device_cert.der` — ECDSA P-256 self-signed certificate, 10-year validity
- `components/dsp_identity/certs/device_key.der` — private key

> For production deployments each device needs a unique certificate signed by your CA. See `tools/provision.py` and [human_to_do.md](human_to_do.md) for the production provisioning workflow.

### 3. Set WiFi credentials

Create `sdkconfig.wifi` (gitignored — never commit credentials) with your network's SSID and password:

```ini
CONFIG_DSP_WIFI_SSID="YourNetworkSSID"
CONFIG_DSP_WIFI_PASSWORD="YourNetworkPassword"
CONFIG_DSP_WIFI_PROVISION=y
```

### 4. Build

```sh
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi" build
```

To also enable 8 MB OPI PSRAM (ESP32-S3 with AP_3v3 PSRAM package):

```sh
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.psram" build
```

> If you previously built without PSRAM and are adding it now, delete the stale `sdkconfig` first so the new defaults are picked up:
>
> ```sh
> rm -f sdkconfig
> idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.psram" reconfigure
> idf.py build
> ```

### 5. Flash

```sh
idf.py -p /dev/ttyACM1 flash
```

Replace `/dev/ttyACM1` with the serial port of your board (e.g. `/dev/ttyUSB0` on some systems, `COM3` on Windows).

After flashing, the device prints its IP address to the serial console:

```
I (dsp_wifi): connected – IP: 192.168.x.x
```

Note this address — it is the `--provider-url` for integration tests.

---

## Integration Tests

Integration tests run pytest against the live device and a Python/FastAPI DSP consumer mock (`docker/dsp-counterpart/`).

### 1. Start the DSP counterpart

```sh
cd docker
docker compose up -d
cd ..
```

The counterpart is ready at `http://localhost:18000`. Verify with:

```sh
curl http://localhost:18000/health
```

### 2. Create the Python virtual environment (first time)

```sh
python3 -m venv integration/.venv
integration/.venv/bin/pip install -r integration/requirements.txt
```

### 3. Run the test suite

```sh
integration/.venv/bin/pytest integration/ \
  --provider-url http://<device-ip> \
  --counterpart-url http://localhost:18000 \
  -v
```

Expected output:

```
integration/test_701_catalog.py::...     PASSED  (14 tests)
integration/test_702_negotiation.py::... PASSED  (18 tests)
integration/test_703_transfer.py::...    PASSED  (20 tests)

52 passed in 3.81s
```

> **Note:** The device slot tables fill across the test suite and are not freed during a session. **Reset the board before re-running the full suite** (`idf.py -p /dev/ttyACM1 flash` or press the reset button).

---

## Static Analysis

Run `cppcheck` on all component and test sources. Requires `cppcheck` ≥ 2.13.

```sh
cppcheck --enable=warning,performance,portability \
         --suppress=missingIncludeSystem \
         --std=c17 --error-exitcode=1 \
         -DDSP_HOST_BUILD=1 --platform=unix32 \
         components/*/dsp_*.c test/test_*.c
```

Expected: `exit code 0` (zero findings at severity ≥ medium). See [doc/static_analysis.md](doc/static_analysis.md) for the full report and a description of two known style-level false positives.

---

## Provisioning (Production)

For production deployments, each device should be provisioned with a unique certificate signed by your CA and written to the `dsp_certs` NVS partition (not embedded in the firmware binary).

```sh
python3 tools/provision.py \
    --cert /path/to/device_cert.der \
    --key  /path/to/device_key.der \
    --port /dev/ttyACM1
```

Dry-run (no flash):

```sh
python3 tools/provision.py \
    --cert /path/to/device_cert.der \
    --key  /path/to/device_key.der \
    --dry-run --output /tmp/dsp_certs.bin
```

```sh
python3 tools/provision.py --help
```

See [human_to_do.md](human_to_do.md) for the full production provisioning procedure.

---

## Optional Build Configurations

### Power Save – Light Sleep

Reduces average current from ~120 mA to ~20–45 mA by entering CPU light-sleep between acquisition cycles:

```sh
printf 'CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP=y\nCONFIG_PM_ENABLE=y\nCONFIG_FREERTOS_USE_TICKLESS_IDLE=y\n' \
  > sdkconfig.light_sleep

idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.light_sleep" build
idf.py -p /dev/ttyACM1 flash
```

### Power Save – Deep Sleep Between Transfers

Reduces sleep-phase current to ~7–15 µA by powering down between transfers:

```sh
printf 'CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=y\n' > sdkconfig.deep_sleep

idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.deep_sleep" build
idf.py -p /dev/ttyACM1 flash
```

See [doc/power_measurement.md](doc/power_measurement.md) for measurement procedure and expected current ranges.

---

## Implemented DSP Endpoints

DSP specification version: **DSP v2024/1** (`https://w3id.org/dspace/2024/1/context.json`)

| Method | Path | Priority | Status |
|--------|------|----------|--------|
| GET | `/catalog` | MUST | Implemented |
| POST | `/negotiations/offers` | MUST | Implemented |
| POST | `/negotiations/{id}/agree` | MUST | Implemented |
| GET | `/negotiations/{id}` | MUST | Implemented |
| POST | `/transfers/start` | MUST | Implemented |
| GET | `/transfers/{id}` | MUST | Implemented |
| POST | `/transfers/{id}/complete` | MUST | Implemented |
| POST | `/catalog/request` | SHOULD | Optional (`CONFIG_DSP_ENABLE_CATALOG_REQUEST=y`) |
| POST | `/negotiations/terminate` | MAY | Optional (`CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=y`) |

All deviations from the specification are documented in [doc/deviation_log.md](doc/deviation_log.md).
Interoperability test results are in [doc/compatibility_matrix.md](doc/compatibility_matrix.md).

---

## Project Structure

```
dsp_embedded/
├── main/                     # app_main entry point
├── components/               # DSP Embedded library (one component per feature)
│   ├── dsp_config/           # Compile-time feature flags (Kconfig + dsp_config.h)
│   ├── dsp_mbedtls/          # TLS server context
│   ├── dsp_http/             # HTTP/1.1 server wrapper
│   ├── dsp_mem/              # Heap instrumentation (dsp_mem_report)
│   ├── dsp_wifi/             # WiFi connection manager + state machine
│   ├── dsp_identity/         # X.509 cert+key loading (EMBED_FILES)
│   ├── dsp_jwt/              # JWT validation ES256 + RS256 via mbedTLS
│   ├── dsp_psk/              # Pre-Shared Key credential management
│   ├── dsp_daps/             # DAPS gateway shim stub
│   ├── dsp_json/             # cJSON v1.7.19 wrapper + DSP JSON-LD helpers
│   ├── dsp_jsonld/           # Header-only: DSP namespace and type constants
│   ├── dsp_msg/              # DSP message schema validators
│   ├── dsp_build/            # DSP outgoing message builders
│   ├── dsp_catalog/          # Static catalog + GET /catalog handler
│   ├── dsp_neg/              # Negotiation state machine + HTTP handlers
│   ├── dsp_xfer/             # Transfer state machine + HTTP handlers
│   ├── dsp_shared/           # Shared memory: ring buffer + FreeRTOS handles
│   ├── dsp_protocol/         # Core 0 task: TLS + HTTP + state machines
│   ├── dsp_application/      # Core 1 task: ADC stub polling → ring buffer
│   ├── dsp_rtc_state/        # RTC memory persistence across deep sleep
│   └── dsp_power/            # Deep sleep entry/wakeup + power management
├── test/                     # Host-native unit tests (Unity, CMake)
│   ├── CMakeLists.txt
│   ├── test_main.c           # Unity runner
│   ├── test_*.c              # Per-component test files
│   ├── unity/                # Unity submodule (ThrowTheSwitch/Unity v2.6.0)
│   └── stubs/                # ESP-IDF header shims for host builds
├── integration/              # pytest integration tests (against live device)
│   ├── test_701_catalog.py   # 14 tests: GET /catalog
│   ├── test_702_negotiation.py # 18 tests: offer → agree flow
│   └── test_703_transfer.py  # 20 tests: transfer start → complete
├── docker/
│   ├── docker-compose.yml    # DSP consumer mock on port 18000
│   └── dsp-counterpart/      # Python/FastAPI DSP consumer implementation
├── doc/
│   ├── deviation_log.md      # All spec deviations with rationale and impact
│   ├── compatibility_matrix.md # Interoperability results per counterpart
│   ├── ram_budget.md         # Heap audit: measured vs budget per component
│   ├── static_analysis.md    # cppcheck results (DSP-802)
│   └── power_measurement.md  # Current measurement procedure and expected ranges
├── tools/
│   ├── gen_dev_certs.sh      # Generate development device_cert.der + device_key.der
│   └── provision.py          # Production NVS provisioning script
├── partitions.csv            # Flash partition table (NVS, factory, dsp_certs, storage)
├── sdkconfig.defaults        # ESP32-S3 Kconfig defaults
├── sdkconfig.psram           # PSRAM overlay: 8 MB OPI PSRAM (CONFIG_DSP_PSRAM_ENABLE=y)
└── CMakeLists.txt
```

---

## Documentation

| File | Contents |
|------|----------|
| [target.md](target.md) | Project description, hardware target, design constraints |
| [development_plan.md](development_plan.md) | Milestone plan and ticket status |
| [doc/deviation_log.md](doc/deviation_log.md) | Spec deviations with rationale (DEV-001–DEV-007) |
| [doc/compatibility_matrix.md](doc/compatibility_matrix.md) | Integration test results per counterpart |
| [doc/ram_budget.md](doc/ram_budget.md) | Heap measurements vs component budgets |
| [doc/static_analysis.md](doc/static_analysis.md) | cppcheck report |
| [doc/power_measurement.md](doc/power_measurement.md) | Power measurement procedure and expected values |

---

## License

Apache License 2.0 — see [LICENSE](LICENSE).
