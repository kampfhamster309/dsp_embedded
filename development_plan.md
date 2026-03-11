# DSP Embedded – Development Plan

## Overview

This plan organizes the implementation of DSP Embedded into milestones and Jira-sized tickets. Each ticket is scoped to be completable in 1–3 days by a single developer. Tickets within a milestone are roughly ordered by dependency.

Agent rules (AGENTS.md) apply throughout: git commit after every ticket, create tests for all new logic, update docs if behavior changes, and write human action items to `human_to_do.md`.

---

## Milestone 0 – Project Scaffolding

> Goal: A buildable, testable skeleton that all future work builds on.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-001 | Initialize ESP-IDF project | Create ESP-IDF project with CMakeLists.txt, sdkconfig defaults, and partition table for 4 MB flash. Initialize git repository. |
| DSP-002 | Define compile-time feature flags | Add `dsp_config.h` with all `CONFIG_DSP_*` flags and their defaults. Document each flag. |
| DSP-003 | Set up host-native build target | Configure a secondary CMake target that compiles the library for the host (x86/x64) to enable running unit tests without hardware. |
| DSP-004 | Integrate Unity test framework | Add Unity as a submodule or vendored copy. Create `test/` directory structure. Add a smoke-test that always passes to verify the test pipeline. |
| DSP-005 | Set up Docker-based EDC counterpart | Create `docker-compose.yml` that spins up a minimal EDC connector for integration testing. Document required EDC version in `human_to_do.md`. |
| DSP-006 | Create initial README and doc structure | Write `README.md` with build instructions, and create `doc/` directory with `deviation_log.md` and `compatibility_matrix.md` stubs. |

---

## Milestone 1 – Networking Foundation

> Goal: A working HTTP/1.1 server with TLS that fits within the memory budget.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-101 | Integrate mbedTLS with reduced config | Configure mbedTLS with a minimal `mbedtls_config.h` targeting ≤50 KB RAM. Disable unused cipher suites, retain AES-GCM, SHA-256, RSA, ECC. |
| DSP-102 | Implement HTTP/1.1 server skeleton | Wrap ESP-IDF's `esp_http_server` (or a lightweight alternative) in a `dsp_http` module. Expose `dsp_http_register_handler()` and `dsp_http_start()/stop()`. Target ≤20 KB RAM. |
| DSP-103 | Add TLS session ticket support | Implement TLS session resumption via session tickets behind `CONFIG_DSP_TLS_SESSION_TICKETS`. Unit-test the enable/disable paths. |
| DSP-104 | RAM usage instrumentation | Add a `dsp_mem_report()` utility that logs heap usage per component at startup. Used to verify budget compliance throughout development. |
| DSP-105 | WiFi connection manager | Implement `dsp_wifi` module: connect, reconnect on drop, report IP ready event. Credentials stored in NVS. Write unit tests for state machine logic on host. |

---

## Milestone 2 – Identity and Security

> Goal: The node can authenticate itself and validate incoming JWT tokens using hardware crypto.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-201 | X.509 certificate provisioning | Implement `dsp_identity` module. Load DER-encoded device certificate and private key from flash (embedded via `COMPONENT_EMBED_FILES`). Expose `dsp_identity_get_cert()`. |
| DSP-202 | JWT validation – ES256 | Implement `dsp_jwt_validate()` for ES256 tokens using mbedTLS ECC hardware acceleration. Parse header, payload, and signature. Unit-test with known-good and known-bad tokens. |
| DSP-203 | JWT validation – RS256 | Extend `dsp_jwt_validate()` for RS256. Unit-test independently. |
| DSP-204 | Pre-Shared Key (PSK) mode | Implement PSK authentication path behind `CONFIG_DSP_PSRAM_ENABLE` (compile-time). PSK stored in NVS. Unit-test PSK setup and TLS PSK handshake mock. |
| DSP-205 | DAPS compatibility shim stub | Create `dsp_daps_shim` module behind `CONFIG_DSP_ENABLE_DAPS_SHIM`. Stub that proxies token requests to a configured gateway URL. Mark as not-yet-implemented with clear error return. Document in `deviation_log.md`. |

---

## Milestone 3 – JSON and DSP Message Handling

> Goal: The node can parse and emit valid DSP-compliant JSON messages without a runtime JSON-LD processor.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-301 | Integrate cJSON | Add cJSON as a vendored dependency. Wrap it in `dsp_json` with helpers for mandatory DSP fields. Verify it fits within 15 KB RAM target. |
| DSP-302 | Static JSON-LD context table | Define a C header `dsp_jsonld_ctx.h` containing all required DSP namespace prefixes and type mappings as compile-time string constants. Document the static-vs-dynamic trade-off in `deviation_log.md`. |
| DSP-303 | DSP message schema validators | Implement `dsp_msg_validate_catalog_request()`, `dsp_msg_validate_offer()`, `dsp_msg_validate_agreement()`, and `dsp_msg_validate_transfer_request()`. Unit-test each with valid and invalid JSON fixtures. |
| DSP-304 | DSP message builders | Implement builder functions for all outgoing message types (catalog response, agreement, transfer start, status responses). Unit-test serialization output against expected JSON fixtures. |

---

## Milestone 4 – DSP State Machines

