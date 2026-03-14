# DSP Embedded v0.1.0 – Release Notes

**Release date:** 2026-03-14
**Target hardware:** ESP32-S3 (Xtensa LX7 dual-core, 512 KB SRAM, optional 8 MB OPI PSRAM)
**ESP-IDF version:** v5.5.3
**DSP specification:** [Dataspace Protocol v2024/1](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol)

---

## What's in v0.1.0

This is the initial release of DSP Embedded — a lean Dataspace Protocol reference
implementation in C for resource-constrained embedded systems. It enables a fully
functional DSP data provider on an ESP32-S3 microcontroller without requiring an
edge gateway or Java-based middleware.

---

## Implemented DSP Endpoints

All endpoints operate as **data provider** only. The node acts as the provider side
of each DSP protocol interaction; consumer-side initiation is not implemented.

| Method | Path | DSP Priority | Notes |
|--------|------|-------------|-------|
| GET | `/catalog` | MUST | Returns a static JSON-LD catalog (`dcat:Catalog`) with configured dataset ID and title |
| POST | `/negotiations/offers` | MUST | Receives a `ContractRequestMessage`; creates a negotiation slot and advances to OFFERED state; returns HTTP 201 + `ContractNegotiationEventMessage` |
| POST | `/negotiations/{id}/agree` | MUST | Advances OFFERED → AGREED; returns HTTP 200 + `dspace:eventType=dspace:AGREED` |
| GET | `/negotiations/{id}` | MUST | Returns current negotiation state (OFFERED or AGREED) |
| POST | `/transfers/start` | MUST | Creates a transfer slot for an AGREED negotiation; advances to TRANSFERRING; returns HTTP 200 + `TransferStartMessage` |
| GET | `/transfers/{id}` | MUST | Returns transfer state: `TransferStartMessage` (TRANSFERRING) or `TransferCompletionMessage` (COMPLETED) |
| POST | `/transfers/{id}/complete` | MUST | Advances TRANSFERRING → COMPLETED; returns HTTP 200 + `TransferCompletionMessage` |
| POST | `/catalog/request` | SHOULD | Optional; disabled by default (`CONFIG_DSP_ENABLE_CATALOG_REQUEST=y` to enable) |
| POST | `/negotiations/terminate` | MAY | Optional; disabled by default (`CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=y` to enable) |

---

## Known Limitations

These limitations are intentional design decisions for the constrained target hardware.
Each is documented with full rationale in [doc/deviation_log.md](doc/deviation_log.md).

| ID | Limitation | Impact |
|----|-----------|--------|
| DEV-001 | **No runtime JSON-LD processor.** JSON-LD contexts are statically compiled as C string constants (`dsp_jsonld_ctx.h`). Remote context dereferencing, expansion, and framing are not performed. `TransferRequestMessage` bodies must use `@context` as a plain string (not an array). | Counterparts using custom JSON-LD extensions or non-standard context URLs will not interoperate. `@context` as a JSON array in `TransferRequestMessage` returns HTTP 400. |
| DEV-002 | **No DAPS/OAuth2 on the node.** Identity uses pre-provisioned X.509 certificates (embedded at flash time or loaded from the `dsp_certs` NVS partition). JWT tokens are validated locally via mbedTLS hardware crypto (ES256/RS256). An optional DAPS gateway shim is available (`CONFIG_DSP_ENABLE_DAPS_SHIM=y`). | Counterparts requiring DAPS-issued Dynamic Attribute Tokens must use the DAPS gateway shim or be configured to accept X.509 identity. |
| DEV-003 | **Provider role only.** Consumer-side DSP endpoints (initiating catalog requests, offers, or transfers from the node) are not implemented. | The node cannot act as a DSP consumer. All DSP interactions must be initiated by the remote counterpart. |
| DEV-004 | **No provider-to-consumer push callbacks.** After state transitions (agree, transfer start, transfer complete), the node does not POST to the consumer's `dspace:callbackAddress`. The field is accepted and silently ignored. Consumers must poll via `GET /negotiations/{id}` and `GET /transfers/{id}`. | Counterparts blocking on push callbacks will time out. |
| DEV-005 | **Simplified negotiation state machine.** States REQUESTED, ACCEPTED, VERIFIED, and FINALIZED are not implemented. The implemented path is: OFFERED → AGREED (+ optional TERMINATED). | Counterparts expecting FINALIZED as the terminal agreed state must treat AGREED as final. |
| DEV-006 | **Simplified transfer state machine.** SUSPENDED and TERMINATED transfer states are not implemented. Implemented path: TRANSFERRING → COMPLETED / FAILED. | Counterparts sending `TransferSuspensionMessage` will receive an error response. |
| DEV-007 | **Fixed concurrent session limits.** Maximum 4 simultaneous negotiations (`CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS`) and 2 simultaneous transfers (`CONFIG_DSP_MAX_CONCURRENT_TRANSFERS`). Slots are not reclaimed during a session; the device must be rebooted to reset the slot tables. | Exceeding the slot limit returns HTTP 500. Both limits are configurable at compile time via Kconfig. |

