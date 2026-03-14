# DSP Embedded – Compatibility Matrix

## Overview

This matrix records interoperability test results for each DSP endpoint implemented by DSP Embedded against known counterpart implementations. Tests are run against the ESP32-S3 device firmware over HTTP on the local network using the integration test suite in `integration/`.

**DSP specification version targeted:** [DSP v2024/1](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol) (`https://w3id.org/dspace/2024/1/context.json`)

**Test command:**
```
integration/.venv/bin/pytest integration/ \
  --provider-url http://<device-ip> \
  --counterpart-url http://localhost:18000 -v
```

**Result key:**

| Symbol | Meaning |
|--------|---------|
| ✅ PASS | Endpoint behaves correctly in all tested scenarios |
| ❌ FAIL | Endpoint fails in one or more tested scenarios |
| ⚠️ PARTIAL | Endpoint works but with known limitations |
| N/A | Endpoint disabled by default (compile-time flag); not registered |
| UNTESTED | Counterpart not tested against this endpoint |

---

## Endpoint Reference

The eight DSP endpoints implemented by DSP Embedded (from `target.md`), plus `POST /transfers/{id}/complete` added in DSP-703:

| # | Method | Path | Priority | Implemented |
|---|--------|------|----------|-------------|
| 1 | GET | `/catalog` | MUST | ✅ |
| 2 | POST | `/negotiations/offers` | MUST | ✅ |
| 3 | POST | `/negotiations/{id}/agree` | MUST | ✅ |
| 4 | GET | `/negotiations/{id}` | MUST | ✅ |
| 5 | POST | `/transfers/start` | MUST | ✅ |
| 6 | GET | `/transfers/{id}` | MUST | ✅ |
| 7 | POST | `/transfers/{id}/complete` | MUST¹ | ✅ |
| 8 | POST | `/catalog/request` | SHOULD | N/A² |
| 9 | POST | `/negotiations/terminate` | MAY | N/A³ |

¹ Not listed in `target.md` original eight, added in DSP-703 to complete the transfer state machine.
² Disabled by default (`CONFIG_DSP_ENABLE_CATALOG_REQUEST=0`). Enableable at compile time.
³ Disabled by default (`CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=0`). Enableable at compile time.

---

## Results

### Counterpart: DSP Embedded Python Mock v0.1

**Description:** Minimal DSP consumer mock implemented in Python/FastAPI (`docker/dsp-counterpart/`). Drives the provider (DSP Embedded node) through full negotiation and transfer flows via test-control endpoints (`/api/test/*`). Serves as the primary integration test counterpart.

**Test date:** 2026-03-14
**Firmware version:** `dsp_embedded` (commit `a7d74e5`)
**Test suite:** `integration/test_701_catalog.py`, `test_702_negotiation.py`, `test_703_transfer.py` — 52 tests total, 3.81 s
**Device:** ESP32-S3 (QFN56, rev v0.2), 8 MB PSRAM, ESP-IDF v5.5.3

| Endpoint | Result | Test(s) | Notes |
|----------|--------|---------|-------|
| GET `/catalog` | ✅ PASS | DSP-701 (14 tests) | `@type=dcat:Catalog`, `@context`, `dcat:dataset`, `dct:title`, `@id` all validated |
| POST `/negotiations/offers` | ✅ PASS | DSP-702 (18 tests) | Returns HTTP 201 + `ContractNegotiationEventMessage` with `dspace:eventType=dspace:OFFERED` |
| POST `/negotiations/{id}/agree` | ✅ PASS | DSP-702 (18 tests) | Returns HTTP 200 + `dspace:eventType=dspace:AGREED`; 404 on unknown ID |
| GET `/negotiations/{id}` | ✅ PASS | DSP-702 (18 tests) | Returns OFFERED after offer, AGREED after agree |
| POST `/transfers/start` | ✅ PASS | DSP-703 (20 tests) | Returns HTTP 200 + `TransferStartMessage`; 400 if negotiation not AGREED |
| GET `/transfers/{id}` | ✅ PASS | DSP-703 (20 tests) | Returns `TransferStartMessage` (TRANSFERRING), `TransferCompletionMessage` (COMPLETED) |
| POST `/transfers/{id}/complete` | ✅ PASS | DSP-703 (20 tests) | Returns HTTP 200 + `TransferCompletionMessage`; 404 on unknown ID |
| POST `/catalog/request` | N/A | — | Disabled by default (`CONFIG_DSP_ENABLE_CATALOG_REQUEST=0`) |
| POST `/negotiations/terminate` | N/A | — | Disabled by default (`CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=0`) |

