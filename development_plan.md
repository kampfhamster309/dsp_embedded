# DSP Embedded – Development Plan

## Overview

This plan organizes the implementation of DSP Embedded into milestones and Jira-sized tickets. Each ticket is scoped to be completable in 1–3 days by a single developer. Tickets within a milestone are roughly ordered by dependency.

Agent rules (AGENTS.md) apply throughout: git commit after every ticket, create tests for all new logic, update docs if behavior changes, and write human action items to `human_to_do.md`.

---

## Milestone 0 – Project Scaffolding

> Goal: A buildable, testable skeleton that all future work builds on.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-001 | Initialize ESP-IDF project | Create ESP-IDF project with CMakeLists.txt, sdkconfig defaults, and partition table for 4 MB flash. Initialize git repository. |
| ✅ DSP-002 | Define compile-time feature flags | Add `dsp_config.h` with all `CONFIG_DSP_*` flags and their defaults. Document each flag. |
| ✅ DSP-003 | Set up host-native build target | Configure a secondary CMake target that compiles the library for the host (x86/x64) to enable running unit tests without hardware. |
| ✅ DSP-004 | Integrate Unity test framework | Add Unity as a submodule or vendored copy. Create `test/` directory structure. Add a smoke-test that always passes to verify the test pipeline. |
| ✅ DSP-005 | Set up Docker-based EDC counterpart | Create `docker-compose.yml` that spins up a minimal EDC connector for integration testing. Document required EDC version in `human_to_do.md`. |
| ✅ DSP-006 | Create initial README and doc structure | Write `README.md` with build instructions, and create `doc/` directory with `deviation_log.md` and `compatibility_matrix.md` stubs. |

**Acceptance Criteria:**

- `cmake -S test -B test/build && cmake --build test/build` completes with zero errors and zero warnings on the host build.
- The smoke test binary runs and exits 0 with all tests reported as PASS.
- Every `CONFIG_DSP_*` flag defined in `dsp_config.h` has a documented default value and a comment explaining its effect; the file compiles with no warnings when each flag is forced to both 0 and 1 independently.
- The ESP-IDF project builds without errors using the `idf.py build` toolchain targeting ESP32-S3.
- `docker-compose up` starts the EDC connector and a `curl` health-check against its management API returns HTTP 200 within 60 seconds.
- `README.md` contains instructions that produce a passing host test run from a clean `git clone` without consulting any other file.

---

## Milestone 1 – Networking Foundation

> Goal: A working HTTP/1.1 server with TLS that fits within the memory budget.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-101 | Integrate mbedTLS with reduced config | Configure mbedTLS with a minimal `mbedtls_config.h` targeting ≤50 KB RAM. Disable unused cipher suites, retain AES-GCM, SHA-256, RSA, ECC. |
| ✅ DSP-102 | Implement HTTP/1.1 server skeleton | Wrap ESP-IDF's `esp_http_server` (or a lightweight alternative) in a `dsp_http` module. Expose `dsp_http_register_handler()` and `dsp_http_start()/stop()`. Target ≤20 KB RAM. |
| ✅ DSP-103 | Add TLS session ticket support | Implement TLS session resumption via session tickets behind `CONFIG_DSP_TLS_SESSION_TICKETS`. Unit-test the enable/disable paths. |
| ✅ DSP-104 | RAM usage instrumentation | Add a `dsp_mem_report()` utility that logs heap usage per component at startup. Used to verify budget compliance throughout development. |
| ✅ DSP-105 | WiFi connection manager | Implement `dsp_wifi` module: connect, reconnect on drop, report IP ready event. Credentials stored in NVS. Write unit tests for state machine logic on host. |

**Acceptance Criteria:**

- `dsp_mem_report()` output on a live ESP32-S3 shows the mbedTLS context consuming ≤50 KB heap after a TLS handshake completes.
- `dsp_mem_report()` output shows the HTTP server consuming ≤20 KB heap with no routes registered.
- On the host build, `dsp_http_start()` returns `ESP_FAIL` (no real server), `dsp_http_register_handler()` accepts valid arguments and returns `ESP_OK`, and `dsp_http_stop()` is safe to call regardless of running state; on the device build the full sequence `dsp_http_start()` → `dsp_http_register_handler()` → `dsp_http_stop()` → `dsp_http_start()` → `dsp_http_stop()` executes without error.
- The session-tickets-enabled binary (`CONFIG_DSP_TLS_SESSION_TICKETS=1`) and the disabled binary (`=0`) each build independently and pass their respective unit tests; neither binary references ticket-related symbols from the other path.
- The WiFi state machine transitions (DISCONNECTED → CONNECTING → CONNECTED → DISCONNECTED → CONNECTING → CONNECTED) are exercised by host unit tests without hardware and all pass.
- `dsp_mem_report()` is callable at startup on the device and does not crash or corrupt heap when called before or after `dsp_http_start()`.

