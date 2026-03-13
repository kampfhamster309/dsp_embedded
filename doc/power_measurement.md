# DSP Embedded – Power Measurement Procedure (DSP-604)

This document describes how to measure average current consumption of the
DSP Embedded node at each operating mode and lists the expected current
ranges derived from the ESP32-S3 datasheet (v1.6, Espressif Systems).

---

## Equipment Required

| Item | Purpose | Notes |
|------|---------|-------|
| Bench power supply (3.3 V, ≥1 A) | Power source with stable voltage | Or USB power meter on 5 V rail if measuring board-level |
| Precision ammeter or current-shunt amplifier | Current measurement | A 0.1 Ω shunt + oscilloscope captures bursts; a multimeter is sufficient for average values |
| 100 µΩ–1 Ω shunt resistor | In-line current sense | Choose shunt so voltage drop < 50 mV at max expected current |
| Oscilloscope (optional) | Capture TX burst peaks | Required for peak current; not needed for average |
| USB-UART adapter (separate from board USB) | Serial log during measurement | Keep board USB disconnected if powering externally to avoid ground loops |
| Multimeter | Voltage verification | Ensure 3.3 V ± 0.1 V at chip VDD during measurement |

---

## Measurement Setup

### Option A – External 3.3 V bench supply (recommended for accuracy)

1. Disconnect the board's USB cable.
2. Locate the 3.3 V supply pin on the board (M5Stack Atom S3 has a 3.3 V pad
   near the GROVE connector).
3. Wire in series: bench supply (+) → ammeter (+) → 3.3 V pad → ESP32-S3
   VDD; bench supply (−) → board GND.
4. Set bench supply to **3.30 V, current limit 600 mA**.
5. Connect the USB-UART adapter to a spare UART pad (TX/RX) for log capture
   without disturbing the power rail.

### Option B – USB power meter (board-level, less accurate)

Insert a USB power meter (e.g. UM25C, FNIRSI FNB58) between the host USB
port and the board's USB-C connector. Multiply the displayed current by the
USB-to-3.3V efficiency (typically 0.85–0.90) to estimate chip-level current.
This method captures all board regulators and is suitable for comparative
measurements only.

### Stabilisation

Allow **30 seconds** after boot before recording steady-state current.
Instantaneous WiFi TX bursts (802.11 beacon frames, DTIM) cause spikes of
100–300 ms; use a **10-second averaging window** on the bench supply or
ammeter when measuring active-WiFi modes.

---

## Operating Modes and Measurement Procedure

### Mode 1 – Active WiFi (baseline, no power save)

This is the default build. The acquisition loop runs at 100 ms intervals,
the HTTP server is always active, and WiFi stays connected.

**Build:**
```sh
source ~/esp/esp-idf/export.sh
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi" build
idf.py -p /dev/ttyACM1 flash
```

**Expected log evidence:**
```
I (dsp_power): TWDT configured: timeout=5000 ms, panic=true
I (dsp_application): subscribed to TWDT (timeout 5000 ms)
```

**Measurement:** Record average current over 30 s while the device is idle
(HTTP server running, no active DSP negotiation or transfer in progress).

**Expected current range (3.3 V rail, chip level):**

| Condition | Expected (mA) | Datasheet reference |
|-----------|---------------|---------------------|
| WiFi RX (beacon listen) | 95–115 | Table 14, "RF receiving" |
| WiFi TX burst (11n MCS7) | 320–360 | Table 14, "RF transmitting 802.11n MCS7" |
| Average with WiFi idle | **105–130** | — |

> **Observed (2026-03-13):** Heap log shows ~73 KB consumed after full init.
> Expected average is ~120 mA based on continuous WiFi association.

---

### Mode 2 – Light Sleep (automatic CPU idle between acquisition cycles)

`esp_pm` automatic light-sleep lowers the CPU clock during FreeRTOS idle
and allows the RF modem to enter modem-sleep between DTIM intervals.

**Build:**
```sh
# Create an sdkconfig overlay enabling light sleep
echo "CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP=y
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y" > sdkconfig.light_sleep

idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.light_sleep" build
idf.py -p /dev/ttyACM1 flash
```