> Goal: Core protocol logic for catalog, negotiation, and transfer flows is implemented and tested.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-401 | Catalog state machine | Implement `dsp_catalog` module: load static catalog from flash, serve `GET /catalog`. Unit-test catalog serialization. |
| DSP-402 | Negotiation state machine – offer receipt | Implement state transitions for receiving `POST /negotiations/offers`. States: IDLE → OFFERED. Persist state in RTC memory. Unit-test transitions. |
| DSP-403 | Negotiation state machine – agreement | Implement `POST /negotiations/{id}/agree` and `GET /negotiations/{id}`. States: OFFERED → AGREED. Unit-test. |
| DSP-404 | Negotiation state machine – termination | Implement `POST /negotiations/terminate` behind `CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE` (MAY). States: any → TERMINATED. Unit-test. |
| DSP-405 | Transfer state machine – start | Implement `POST /transfers/start`. States: AGREED → TRANSFERRING. Trigger data delivery to Core 1 via FreeRTOS queue. Unit-test. |
| DSP-406 | Transfer state machine – status and completion | Implement `GET /transfers/{id}`. States: TRANSFERRING → COMPLETED / FAILED. Unit-test all terminal transitions. |
| DSP-407 | Dynamic catalog request endpoint | Implement `POST /catalog/request` behind `CONFIG_DSP_ENABLE_CATALOG_REQUEST` (SHOULD). Unit-test enable/disable paths. |

---

## Milestone 5 – Dual-Core Integration

> Goal: Protocol (Core 0) and application (Core 1) communicate correctly via shared memory structures.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-501 | Shared memory layout definition | Define `dsp_shared.h`: ring buffer struct, FreeRTOS queue handles, and mutex guards for all shared state. |
| DSP-502 | Core 0 task setup | Pin `dsp_protocol_task` to Core 0. Initialize HTTP server, DSP state machines, and JWT validator within this task. Set stack size to fit memory budget. |
| DSP-503 | Core 1 task setup | Pin `dsp_application_task` to Core 1. Implement stub sensor polling loop and ADC read. Write data into ring buffer. Set stack size. |
| DSP-504 | Ring buffer producer/consumer | Implement lock-free ring buffer (or FreeRTOS stream buffer) between Core 1 (producer) and Core 0 (consumer for transfers). Unit-test on host. |
| DSP-505 | RTC memory persistence | Implement `dsp_rtc_state` module: serialize/deserialize negotiation and transfer state to/from RTC memory. Unit-test save/restore round-trip. |

---

## Milestone 6 – Power Management

> Goal: The node correctly enters and wakes from Deep Sleep, preserving protocol state.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-601 | Deep Sleep entry logic | Implement `dsp_power_enter_deep_sleep()`: flush pending state to RTC memory, close HTTP server gracefully, trigger esp_deep_sleep_start(). Behind `CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX`. |
| DSP-602 | Wake-up and state restoration | Implement wake-up handler: detect wake cause, restore state from RTC memory, reconnect WiFi, resume HTTP server. Unit-test state restore logic on host. |
| DSP-603 | Watchdog-secured power-save mode | Implement power-save idle loop between data acquisition cycles using `esp_pm` light-sleep. Configure watchdog timer. Test that watchdog fires on simulated hang. |
| DSP-604 | Power budget measurement | Document procedure in `doc/power_measurement.md` for measuring current at each operating mode (active WiFi, light sleep, deep sleep). Add to `human_to_do.md` (requires hardware + ammeter). |

---

## Milestone 7 – Integration Testing and Interoperability

> Goal: The node exchanges valid DSP messages with a real EDC connector and all deviations are documented.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-701 | Integration test: catalog fetch | Write an integration test that starts the EDC Docker container, queries `GET /catalog` on the DSP Embedded node (running on host build), and validates the response. |
| DSP-702 | Integration test: negotiation flow | Integration test covering the full offer → agree sequence against the EDC connector. |
| DSP-703 | Integration test: transfer flow | Integration test covering transfer start → status → completion against the EDC connector. |
| DSP-704 | Fill compatibility matrix | Run integration tests against each supported EDC version. Document results in `doc/compatibility_matrix.md`. |
| DSP-705 | Finalize deviation log | Review all `deviation_log.md` entries. Ensure every deviation from the DSP spec has a rationale and a note on interoperability impact. |
| DSP-706 | RAM budget audit | Run `dsp_mem_report()` in a full integration scenario. Verify all components are within their budgets from Milestone 1. Fix any overruns. |

---

## Milestone 8 – Hardening and Release

> Goal: The implementation is stable, documented, and ready for use as a reference.

| Ticket | Title | Description |
|--------|-------|-------------|
| DSP-801 | Full unit test coverage pass | Review test coverage across all modules. Add missing tests for untested branches identified during integration. |
| DSP-802 | Static analysis pass | Run `cppcheck` or `clang-tidy` on the codebase. Fix all warnings of severity ≥ medium. |
| DSP-803 | Flash-time provisioning tooling | Write a Python helper script `tools/provision.py` that flashes device certificate and PSK into NVS partitions. Document in `human_to_do.md` (requires connected hardware). |
| DSP-804 | PSRAM support | Enable and test operation with 8 MB PSRAM behind `CONFIG_DSP_PSRAM_ENABLE`. Verify no heap corruption at the boundary. |
| DSP-805 | Final README and documentation update | Update `README.md` with complete build, flash, and test instructions. Ensure `doc/` is complete and all links resolve. |
| DSP-806 | Tag v0.1.0 release | Create git tag `v0.1.0`. Write a short release note summarizing implemented endpoints, known limitations, and tested EDC versions. |

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