---

## Milestone 2 – Identity and Security

> Goal: The node can authenticate itself and validate incoming JWT tokens using hardware crypto.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-201 | X.509 certificate provisioning | Implement `dsp_identity` module. Load DER-encoded device certificate and private key from flash (embedded via `COMPONENT_EMBED_FILES`). Expose `dsp_identity_get_cert()`. |
| ✅ DSP-202 | JWT validation – ES256 | Implement `dsp_jwt_validate()` for ES256 tokens using mbedTLS ECC hardware acceleration. Parse header, payload, and signature. Unit-test with known-good and known-bad tokens. |
| ✅ DSP-203 | JWT validation – RS256 | Extend `dsp_jwt_validate()` for RS256. Unit-test independently. |
| ✅ DSP-204 | Pre-Shared Key (PSK) mode | Implement PSK authentication path behind `CONFIG_DSP_ENABLE_PSK` (compile-time flag, default 0). When enabled, PSK identity and key are stored in a module-internal buffer and applied to a TLS context via `dsp_psk_apply()`. Unit-test PSK setup (init/deinit/set/get) and the disabled-path compilation guard. |
| ✅ DSP-205 | DAPS compatibility shim stub | Create `dsp_daps_shim` module behind `CONFIG_DSP_ENABLE_DAPS_SHIM`. Stub that proxies token requests to a configured gateway URL. Mark as not-yet-implemented with clear error return. Document in `deviation_log.md`. |

**Acceptance Criteria:**

- `dsp_identity_get_cert()` returns a non-null pointer and a positive byte-length on both host and device builds; the returned bytes parse as a valid DER-encoded X.509 certificate (verified by `mbedtls_x509_crt_parse_der` returning 0).
- `dsp_jwt_validate()` returns success for a known-good ES256 token fixture and a distinct, non-zero error code for each of: tampered signature, expired `exp` claim, and a token signed with the wrong key.
- `dsp_jwt_validate()` returns success for a known-good RS256 token fixture and the same failure cases as ES256; the RS256 tests pass independently of the ES256 tests.
- The codebase compiles cleanly with `CONFIG_DSP_ENABLE_PSK=0` (PSK excluded) and `=1` (PSK included); no PSK symbols appear in the `=0` binary (verified by `nm`).
- Calling the DAPS shim returns a documented, non-crash error code; `doc/deviation_log.md` contains an entry for this deviation with a non-empty rationale and interoperability-impact note.

---

## Milestone 3 – JSON and DSP Message Handling

> Goal: The node can parse and emit valid DSP-compliant messages without a runtime JSON-LD processor.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-301 | Integrate cJSON | Add cJSON as a vendored dependency. Wrap it in `dsp_json` with helpers for mandatory DSP fields. Verify it fits within 15 KB RAM target. |
| ✅ DSP-302 | Static JSON-LD context table | Define a C header `dsp_jsonld_ctx.h` containing all required DSP namespace prefixes and type mappings as compile-time string constants. Document the static-vs-dynamic trade-off in `deviation_log.md`. |
| ✅ DSP-303 | DSP message schema validators | Implement `dsp_msg_validate_catalog_request()`, `dsp_msg_validate_offer()`, `dsp_msg_validate_agreement()`, and `dsp_msg_validate_transfer_request()`. Unit-test each with valid and invalid JSON fixtures. |
| ✅ DSP-304 | DSP message builders | Implement builder functions for all outgoing message types (catalog response, agreement, transfer start, status responses). Unit-test serialization output against expected JSON fixtures. |

**Acceptance Criteria:**

- `dsp_mem_report()` shows cJSON peak heap usage ≤15 KB when parsing a 512-byte DSP message.
- All string constants in `dsp_jsonld_ctx.h` are `const char *` literals or `#define` macros; the file contains no `malloc`, `strdup`, or any runtime allocation; `deviation_log.md` has an entry documenting the static-vs-dynamic trade-off.
- Each validator (`dsp_msg_validate_catalog_request`, `dsp_msg_validate_offer`, `dsp_msg_validate_agreement`, `dsp_msg_validate_transfer_request`) returns success for its valid fixture and a distinct error code for each of: missing required field, wrong `@type` value, and malformed (non-parseable) JSON; all four validators are tested independently.
- Each builder function produces a JSON string that (a) parses without error through `dsp_json_parse`, (b) contains the correct `@context` matching `DSP_JSONLD_CONTEXT_URL`, (c) contains the correct `@type` value, and (d) contains all fields required by the corresponding DSP message type; unit tests verify all four properties for every builder.

