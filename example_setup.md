# DSP Embedded – Example Setup: ESP32-S3 with DHT20 Sensor

This guide walks through connecting a DHT20 temperature and humidity sensor to an
ESP32-S3 development board, configuring and flashing the DSP Embedded firmware, and
querying live sensor readings through the Dataspace Protocol.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Bill of Materials](#2-bill-of-materials)
3. [Wiring the DHT20 to the ESP32-S3](#3-wiring-the-dht20-to-the-esp32-s3)
4. [Prerequisites](#4-prerequisites)
5. [Configuration](#5-configuration)
6. [Build and Flash](#6-build-and-flash)
7. [Verifying the Sensor at Boot](#7-verifying-the-sensor-at-boot)
8. [Interacting with the DSP Endpoint](#8-interacting-with-the-dsp-endpoint)
9. [Decoding Sensor Samples](#9-decoding-sensor-samples)
10. [Optional Configuration](#10-optional-configuration)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Overview

The firmware runs two FreeRTOS tasks pinned to separate CPU cores:

| Core | Task | Role |
|------|------|------|
| Core 0 (`dsp_protocol`) | Protocol stack | TLS, HTTP/1.1 server, DSP state machines, catalog/negotiation/transfer endpoints |
| Core 1 (`dsp_application`) | Sensor acquisition | Reads DHT20 every 100 ms, pushes temperature and humidity samples into the shared ring buffer |

The DHT20 is polled continuously. Its readings are stored in a 32-entry ring buffer shared
between the two cores. A DSP counterpart (e.g. an EDC connector or any DSP v2024/1 consumer)
discovers the dataset through `GET /catalog`, negotiates a contract, and initiates a transfer.
Core 0 drains the ring buffer during the active transfer and delivers the buffered samples
to the consumer.

```
DHT20 ──I2C──► Core 1          ring buffer          Core 0 ──HTTP──► DSP counterpart
               (acquisition)  ◄──────────────────►  (protocol)
               100 ms poll     push T + RH samples   drain on transfer
```

---

## 2. Bill of Materials

| Item | Notes |
|------|-------|
| ESP32-S3 development board | ESP32-S3-DevKitC-1 or compatible; minimum 4 MB flash |
| DHT20 sensor module | I2C variant; breakout boards (e.g. from Adafruit or AZDelivery) usually include pull-up resistors |
| 2× 4.7 kΩ resistors | Only needed if using a bare DHT20 IC without a breakout board |
| Jumper wires | Female-to-male work well with DevKit header pins |
| USB-C cable | For flashing and serial monitor |

> **DHT20 vs DHT11/DHT22:** The DHT20 uses I2C (fixed address 0x38), not the single-wire
> protocol of the DHT11/DHT22. Make sure you have the correct sensor variant before wiring.

---

## 3. Wiring the DHT20 to the ESP32-S3

### 3.1 Pin Assignment (defaults)

| DHT20 pin | ESP32-S3 GPIO | DevKitC-1 header label | Notes |
|-----------|---------------|------------------------|-------|
| VDD | 3V3 | 3V3 | **3.3 V only** – do not connect to 5 V |
| GND | GND | GND | Any GND pin |
| SDA | GPIO 8 | IO8 | I2C data |
| SCL | GPIO 9 | IO9 | I2C clock |

These are the firmware defaults; see [Section 5.3](#53-sensor-gpio-optional) to use
different GPIO numbers.

### 3.2 Pull-up Resistors

The I2C bus requires pull-up resistors on SDA and SCL.

- **Breakout board with resistors already fitted:** wire directly, nothing extra required.
  Most commercially available DHT20 modules include 4.7 kΩ pull-ups and a decoupling
  capacitor on the board.
- **Bare DHT20 IC:** place one 4.7 kΩ resistor between SDA and 3.3 V, and a second
  4.7 kΩ resistor between SCL and 3.3 V.

> The firmware enables the ESP32-S3 internal weak pull-ups
> (`flags.enable_internal_pullup = true`) as a fallback, but these are typically 45–70 kΩ
> and are too weak for reliable I2C at 100 kHz. External 4.7 kΩ pull-ups are required for
> production use.

### 3.3 Wiring Diagram

```
ESP32-S3-DevKitC-1            DHT20 breakout module
┌─────────────────┐            ┌──────────────────┐
│            3V3  │────────────│ VDD              │
│            GND  │────────────│ GND              │
│            IO8  │────────────│ SDA              │
│            IO9  │────────────│ SCL              │
└─────────────────┘            └──────────────────┘

If using a bare DHT20 IC (no breakout):

         3V3 ──┬──────────┬────── DHT20 VDD
               │          │
             4.7k        4.7k   (pull-up resistors)
               │          │
         IO8 ──┘   IO9 ──┘
         SDA         SCL
```

### 3.4 DevKitC-1 Header Reference

On the ESP32-S3-DevKitC-1, GPIO 8 and 9 are located on the left-side header when the USB
connector faces down:

```
                USB-C
              ┌───────┐
  (left side) │       │ (right side)
    3V3  ─── 1│       │1 ─── GND
    3V3  ─── 2│       │2 ─── TX
    RST  ─── 3│       │3 ─── RX
     4   ─── 4│       │4 ─── 1
     5   ─── 5│       │5 ─── 2
     6   ─── 6│       │6 ─── 42
     7   ─── 7│       │7 ─── 41
   ► 8   ─── 8│       │8 ─── 40   ◄ SDA (GPIO 8)
   ► 9   ─── 9│       │9 ─── 39   ◄ SCL (GPIO 9)
    10   ──10 │       │10─── 38
              └───────┘
```

Exact pin numbering varies by board revision; always verify against the schematic for your
specific board.

---

## 4. Prerequisites

### 4.1 Software

| Tool | Version | Install |
|------|---------|---------|
| ESP-IDF | v5.5 | `git clone --recursive https://github.com/espressif/esp-idf && cd esp-idf && ./install.sh esp32s3` |
| CMake | ≥ 3.16 | Bundled with ESP-IDF |
| Python | ≥ 3.10 | Bundled with ESP-IDF |
| OpenSSL | Any recent | `apt install openssl` / `brew install openssl` |

After installing ESP-IDF, source its environment in every new terminal session:

```bash
source ~/esp/esp-idf/export.sh
```

### 4.2 Repository

```bash
git clone https://github.com/your-org/dsp_embedded.git
cd dsp_embedded
git submodule update --init test/unity
```

---

## 5. Configuration

### 5.1 Device Certificate

The DSP HTTP server presents a TLS certificate to counterparts. Generate a self-signed
development certificate and embed it into the firmware:

```bash
bash tools/gen_dev_certs.sh
```

This creates `main/certs/device_cert.pem` and `main/certs/device_key.pem`. These files are
embedded at build time via ESP-IDF's `EMBED_TEXTFILES` mechanism.

> For production, replace these with a certificate signed by your trust anchor CA.

### 5.2 WiFi Credentials

Create an sdkconfig overlay file with your network credentials:

```bash
cat > sdkconfig.wifi <<'EOF'
CONFIG_DSP_WIFI_PROVISION=y
CONFIG_DSP_WIFI_PROVISION_SSID="YourNetworkName"
CONFIG_DSP_WIFI_PROVISION_PASSWORD="YourPassword"
EOF
```

Credentials are written to NVS on first boot and survive subsequent flashes unless the NVS
partition is erased.

### 5.3 Sensor GPIO (optional)

The default SDA/SCL assignment is GPIO 8 and 9. To use different pins, either:

**Option A – sdkconfig override file** (recommended, no rebuild of all sources required):

```bash
cat >> sdkconfig.wifi <<'EOF'
CONFIG_DSP_DHT20_SDA_PIN=21
CONFIG_DSP_DHT20_SCL_PIN=22
CONFIG_DSP_DHT20_I2C_PORT=0
EOF
```

**Option B – interactive menuconfig:**

```bash
idf.py menuconfig
# Navigate to: Component config → DHT20 Sensor (dsp_dht20)
```

### 5.4 Poll Interval (optional)

The sensor is read every 100 ms by default (10 readings per second per channel). The
DHT20 conversion time is approximately 85 ms, so values below 100 ms will cause consecutive
reads to collide; 100 ms is the practical minimum.

To reduce CPU load or ring buffer pressure, increase the interval:

```bash
# In sdkconfig.wifi, add:
CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS=500   # 2 readings/second
```

The ring buffer holds 32 samples. At 100 ms poll interval this represents 1.6 seconds of
backlog (2 channels × 16 slots each). If the ring buffer fills before a transfer drains it,
the newest samples are dropped and a warning is logged.

### 5.5 Catalog and Dataset Identity (optional)

Customize how the sensor dataset appears to DSP counterparts:

```bash
# In sdkconfig.wifi, add:
CONFIG_DSP_PROVIDER_ID="urn:uuid:my-esp32s3-node-001"
CONFIG_DSP_CATALOG_DATASET_ID="urn:uuid:my-dht20-dataset-001"
CONFIG_DSP_CATALOG_TITLE="ESP32-S3 Environmental Sensor"
```

These strings appear verbatim in the JSON-LD catalog response.

---

## 6. Build and Flash

### 6.1 Build

```bash
idf.py \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi" \
  build
```

With 8 MB OPI PSRAM (ESP32-S3 QFN56):

```bash
idf.py \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.psram" \
  build
```

> **Note:** If you switch between PSRAM and non-PSRAM builds, delete `build/` and
> `sdkconfig` first to avoid stale configuration:
> ```bash
> rm -rf build sdkconfig
> ```

### 6.2 Flash

Identify the device port:

```bash
ls /dev/ttyACM* /dev/ttyUSB*   # Linux
ls /dev/cu.usb*                 # macOS
```

Flash and open the serial monitor in one step:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Press `Ctrl-]` to exit the monitor.

---

## 7. Verifying the Sensor at Boot

After reset, the serial monitor should show the following boot sequence. The lines marked
with `►` confirm that the DHT20 driver initialised and is producing readings:

```
I (dsp_embedded): DSP Embedded starting (M5 dual-core build)...
I (dsp_wifi): connected – IP: 192.168.1.42
I (dsp_protocol): protocol task running on core 0 (expected 0)
► I (dsp_dht20): init ok — SDA=8 SCL=9 I2C0 addr=0x38
► I (dsp_dht20): sensor calibrated (status=0x18)
I (dsp_application): application task ready (poll interval 100 ms)
I (dsp_application): subscribed to TWDT (timeout 5000 ms)
I (dsp_embedded): DSP Embedded running on port 80
```

If calibration bits are not set on your specific DHT20 unit (uncommon), the driver resets
the three internal calibration registers automatically:

```
W (dsp_dht20): sensor uncalibrated (status=0x00), resetting regs
I (dsp_dht20): calibration registers restored
```

This is normal and requires no user action.

### 7.1 Confirming I2C Connectivity

If the DHT20 is not detected, `dsp_dht20_init()` returns a non-OK status and the
application task logs:

```
E (dsp_application): DHT20 init failed (0x107) – acquisitions will be skipped
```

Common error codes:

| Code | Meaning |
|------|---------|
| `0x107` (`ESP_ERR_NOT_FOUND`) | No ACK on I2C bus at address 0x38 – check wiring |
| `0x103` (`ESP_ERR_INVALID_STATE`) | I2C port already in use by another component |
| `0x101` (`ESP_ERR_NO_MEM`) | Heap exhausted during bus creation |

The device continues running without sensor data if the DHT20 fails to initialise – the
DSP protocol stack remains fully operational.

---

## 8. Interacting with the DSP Endpoint

Once the device is running and connected to WiFi, a DSP counterpart can discover and
consume the sensor dataset using standard DSP v2024/1 messages. The following examples use
`curl` to walk through the full flow manually.

Replace `192.168.1.42` with the IP address printed in the boot log.

### 8.1 Discover the Catalog

```bash
curl -s http://192.168.1.42/catalog | python3 -m json.tool
```

Example response (JSON-LD):

```json
{
  "@context":  "https://w3id.org/dspace/2024/1/context.json",
  "@type":     "dcat:Catalog",
  "dct:title": "ESP32-S3 Environmental Sensor",
  "dcat:dataset": [
    {
      "@type": "dcat:Dataset",
      "@id":   "urn:uuid:my-dht20-dataset-001",
      "odrl:hasPolicy": {
        "@type": "odrl:Offer",
        "@id":   "urn:uuid:dsp-embedded-offer-1"
      }
    }
  ]
}
```

### 8.2 Initiate a Contract Negotiation

```bash
curl -s -X POST http://192.168.1.42/negotiations/offers \
  -H "Content-Type: application/json" \
  -d '{
    "@context": "https://w3id.org/dspace/2024/1/context.json",
    "@type":    "dspace:ContractRequestMessage",
    "dspace:processId":    "urn:uuid:neg-001",
    "dspace:consumerPid": "urn:uuid:consumer-001",
    "dspace:offer": {
      "@type": "odrl:Offer",
      "@id":   "urn:uuid:dsp-embedded-offer-1"
    }
  }' | python3 -m json.tool
```

The device responds with a `ContractNegotiationMessage` in the `OFFERED` state. Record the
`dspace:processId` from the response for the next step.

### 8.3 Accept the Offer

```bash
curl -s -X POST http://192.168.1.42/negotiations/urn:uuid:neg-001/agree \
  -H "Content-Type: application/json" \
  -d '{
    "@context": "https://w3id.org/dspace/2024/1/context.json",
    "@type":    "dspace:ContractAgreementMessage",
    "dspace:processId":   "urn:uuid:neg-001",
    "dspace:consumerPid": "urn:uuid:consumer-001",
    "dspace:agreement": {
      "@type": "odrl:Agreement",
      "@id":   "urn:uuid:agreement-001"
    }
  }' | python3 -m json.tool
```

### 8.4 Start a Transfer

```bash
curl -s -X POST http://192.168.1.42/transfers/start \
  -H "Content-Type: application/json" \
  -d '{
    "@context": "https://w3id.org/dspace/2024/1/context.json",
    "@type":         "dspace:TransferRequestMessage",
    "dspace:processId":    "urn:uuid:xfer-001",
    "dspace:consumerPid": "urn:uuid:consumer-001",
    "dspace:dataAddress": {
      "dspace:endpointType": "https://w3id.org/idsa/v4.1/HTTP"
    }
  }' | python3 -m json.tool
```

### 8.5 Check Transfer State

```bash
curl -s http://192.168.1.42/transfers/urn:uuid:xfer-001 | python3 -m json.tool
```

### 8.6 Complete the Transfer

```bash
curl -s -X POST http://192.168.1.42/transfers/urn:uuid:xfer-001/complete \
  -H "Content-Type: application/json" \
  -d '{
    "@context": "https://w3id.org/dspace/2024/1/context.json",
    "@type":         "dspace:TransferCompletionMessage",
    "dspace:processId": "urn:uuid:xfer-001"
  }' | python3 -m json.tool
```

---

## 9. Decoding Sensor Samples

The ring buffer stores `dsp_sample_t` records. The two DHT20 channels are encoded as fixed-
point integers to avoid floating-point in the shared struct:

| `channel` | Constant | Sensor reading | Encoding | Decode |
|-----------|----------|----------------|----------|--------|
| `0` | `DSP_DHT20_CH_TEMP` | Temperature (°C) | `(uint32_t)(int32_t)(T × 100)` | `(float)(int32_t)raw_value / 100.0f` |
| `1` | `DSP_DHT20_CH_HUM` | Relative humidity (%) | `(uint32_t)(RH × 100)` | `(float)raw_value / 100.0f` |

### Examples

| T (°C) | raw_value (ch 0) | Decoded |
|--------|-----------------|---------|
| 21.50 | 2150 | (float)(int32_t)2150 / 100.0 = 21.50 °C |
| −5.25 | 4294966771 | (float)(int32_t)4294966771 / 100.0 = −5.25 °C |
| 0.00 | 0 | 0.00 °C |

| RH (%) | raw_value (ch 1) | Decoded |
|--------|-----------------|---------|
| 55.00 | 5500 | (float)5500 / 100.0 = 55.00 % |
| 100.00 | 10000 | 100.00 % |
| 0.00 | 0 | 0.00 % |

The temperature channel uses a signed integer cast so that sub-zero values round-trip
correctly through the `uint32_t` storage field.

### DHT20 Sensor Accuracy

| Parameter | Range | Accuracy |
|-----------|-------|----------|
| Temperature | −40 °C to +85 °C | ±0.5 °C (typ.) |
| Relative Humidity | 0 % to 100 % | ±3 % RH (typ.) |
| Resolution | — | 0.01 °C / 0.024 % RH |

---

## 10. Optional Configuration

### 10.1 Power Saving – CPU Light Sleep

Reduces active-WiFi current from ~120 mA to ~20–40 mA at negligible HTTP latency cost:

```bash
# Add to sdkconfig.wifi:
CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP=y
```

### 10.2 Power Saving – Deep Sleep Between Transfers

Enters ESP32-S3 deep sleep after each completed transfer. DSP protocol state
(active negotiations and transfers) is preserved in RTC memory and restored on the next
wakeup:

```bash
# Add to sdkconfig.wifi:
CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=y
```

> Deep sleep stops the DHT20 acquisition loop. The sensor powers down and is re-initialised
> on each wakeup. Allow at least 100 ms after wakeup before the first valid reading is
> available (DHT20 power-on stabilisation requirement).

### 10.3 Verbose DSP Logging

Logs full request/response bodies and state-machine transitions. Disable in production:

```bash
# Add to sdkconfig.wifi:
CONFIG_DSP_LOG_LEVEL_VERBOSE=y
```

### 10.4 PSK Authentication

Enables the `dsp_psk` module so TLS connections can additionally be validated with a
pre-shared key:

```bash
# Add to sdkconfig.wifi:
CONFIG_DSP_ENABLE_PSK=y
```

After flashing, provision the PSK credential using the provisioning tool:

```bash
source ~/esp/esp-idf/export.sh
python3 tools/provision.py \
  --cert main/certs/device_cert.pem \
  --key  main/certs/device_key.pem  \
  --port /dev/ttyACM0
```

### 10.5 Alternative I2C Port

The ESP32-S3 has two hardware I2C controllers (I2C0 and I2C1). If I2C0 is used by another
peripheral, switch to I2C1:

```bash
# Add to sdkconfig.wifi:
CONFIG_DSP_DHT20_I2C_PORT=1
CONFIG_DSP_DHT20_SDA_PIN=17
CONFIG_DSP_DHT20_SCL_PIN=18
```

---

## 11. Troubleshooting

### DHT20 not detected at boot

```
E (dsp_application): DHT20 init failed (0x107) – acquisitions will be skipped
```

1. Check that VDD is 3.3 V (measure with a multimeter).
2. Confirm SDA and SCL are connected to the correct GPIO pins.
3. Verify pull-up resistors are present on SDA and SCL (4.7 kΩ to 3.3 V).
4. Check that you are using a DHT20 and not a DHT11 or DHT22 (the DHT11/22 use a
   different single-wire protocol and will not respond on I2C).
5. If using a long cable (> 30 cm), reduce pull-up resistor value to 2.2 kΩ.

### CRC errors during operation

```
E (dsp_dht20): CRC mismatch: computed=0xAB received=0xCD
```

Intermittent CRC failures indicate electrical noise on the I2C bus:

- Keep I2C wires shorter than 30 cm.
- Route SDA and SCL away from power traces and the antenna area.
- Add a 100 nF decoupling capacitor across VDD and GND on the DHT20 module.
- Lower the I2C clock from 100 kHz (requires modifying `DHT20_I2C_SCL_HZ` in
  `dsp_dht20.c` to 50000 and rebuilding).

### Ring buffer full warnings

```
W (dsp_application): ring buffer full, temperature sample dropped
```

This means Core 0 is not draining samples fast enough. The ring buffer holds 32 samples
(~1.6 s at the default 100 ms poll interval). Options:

- Increase `CONFIG_DSP_APPLICATION_POLL_INTERVAL_MS` (slower poll = more headroom).
- Ensure an active DSP transfer is draining the buffer; samples are not forwarded until
  a transfer is in the `TRANSFERRING` state.

### WiFi connection fails at boot

The device retries up to `CONFIG_DSP_WIFI_MAX_RETRY` times (default: 5) with
`CONFIG_DSP_WIFI_RECONNECT_DELAY_MS` between attempts (default: 1000 ms). If all retries
are exhausted, the device halts with:

```
E (dsp_wifi): all retries exhausted – halting
```

Check SSID and password in `sdkconfig.wifi`, rebuild, and reflash.

### Serial monitor shows only garbage characters

The USB-CDC interface needs 3 seconds to re-enumerate after reset. The firmware waits for
this automatically, so the first log lines may be lost if the monitor was not open yet.
Press the reset button on the board after the monitor is running.

### Build fails: `sdkconfig` conflicts with previous build

Delete stale configuration files before switching between sdkconfig overlays:

```bash
rm -rf build sdkconfig
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi" build
```