---

## Tested Counterparts

### DSP Embedded Python Mock v0.1 — PASS

**Description:** Minimal DSP consumer mock implemented in Python/FastAPI
(`docker/dsp-counterpart/`). Used as the primary integration test counterpart.

**Test date:** 2026-03-14
**Test suite:** `integration/test_701_catalog.py`, `test_702_negotiation.py`,
`test_703_transfer.py` — **52 tests, 52 passed, 3.81 s**
**Firmware:** `dsp_embedded` v0.1.0, ESP-IDF v5.5.3
**Device:** ESP32-S3 (QFN56 rev 0.2), 8 MB OPI PSRAM, WiFi 802.11n

| Endpoint | Result |
|----------|--------|
| GET `/catalog` | ✅ PASS (14 tests) |
| POST `/negotiations/offers` | ✅ PASS (18 tests) |
| POST `/negotiations/{id}/agree` | ✅ PASS (18 tests) |
| GET `/negotiations/{id}` | ✅ PASS (18 tests) |
| POST `/transfers/start` | ✅ PASS (20 tests) |
| GET `/transfers/{id}` | ✅ PASS (20 tests) |
| POST `/transfers/{id}/complete` | ✅ PASS (20 tests) |

---

### Eclipse Tractus-X EDC (v0.7.7–v0.12.0) — UNTESTED

**Reason:** Tractus-X EDC ≥ v0.7.7 requires a full Catena-X DCP/STS/BDRS
infrastructure stack (Managed Identity Wallet, Secure Token Service, BPN/DID
Resolution Service) to boot. This infrastructure cannot be replicated in a
single-machine Docker setup.

**Expected compatibility:** DSP Embedded targets the DSP v2024/1 message schema
used by Tractus-X EDC ≥ v0.7.x. Message field names (`dspace:processId`,
`dspace:eventType`, `dspace:dataset`) match Tractus-X EDC expectations based on
specification review. Known constraint: `TransferRequestMessage` must supply
`@context` as a plain string, not an array.

**How to test when a Catena-X environment is available:** See [human_to_do.md](human_to_do.md)
(DSP-704 section) for the full procedure.

---

## Unit Test Summary

| Test binary | Tests | Result |
|-------------|-------|--------|
| `dsp_test_runner` (main suite) | 441 | ✅ 0 failures |
| `dsp_test_no_tickets` (TLS session tickets disabled) | 7 | ✅ 0 failures |
| `dsp_test_deep_sleep_on` (deep sleep enabled) | 19 | ✅ 0 failures |
| `dsp_test_psk_on` (PSK enabled) | 14 | ✅ 0 failures |
| `dsp_test_terminate_on` (negotiate terminate enabled) | 8 | ✅ 0 failures |
| `dsp_test_catalog_req_on` (catalog request enabled) | 8 | ✅ 0 failures |
| **Total** | **497** | **✅ 0 failures** |

---

## RAM Budget (ESP32-S3 internal SRAM, measured on device)

| Component | Measured | Budget |
|---|---|---|
| TLS server context | 2.0 KB | ≤ 50 KB ✅ |
| HTTP server | 11.3 KB | ≤ 20 KB ✅ |
| DSP state machines | 0 KB (BSS) | ≤ 20 KB ✅ |
| JSON parser (peak transient) | < 2 KB | ≤ 15 KB ✅ |
| JWT processing (peak transient) | < 2 KB | ≤ 25 KB ✅ |

Steady-state free heap: **246 KB**. All-time minimum free (post-integration-suite): **236 KB**.

---

## Static Analysis

`cppcheck 2.13.0` — **zero findings at severity ≥ medium** across 19 component sources
and 34 test sources. See [doc/static_analysis.md](doc/static_analysis.md).

---

## How to Build and Run

See [README.md](README.md) for complete build, flash, and test instructions.