---

## Milestone 4 – DSP State Machines

> Goal: Core protocol logic for catalog, negotiation, and transfer flows is implemented and tested.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-401 | Catalog state machine | Implement `dsp_catalog` module: load static catalog from flash, serve `GET /catalog`. Unit-test catalog serialization. |
| ✅ DSP-402 | Negotiation state machine – offer receipt | Implement state transitions for receiving `POST /negotiations/offers`. States: IDLE → OFFERED. Persist state in RTC memory. Unit-test transitions. |
| ✅ DSP-403 | Negotiation state machine – agreement | Implement `POST /negotiations/{id}/agree` and `GET /negotiations/{id}`. States: OFFERED → AGREED. Unit-test. |
| ✅ DSP-404 | Negotiation state machine – termination | Implement `POST /negotiations/terminate` behind `CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE` (MAY). States: any → TERMINATED. Unit-test. |
| ✅ DSP-405 | Transfer state machine – start | Implement `POST /transfers/start`. States: AGREED → TRANSFERRING. Trigger data delivery to Core 1 via FreeRTOS queue. Unit-test. |
| ✅ DSP-406 | Transfer state machine – status and completion | Implement `GET /transfers/{id}`. States: TRANSFERRING → COMPLETED / FAILED. Unit-test all terminal transitions. |
| ✅ DSP-407 | Dynamic catalog request endpoint | Implement `POST /catalog/request` behind `CONFIG_DSP_ENABLE_CATALOG_REQUEST` (SHOULD). Unit-test enable/disable paths. |

**Acceptance Criteria:**

- `dsp_catalog_get_json()` produces a document that parses successfully and contains the correct `@context`, `@type` = `dcat:Catalog`, a non-empty `dcat:dataset` array, and a non-empty `dct:title`; the host unit tests verify each field independently.
- `POST /negotiations/offers` with a valid `ContractRequestMessage` creates a negotiation slot; `dsp_neg_get_state()` for that slot returns OFFERED immediately after; a second offer with the same `processId` does not create a duplicate slot.
- `POST /negotiations/{id}/agree` transitions an OFFERED slot to AGREED; `GET /negotiations/{id}` returns a response whose `dsp:state` field equals `dspace:AGREED`; calling agree on a non-OFFERED slot returns a non-OK error code.
- With `CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=0`, `POST /negotiations/terminate` is not registered and calling `dsp_neg_register_handlers()` does not register it; with `=1`, a separate test binary verifies the handler is registered and transitions OFFERED and AGREED slots to TERMINATED; TERMINATED is a terminal state (further events produce no state change).
- `POST /transfers/start` with an AGREED negotiation slot creates a transfer in TRANSFERRING state; the notify callback fires exactly once on the INITIAL → TRANSFERRING transition and does not fire again if START is applied to an already-TRANSFERRING slot.
- `GET /transfers/{id}` returns a `TransferStartMessage` for TRANSFERRING, a `TransferCompletionMessage` for COMPLETED, and a `dspace:Error` response for FAILED; COMPLETED and FAILED are terminal (COMPLETE and FAIL events applied to them produce no further state change); all transitions are covered by host unit tests.
- With `CONFIG_DSP_ENABLE_CATALOG_REQUEST=0`, `dsp_catalog_register_request_handler()` returns `ESP_ERR_NOT_SUPPORTED` after init; with `=1`, a separate test binary verifies it returns `ESP_OK` after init and `ESP_ERR_INVALID_STATE` before init.

---

## Milestone 5 – Dual-Core Integration

> Goal: Protocol (Core 0) and application (Core 1) communicate correctly via shared memory structures.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-501 | Shared memory layout definition | Define `dsp_shared.h`: ring buffer struct, FreeRTOS queue handles, and mutex guards for all shared state. |
| ✅ DSP-502 | Core 0 task setup | Pin `dsp_protocol_task` to Core 0. Initialize HTTP server, DSP state machines, and JWT validator within this task. Set stack size to fit memory budget. |
| ✅ DSP-503 | Core 1 task setup | Pin `dsp_application_task` to Core 1. Implement stub sensor polling loop and ADC read. Write data into ring buffer. Set stack size. |
| ✅ DSP-504 | Ring buffer producer/consumer | Implement lock-free ring buffer (or FreeRTOS stream buffer) between Core 1 (producer) and Core 0 (consumer for transfers). Unit-test on host. |
| ✅ DSP-505 | RTC memory persistence | Implement `dsp_rtc_state` module: serialize/deserialize negotiation and transfer state to/from RTC memory. Unit-test save/restore round-trip. |

