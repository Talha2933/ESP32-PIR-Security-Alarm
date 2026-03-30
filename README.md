# ESP32 IoT Security Alarm System

> WiFi-connected PIR motion alarm with remote monitoring via Blynk IoT — works fully offline too.

**Version 3.0 • Blynk IoT • Arduino IDE**  
📺 YouTube: [@funwithelectronics273](https://youtube.com/@funwithelectronics273)  
💻 GitHub: (https://github.com/Talha2933)

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Features](#2-features)
3. [Hardware Components](#3-hardware-components)
4. [Pin Configuration](#4-pin-configuration)
5. [Blynk Virtual Pins](#5-blynk-virtual-pins)
6. [WiFi Setup](#6-wifi-setup)
7. [Offline Mode](#7-offline-mode)
8. [Blynk Mobile Dashboard](#8-blynk-mobile-dashboard)
9. [Getting Started](#9-getting-started)
10. [Troubleshooting](#10-troubleshooting)
11. [Security Warning](#11-security-warning)
12. [License](#12-license)

---

## 1. Project Overview

This project turns an ESP32 microcontroller into a WiFi-connected PIR motion alarm that you can monitor and control remotely from your phone using the [Blynk IoT](https://blynk.cloud) platform.

When motion is detected, the relay fires immediately — **even without internet** — and Blynk sends you a push notification and email alert.

---

## 2. Features

- PIR motion detection with configurable hold time
- Works fully offline — relay triggers without WiFi
- Quick WiFi switch: tries primary network, instantly falls back to secondary
- Blynk push notification + email on every motion event
- Real-time ESP32 chip temperature on dashboard (every 2 seconds)
- Live uptime counter (HH:MM:SS)
- False trigger protection (5-second PIR warm-up on arm)
- No external temperature library required

---

## 3. Hardware Components

| Component                      | Quantity | Notes                        |
|--------------------------------|----------|------------------------------|
| ESP32 Dev Board (any variant)  | 1        | 38-pin or 30-pin both work   |
| HC-SR501 PIR Motion Sensor     | 1        | Adjust sensitivity as needed |
| 5V Relay Module                | 1        | Active-HIGH, GPIO 22         |
| Buzzer or Siren (optional)     | 1        | Connect to relay NO terminal |
| Micro USB Cable + 5V Adapter   | 1        | Minimum 1A rated             |

---

## 4. Pin Configuration

| Signal         | GPIO  | Notes                        |
|----------------|-------|------------------------------|
| PIR Sensor     | 27    | Digital input                |
| Relay          | 22    | Active-HIGH output           |
| Status LED     | 2     | Built-in LED on most boards  |

---

## 5. Blynk Virtual Pins

| Pin | Data Type | Range   | Purpose                      |
|-----|-----------|---------|------------------------------|
| V0  | Integer   | 0 / 1   | Motion indicator LED widget  |
| V1  | String    | —       | Motion status text label     |
| V2  | Integer   | 0 / 1   | Alarm ON / OFF switch button |
| V3  | Double    | 0–100   | ESP32 chip temperature °C    |
| V4  | Integer   | 0 / 1   | ESP32 online status          |
| V5  | String    | —       | ESP32 uptime HH:MM:SS        |

> **Note on V3 Temperature:** V3 shows ESP32 chip die temperature — not room temperature. A reading of 45–65°C is completely normal. The chip runs 10–20°C hotter than ambient air. Only worry if it exceeds 80°C.

---

## 6. WiFi Setup

Update your WiFi credentials in the code before uploading:

```cpp
const char* WIFI_SSID1 = "YOUR_WIFI_NAME";       // Primary network
const char* WIFI_PASS1 = "YOUR_WIFI_PASSWORD";

const char* WIFI_SSID2 = "YOUR_BACKUP_WIFI_NAME"; // Backup / hotspot
const char* WIFI_PASS2 = "YOUR_BACKUP_PASSWORD";
```

Also fill in your Blynk credentials:

```cpp
#define BLYNK_TEMPLATE_ID  "YOUR_TEMPLATE_ID"
#define BLYNK_AUTH_TOKEN   "YOUR_AUTH_TOKEN_HERE"
```

> ⚠️ **Never commit real credentials to a public repo.** Always use placeholder text as shown above.

### WiFi Quick-Switch Logic

If the primary network is unavailable, the ESP32 automatically tries the backup within 8 seconds:

1. On boot, `tryConnect()` attempts `WIFI_SSID1` for 8 seconds.
2. If that fails, it immediately tries `WIFI_SSID2` for 8 seconds.
3. If both fail, the ESP runs in offline mode and retries every 20 seconds.
4. The moment either network is available, Blynk reconnects automatically.

> **Tip:** Set `WIFI_SSID2` to your mobile hotspot name. If your router goes down, enable your hotspot and the ESP32 will connect within 8 seconds.

---

## 7. Offline Mode

The alarm works **without internet**. Here is what happens in each scenario:

| Scenario                          | Relay / Buzzer           | Blynk Dashboard                                   |
|-----------------------------------|--------------------------|---------------------------------------------------|
| WiFi connected, alarm ON          | Triggers on motion       | Notification + LED + label update                 |
| WiFi lost mid-session, alarm ON   | Still triggers on motion | Dashboard shows device offline                    |
| No WiFi at all, alarm ON          | Still triggers on motion | No update (no connection)                         |
| WiFi reconnects                   | Continues normally       | Alarm switch state restored via `syncVirtual(V2)` |

> **How it works:** The relay is driven by `setAlarmOutput()` which calls `digitalWrite(PIN_RELAY, HIGH)` regardless of WiFi state. All `Blynk.virtualWrite()` calls are wrapped in `if (Blynk.connected())` checks and are simply skipped when offline. **The physical siren always works.**

---

## 8. Blynk Mobile Dashboard

Open the Blynk mobile app, tap your device, then tap the pencil icon to add widgets:

| Widget Type         | Pin | Label        | Settings              |
|---------------------|-----|--------------|-----------------------|
| Button              | V2  | Alarm Switch | Mode: Switch          |
| LED                 | V0  | Motion       | Color: Red            |
| Label               | V1  | Status       | Font: Medium          |
| Gauge / SuperChart  | V3  | Chip Temp °C | Min: 0, Max: 100      |
| Label               | V5  | Uptime       | Font: Small           |

> **SuperChart Tip:** Use SuperChart for V3 instead of a plain Gauge to see temperature history over time. Set the time range to **Live** for a real-time scrolling graph.

---

## 9. Getting Started

### Prerequisites

- [Arduino IDE 2.x](https://arduino.cc/en/software)
- ESP32 board support installed via Boards Manager:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- [Blynk library](https://github.com/blynkkk/blynk-library) installed via Library Manager
- A free [Blynk account](https://blynk.cloud)

### Quick Start

1. **Clone this repo:**
   ```bash
   git clone https://github.com/Talha2933/ESP32-PIR-Security-Alarm.git
   ```

2. **Set up Blynk:**
   - Create a template named `Security Alarm` (Hardware: ESP32, Connection: WiFi)
   - Add datastreams for V0–V5 as listed in [Section 5](#5-blynk-virtual-pins)
   - Create a `motion_detected` event (Type: Critical, enable push + email)

3. **Configure credentials** in `SecurityAlarm.ino` (see [Section 6](#6-wifi-setup))

4. **Select your board:** `Tools > Board > ESP32 Dev Module`

5. **Upload** and open Serial Monitor at `115200` baud — you should see `Blynk ONLINE`

6. **Test:** Turn alarm ON via app, wait 5 seconds, walk past the sensor — the relay should fire and a push notification should arrive.

---

## 10. Troubleshooting

| Problem | Solution |
|---------|----------|
| Blynk shows device offline even when ESP is running | Correct behaviour when WiFi drops. `BLYNK_DISCONNECTED()` sends V4=0 before socket closes. Check your router. |
| Temperature shows 53°C at rest | Normal — sensor reads chip die temp, which is 10–20°C above room temperature. |
| No push notification received | Ensure event code in Blynk is exactly `motion_detected` (lowercase, underscore). Enable notifications in the Blynk app settings. |
| PIR triggers immediately when alarm turns ON | Increase `PIR_IGNORE_MS` to `8000` or `10000` in the code if false triggers persist. |
| WiFi 2 not connecting | Confirm `WIFI_SSID2` / `WIFI_PASS2` are correct. SSID is case-sensitive. Ensure DHCP is enabled on that network. |
| COM port not visible in Arduino IDE | Install the CP2102 or CH340 USB driver for your ESP32 board. |
| Upload fails with 'Failed to connect' | Hold the **BOOT** button on the ESP32 while clicking Upload. Release after seeing `Connecting...` in the IDE. |

---

## 11. Security Warning

> ⚠️ **Never upload real credentials to a public repository.**  
> Your Auth Token and WiFi passwords must be replaced with placeholder text before committing. Anyone with your Auth Token can control your device.

---

## 12. License

This project is licensed under the [MIT License](LICENSE).

---

*Made with ❤️ by Fun With Electronics • [YouTube](https://youtube.com/@funwithelectronics273) • [GitHub](https://github.com/Talha2933)*
