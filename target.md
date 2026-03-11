# DSP Embedded – Lean Dataspace Protocol Reference Implementation for Microcontrollers

## Project Description

**DSP Embedded** is an experimental reference implementation of the [Dataspace Protocol (DSP)](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol) by the International Data Spaces Association (IDSA), written in C for resource-constrained embedded systems.

The goal is to enable a fully functional — albeit simplified — Dataspace participant at the microcontroller level, without relying on an edge gateway or Java-based middleware. This is particularly relevant for mobile, battery-powered platforms such as UAVs, UGVs, and IoT sensor nodes, where every watt and every gram counts.

The implementation is **not a full Eclipse Dataspace Connector (EDC)** and does not claim full standards compliance. It implements a practical subset of the DSP that enables interoperability with compliant counterparts, provided they support a compatible lean mode.

---

## Target Hardware

**Primary: ESP32-S3**

| Feature | Value |
|---|---|
| CPU | Xtensa LX7 Dual-Core, up to 240 MHz |
| RAM | 512 KB SRAM + optional 8 MB PSRAM |
| Flash | 4–16 MB |
| Connectivity | WiFi 802.11b/g/n, Bluetooth 5 |
| Hardware Crypto | AES, SHA, RSA, ECC (hardware acceleration) |
| Deep Sleep | ~20 µA |
| Active Operation (WiFi) | ~100–150 mA |
| SDK | ESP-IDF (FreeRTOS-based) |

The ESP32-S3 represents the realistic lower bound for a Dataspace-capable node: sufficient RAM for TLS and a DSP subset, hardware cryptography for performant JWT processing, and power-saving sleep modes for battery-powered operation.

---

## Constraints and Design Principles

### 1. Memory Budget

The entire implementation must operate within a defined RAM budget that leaves sufficient headroom for application logic (sensor reading, data buffering).

| Component | Target RAM (max.) |
|---|---|
| TLS Stack (mbedTLS, reduced) | 50 KB |
| HTTP/1.1 Server | 20 KB |
| JSON Parser | 15 KB |
| JWT Processing | 25 KB |
| DSP State Machine | 20 KB |
| Application Logic / Sensor Data | ≥ 150 KB (to remain free) |
| **Total DSP Embedded** | **≤ 130 KB RAM** |

### 2. No JSON-LD at Runtime

JSON-LD requires a context-sensitive RDF parser at runtime, which cannot be reasonably implemented on the ESP32-S3. Instead:

- JSON-LD contexts are **statically pre-compiled at compile time**.
- The node uses a fixed, predefined set of namespaces.
- Incoming messages are validated against predefined schemas, not dynamically resolved.
- The result is **not a full JSON-LD processor**, but an endpoint capable of sending and interpreting valid DSP-compliant messages.

### 3. Simplified Identity Instead of DAPS

The Dynamic Attribute Provisioning Service (DAPS) requires a complex OAuth2 PKI infrastructure that is inappropriate for many edge scenarios. DSP Embedded uses instead:

- **Pre-Provisioned X.509 Certificates** (embedded at flash time)
- Optional **Pre-Shared Key (PSK)** for highly constrained environments
- JWT tokens are validated locally (ES256/RS256 via hardware crypto)
- Compatibility shim for DAPS integration via an optional gateway

### 4. Minimal DSP Subset

Only the DSP endpoints strictly necessary for a data provider are implemented:

```
DSP Endpoint                  Priority    Description
─────────────────────────────────────────────────────────────────
GET  /catalog                 MUST        Static asset catalog
POST /negotiations/offers     MUST        Receive contract offer
POST /negotiations/{id}/agree MUST        Confirm contract
GET  /negotiations/{id}       MUST        Negotiation status
POST /transfers/start         MUST        Start data transfer
GET  /transfers/{id}          MUST        Transfer status
POST /catalog/request         SHOULD      Dynamic catalog queries
POST /negotiations/terminate  MAY         Terminate negotiation
```

Consumer-side endpoints (i.e., the node requesting data itself) are not in the initial scope.

### 5. Power Management as a First-Class Citizen

Energy management is not a secondary feature but an integral part of the architecture:

- DSP connections are **stateless where possible** (HTTP/1.1, no persistent keep-alive)
- The node can enter **Deep Sleep** between transfers
- Catalog and contract parameters are held in **RTC memory** (survives deep sleep)
- Connection setup (TLS handshake) is minimized via **Session Tickets**
- Watchdog-secured **power-save mode** between data acquisition cycles

### 6. Dual-Core Architecture (ESP32-S3 specific)

```
Core 0 (Protocol)                 Core 1 (Application)
─────────────────────────         ─────────────────────────
lwIP Network Stack                Sensor Polling Loop
mbedTLS                           ADC / I2C / SPI
HTTP Server                       Data Buffering
DSP State Machine                 Data Compression
JWT Processing                    Sleep Management
         │                                │
         └──────── Shared Memory ─────────┘
                  (Ring Buffer,
                   FreeRTOS Queue)
```

### 7. Configurability via Compile-Time Flags

Resource constraints vary greatly depending on the use case. All optional components are controllable via `#ifdef` guards:

```c
CONFIG_DSP_ENABLE_CATALOG_REQUEST   // Dynamic catalog queries
CONFIG_DSP_ENABLE_CONSUMER          // Enable consumer role
CONFIG_DSP_ENABLE_DAPS_SHIM         // DAPS gateway integration
CONFIG_DSP_TLS_SESSION_TICKETS      // TLS session resumption
CONFIG_DSP_PSRAM_ENABLE             // Use external PSRAM
CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX    // Sleep between transfers
```

### 8. Testability and Interoperability

- Full **unit test suite** (Unity Framework, also runs on host system)
- **Integration tests** against a full EDC connector (Docker-based)
- Documented **deviation log**: All deviations from the DSP standard are explicitly documented and justified
- Compatibility matrix against known EDC versions

---

## Non-Goals (explicitly excluded)

- Full JSON-LD runtime processing
- Full DAPS/OAuth2 implementation on the node itself
- Consumer role in the initial release
- Java/Python bindings
- MQTT or CoAP transport (HTTP/1.1 only in initial scope)
- Support for controllers with less than 256 KB RAM (e.g., RP2040, nRF9160)

---

## Related Projects and Standards

- [Eclipse Dataspace Components (EDC)](https://github.com/eclipse-edc)
- [IDSA Dataspace Protocol Specification](https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol)
- [mbedTLS](https://github.com/Mbed-TLS/mbedtls)
- [ESP-IDF](https://github.com/espressif/esp-idf)
- [cJSON](https://github.com/DaveGamble/cJSON)

---

## Status

> ⚠️ Concept phase – This document describes the framework conditions for a reference implementation yet to be developed.