**Acceptance Criteria:**

- `dsp_shared.h` compiles without warnings on both the host (x86-64) and ESP32-S3 toolchains; all struct sizes are verified with `static_assert` or equivalent to be deterministic regardless of pointer width.
- `dsp_protocol_task` is confirmed pinned to Core 0 by `xTaskGetCoreID()` at startup; FreeRTOS task stats show it does not hold any mutex for longer than 10 ms continuously during a full negotiation + transfer cycle.
- `dsp_application_task` is confirmed pinned to Core 1; after one acquisition cycle completes, the ring buffer contains at least one unread sample; Core 0 is not blocked during the write.
- Host unit tests verify the ring buffer for: exactly 1 item (produce then consume), capacity items without loss, and overflow behavior (item at capacity+1 is either dropped or blocks as documented); the chosen overflow policy is documented in a code comment.
- A save/restore round-trip through `dsp_rtc_state_save()` / `dsp_rtc_state_restore()` produces byte-identical negotiation and transfer state; host unit tests verify this for an empty table, a single active slot, and a fully-populated table (max slots).

---

## Milestone 6 – Power Management

> Goal: The node correctly enters and wakes from Deep Sleep, preserving protocol state.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-601 | Deep Sleep entry logic | Implement `dsp_power_enter_deep_sleep()`: flush pending state to RTC memory, close HTTP server gracefully, trigger esp_deep_sleep_start(). Behind `CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX`. |
| ✅ DSP-602 | Wake-up and state restoration | Implement wake-up handler: detect wake cause, restore state from RTC memory, reconnect WiFi, resume HTTP server. Unit-test state restore logic on host. |
| ✅ DSP-603 | Watchdog-secured power-save mode | Implement power-save idle loop between data acquisition cycles using `esp_pm` light-sleep. Configure watchdog timer. Test that watchdog fires on simulated hang. |
| ✅ DSP-604 | Power budget measurement | Document procedure in `doc/power_measurement.md` for measuring current at each operating mode (active WiFi, light sleep, deep sleep). Add to `human_to_do.md` (requires hardware + ammeter). |

**Acceptance Criteria:**

- On the host build, calling `dsp_power_enter_deep_sleep()` with an active negotiation slot flushes that slot's state to RTC memory before the sleep entry point is reached; a subsequent `dsp_rtc_state_restore()` recovers the slot with identical state (verified by host unit test with a mock sleep entry function).
- With `CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=0`, no deep-sleep-related symbols from the `dsp_power` module appear in the compiled output (verified by `nm` on the device binary).
- A host unit test simulates wake-from-deep-sleep (ESP_SLEEP_WAKEUP_TIMER cause) with a saved negotiation slot and verifies that after `dsp_rtc_state_restore()` the slot's state and process ID are unchanged.
- A test on the device verifies that when the acquisition loop is intentionally blocked (busy-wait without feeding the watchdog), the watchdog resets the system within the configured timeout; the watchdog does not fire during a normal acquisition cycle.
- `doc/power_measurement.md` exists with a documented measurement procedure for each operating mode (active WiFi, light sleep, deep sleep) and expected current ranges derived from the ESP32-S3 datasheet.

---

## Milestone 7 – Integration Testing and Interoperability

> Goal: The node exchanges valid DSP messages with a real EDC connector and all deviations are documented.

| Ticket | Title | Description |
|--------|-------|-------------|
| ✅ DSP-701 | Integration test: catalog fetch | Write an integration test that starts the EDC Docker container, queries `GET /catalog` on the DSP Embedded node (running on host build), and validates the response. |
| ✅ DSP-702 | Integration test: negotiation flow | Integration test covering the full offer → agree sequence against the EDC connector. |
| ✅ DSP-703 | Integration test: transfer flow | Integration test covering transfer start → status → completion against the EDC connector. |
| ✅ DSP-704 | Fill compatibility matrix | Run integration tests against each supported EDC version. Document results in `doc/compatibility_matrix.md`. |
| ✅ DSP-705 | Finalize deviation log | Review all `deviation_log.md` entries. Ensure every deviation from the DSP spec has a rationale and a note on interoperability impact. |
| ✅ DSP-706 | RAM budget audit | Run `dsp_mem_report()` in a full integration scenario. Verify all components are within their budgets from Milestone 1. Fix any overruns. |