**Summary:** All 6 MUST-priority endpoints fully operational. 52/52 integration tests pass. Both disabled SHOULD/MAY endpoints are N/A by design.

---

### Counterpart: Eclipse Tractus-X EDC (≥ v0.7.7)

**Description:** Production Eclipse Dataspace Connector used in Catena-X deployments.

**Test date:** Not tested
**Reason:** Tractus-X EDC v0.7.7–v0.12.0 requires a full Catena-X DCP/STS/BDRS infrastructure stack to boot (Managed Identity Wallet, Secure Token Service, Business Partner Data Management). This infrastructure cannot be replicated in a single-machine Docker setup for integration testing. See `human_to_do.md` for the procedure to test against a full Catena-X environment when one is available.

| Endpoint | Result | Notes |
|----------|--------|-------|
| GET `/catalog` | UNTESTED | Requires Catena-X DCP stack |
| POST `/negotiations/offers` | UNTESTED | Requires Catena-X DCP stack |
| POST `/negotiations/{id}/agree` | UNTESTED | Requires Catena-X DCP stack |
| GET `/negotiations/{id}` | UNTESTED | Requires Catena-X DCP stack |
| POST `/transfers/start` | UNTESTED | Requires Catena-X DCP stack |
| GET `/transfers/{id}` | UNTESTED | Requires Catena-X DCP stack |
| POST `/transfers/{id}/complete` | UNTESTED | Requires Catena-X DCP stack |
| POST `/catalog/request` | N/A | Disabled by default |
| POST `/negotiations/terminate` | N/A | Disabled by default |

**Expected compatibility:** The DSP Embedded node targets the DSP v2024/1 message schema used by Tractus-X EDC ≥ v0.7.x. Message fields (`dspace:processId`, `dspace:eventType`, `dspace:dataset`) match the Tractus-X EDC field expectations based on specification review. Full interoperability testing is blocked by the infrastructure requirement.

---

## Known Interoperability Constraints

These constraints apply to all counterparts. See `doc/deviation_log.md` for full rationale.

| ID | Constraint | Impact |
|----|-----------|--------|
| DEV-001 | Static JSON-LD context only; no runtime JSON-LD expansion | Counterparts must use standard DSP v2024/1 namespaces (`dspace:`, `dcat:`, `odrl:`, `dct:`). Custom JSON-LD extensions are not processed. |
| DEV-002 | No DAPS/OAuth2 on node; identity via X.509 + local JWT validation | Counterparts requiring DAPS token exchange must use the DAPS gateway shim (`CONFIG_DSP_ENABLE_DAPS_SHIM=1`) or be configured for X.509 identity. |
| DEV-003 | Provider role only; no consumer endpoints | Node cannot initiate data requests. |
| — | Transfer `@context` must be a plain string | `dsp_msg_validate_transfer_request()` uses `cJSON_IsString()` for `@context`. Counterparts sending `@context` as an array (common in offer messages) must send it as a string in `TransferRequestMessage`. |
| — | Negotiation slot limit: 4 concurrent | `DSP_NEG_MAX=CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS` (default 4). Slots are never freed during a session. Counterparts must not create more than 4 simultaneous negotiations. |
| — | Transfer slot limit: 2 concurrent | `DSP_XFER_MAX=CONFIG_DSP_MAX_CONCURRENT_TRANSFERS` (default 2). |
| — | No provider callbacks | The node does not push callbacks to `callbackAddress` after agree or transfer events. Counterparts relying on push callbacks must poll instead. |

---

## How to Add a New Counterpart Row

1. Ensure the device is freshly booted (reset to clear negotiation/transfer slot tables).
2. Start the counterpart service and note its version.
3. Run: `integration/.venv/bin/pytest integration/ --provider-url http://<device-ip> --counterpart-url <counterpart-url> -v`
4. Record PASS / FAIL / PARTIAL for each endpoint; note any failures with the error message.
5. Add a row to this document under **Results** following the format above.
6. Commit the update.