**Expected log evidence:**
```
I (dsp_power): light sleep enabled (max 240 MHz, min 80 MHz)
```

**Measurement:** Record average current over 30 s. Expect periodic dips to
< 5 mA during FreeRTOS idle periods between 100 ms acquisition cycles.

**Expected current range:**

| Condition | Expected (mA) | Datasheet reference |
|-----------|---------------|---------------------|
| During light-sleep period (no WiFi TX) | 0.5–2 | Table 14, "Light-sleep" |
| During WiFi TX burst | 320–360 | Table 14, "RF transmitting" |
| **Average (duty-cycled, DTIM=1)** | **20–45** | — |

> The average depends heavily on DTIM interval and beacon period
> configured by the access point. A DTIM of 1 (100 ms) yields higher
> average current than DTIM 3 (300 ms).

---

### Mode 3 – Deep Sleep (between transfers)

The node enters deep sleep after each completed transfer, preserving
negotiation and transfer state in RTC memory, and wakes on a timer for the
next acquisition cycle.

**Build:**
```sh
echo "CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=y" > sdkconfig.deep_sleep

idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wifi;sdkconfig.deep_sleep" build
idf.py -p /dev/ttyACM1 flash
```

**Expected log evidence (after `dsp_power_enter_deep_sleep()` is called):**
```
I (dsp_power): saving protocol state to RTC memory
I (dsp_power): stopping HTTP server
I (dsp_power): entering deep sleep
```

**Measurement:** Measure current **after** the device enters deep sleep
(current will drop sharply from ~120 mA to the µA range). Record the
steady-state deep-sleep current with a µA-range multimeter or shunt amplifier.

**Expected current range:**

| RTC configuration | Expected (µA) | Datasheet reference |
|-------------------|---------------|---------------------|
| RTC fast + slow memory on (state preserved) | **7–15** | Table 14, "Deep-sleep, RTC fast+slow memory" |
| RTC slow memory only | 6–8 | Table 14, "Deep-sleep, RTC slow memory" |
| All power domains off (no state) | 5–7 | Table 14, "Deep-sleep, minimum" |

> DSP Embedded uses RTC fast + slow memory (for `dsp_rtc_state`), so expect
> **~7–15 µA** during the sleep interval. Peak current at wake-up (RF
> re-enable + TLS handshake) will briefly return to the active-WiFi range.

---

## Summary Table

| Operating mode | Build flag | Expected avg current | vs. always-on |
|----------------|-----------|---------------------|---------------|
| Active WiFi (baseline) | default | 105–130 mA | — |
| Light sleep (`esp_pm`) | `CONFIG_DSP_POWER_SAVE_LIGHT_SLEEP=y` | 20–45 mA | ~4–6× reduction |
| Deep sleep (between TX) | `CONFIG_DSP_DEEP_SLEEP_BETWEEN_TX=y` | 7–15 µA (sleep), ~120 mA (active) | ~10 000× during sleep |

> Average current in deep-sleep mode depends almost entirely on the
> duty cycle: (active\_time × 120 mA + sleep\_time × 10 µA) / cycle\_period.
> For a 10-second cycle with 3-second active phase:
> avg ≈ (3 × 120 + 7 × 0.01) / 10 ≈ **36 mA**.

---

## Recording Results

Fill in measured values in the table below and commit to `cache.md`:

| Date | Build mode | Supply voltage (V) | Avg current (mA or µA) | Notes |
|------|------------|--------------------|------------------------|-------|
| | Active WiFi | 3.30 | | |
| | Light sleep | 3.30 | | |
| | Deep sleep (sleep phase) | 3.30 | | |
| | Deep sleep (active phase) | 3.30 | | |

---

## References

- ESP32-S3 Datasheet v1.6 – Section 4.7 "Electrical Characteristics", Table 14 "Current Consumption"
- ESP-IDF Power Management Guide: `components/esp_pm/README.md`
- ESP-IDF Sleep Modes: `docs/en/api-reference/system/sleep_modes.rst`