**Acceptance Criteria:**

- The catalog integration test (`DSP-701`) completes end-to-end in under 30 seconds on a developer machine; the EDC connector's response to `GET /catalog` parses as valid JSON-LD with the correct `@type` = `dcat:Catalog`.
- The negotiation integration test (`DSP-702`) drives the full offer → agree sequence; the final `GET /negotiations/{id}` response from the DSP Embedded node contains `dsp:state` = `dspace:AGREED` and the EDC connector reaches its equivalent AGREED state without error.
- The transfer integration test (`DSP-703`) drives start → status → completion; both the DSP Embedded node and the EDC connector reach COMPLETED state; the test exits 0.
- `doc/compatibility_matrix.md` contains one row per tested EDC version; each row records the result (PASS / FAIL / PARTIAL) for each of the eight DSP endpoints defined in `target.md`; no cell is left blank.
- `doc/deviation_log.md` contains no entry with an empty rationale or empty interoperability-impact field; every entry references the DSP spec section it deviates from.
- `dsp_mem_report()` output captured during a full integration run shows: TLS ≤50 KB, HTTP server ≤20 KB, JSON parser ≤15 KB, JWT processing ≤25 KB, DSP state machines ≤20 KB; any component that exceeds its budget fails the test.

---

## Milestone 8 – Hardening and Release

> Goal: The implementation is stable, documented, and ready for use as a reference.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-801 ✅ | Full unit test coverage pass | Review test coverage across all modules. Add missing tests for untested branches identified during integration. |
| DSP-802 ✅ | Static analysis pass | Run `cppcheck` or `clang-tidy` on the codebase. Fix all warnings of severity ≥ medium. |
| DSP-803 | Flash-time provisioning tooling | Write a Python helper script `tools/provision.py` that flashes device certificate and PSK into NVS partitions. Document in `human_to_do.md` (requires connected hardware). |
| DSP-804 | PSRAM support | Enable and test operation with 8 MB PSRAM behind `CONFIG_DSP_PSRAM_ENABLE`. Verify no heap corruption at the boundary. |
| DSP-805 | Final README and documentation update | Update `README.md` with complete build, flash, and test instructions. Ensure `doc/` is complete and all links resolve. |
| DSP-806 | Tag v0.1.0 release | Create git tag `v0.1.0`. Write a short release note summarizing implemented endpoints, known limitations, and tested EDC versions. |

**Acceptance Criteria:**

- All host unit test binaries pass with zero failures and zero ignored tests; every public function in every `components/*/` module is exercised by at least one test that reaches the non-trivial branch (verified by manual coverage review or `gcov`).
- `cppcheck` (or `clang-tidy`) reports zero findings of severity ≥ medium on all production source files (the `test/` directory and vendored `cjson/` are excluded); the clean invocation command is documented in `README.md`.
- `python tools/provision.py --help` exits 0 and prints usage; `python tools/provision.py --dry-run` prints the NVS partition keys and values it would write without requiring connected hardware or an NVS binary.
- The firmware builds cleanly with `CONFIG_DSP_PSRAM_ENABLE=1`; a full negotiation + transfer cycle on hardware with PSRAM enabled produces no heap corruption as reported by the ESP-IDF heap integrity check or AddressSanitizer on the host build.
- `README.md` build, flash, and test instructions can be executed from a clean `git clone` to a passing test run without consulting any file outside `README.md`; all `doc/` links referenced in `README.md` resolve to existing files.
- Git tag `v0.1.0` exists on the main branch; the associated release note lists every implemented DSP endpoint with its method and path, every known limitation with a brief description, and every EDC version tested with its result.

---

## Dependency Summary

```
M0 (Scaffolding)
  └─> M1 (Networking)
        └─> M2 (Identity/Security)
              └─> M3 (JSON/Messages)
                    └─> M4 (State Machines)
                          └─> M5 (Dual-Core)
                          └─> M6 (Power Management)
                                └─> M7 (Integration Testing)
                                      └─> M8 (Hardening/Release)
```

M5 and M6 can be developed in parallel after M4.

---

## Notes

- **Hardware required from M5 onwards**: An ESP32-S3 development board is needed for on-device testing. All earlier milestones can be developed and tested on the host build target.
- **EDC Docker environment** (DSP-005) should be set up in M0 but is only exercised in M7.
- Tickets marked with `CONFIG_DSP_*` guards are optional features and can be deferred without blocking the critical path.
