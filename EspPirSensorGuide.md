# ESP32 IoT Security Alarm System

> PIR motion alarm with Blynk IoT push notifications, email alerts, and full offline support.

**Version 3.2 &nbsp;|&nbsp; Blynk IoT &nbsp;|&nbsp; Arduino IDE**

[![YouTube](https://img.shields.io/badge/YouTube-Fun%20With%20Electronics-red?logo=youtube)](https://www.youtube.com/@funwithelectronics273)
[![GitHub](https://img.shields.io/badge/GitHub-Repo-black?logo=github)](https://github.com/Talha2933/ESP32-PIR-Security-Alarm)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Table of Contents

- [Project Overview](#1-project-overview)
- [Blynk IoT Configuration](#2-blynk-iot-configuration)
- [Arduino IDE Setup](#3-arduino-ide-setup)
- [Offline Mode Explained](#4-offline-mode-explained)
- [Troubleshooting](#5-troubleshooting)

---

## 1. Project Overview

This project turns an ESP32 microcontroller into a WiFi-connected PIR motion alarm that you can monitor and control remotely from your phone using the **Blynk IoT** platform. When motion is detected, the relay fires immediately — even without internet — and Blynk sends you a **push notification and email alert**.

### Key Features

- PIR motion detection with configurable hold time (`MOTION_HOLD_MS`)
- Works fully **offline** — relay triggers without WiFi
- **Quick WiFi switch**: tries primary network, falls back to secondary within 8 seconds each
- Non-blocking WiFi + Blynk watchdog — motion detection is never paused during reconnect
- Blynk push notification + email on every motion event
- Real-time ESP32 chip temperature via `temperatureRead()` (Arduino core 3.x, every 2 seconds)
- Live uptime counter (`HH:MM:SS`) on V5
- No external temperature library required

### 1.1 Virtual Pin Map

| Pin | Data Type | Range   | Purpose                        |
|-----|-----------|---------|--------------------------------|
| V0  | Integer   | 0 / 1   | Motion indicator LED widget    |
| V1  | String    | —       | Motion status text label       |
| V2  | Integer   | 0 / 1   | Alarm ON / OFF switch button   |
| V3  | Double    | 0 – 100 | ESP32 chip temperature in °C   |
| V5  | String    | —       | ESP32 uptime (HH:MM:SS)        |

### 1.2 Hardware Required

| Component                        | Qty | Notes                              |
|----------------------------------|-----|------------------------------------|
| ESP32 Dev Board (any variant)    | 1   | 38-pin or 30-pin both work         |
| HC-SR501 PIR Motion Sensor       | 1   | OUTPUT → GPIO 27 (INPUT_PULLDOWN)  |
| 5V Relay Module                  | 1   | Active-HIGH → GPIO 22              |
| Buzzer or Siren (optional)       | 1   | Connect to relay NO terminal       |
| Micro USB Cable + 5V Adapter     | 1   | Minimum 1A rated                   |

---

## 2. Blynk IoT Configuration

Complete these steps in the **Blynk web dashboard** ([blynk.cloud](https://blynk.cloud)) before uploading any code.

### Step 1 — Create a Blynk Account

1. Go to [blynk.cloud](https://blynk.cloud) and click **Sign Up**.
2. Create a free account using your email address.
3. Verify your email when the confirmation message arrives.
4. Log in to the Blynk web dashboard.

### Step 2 — Create a New Template

> **What is a Template?**  
> A template is a reusable blueprint that defines what your device can do. All data streams, events, and dashboard layouts are stored inside it.

5. In the left sidebar click **Developer Zone** → **My Templates**.
6. Click the blue **+ New Template** button.
7. Fill in the form:

   | Field              | Value to Enter                              |
   |--------------------|---------------------------------------------|
   | Template Name      | Security Alarm                              |
   | Hardware           | ESP32                                       |
   | Connection Type    | WiFi                                        |
   | Description        | PIR motion alarm by Fun With Electronics    |

8. Click **Done** to save the template.
9. Copy your **Template ID** shown at the top of the page.
10. Click the key icon to view and copy your **Auth Token**.

> ⚠️ **Important — Save These Credentials**  
> Template ID and Auth Token are secret. Paste them into the `#define` lines at the top of the `.ino` file. **Never share them publicly.**

### Step 3 — Create Datastreams (Virtual Pins)

Inside your template, go to the **Datastreams** tab and create each pin below:

#### V0 — Motion Indicator              #### V1 — Motion Status              #### V2 — Alarm Control Switch       #### V3 — ESP32 Temperature

| Field        | Value            |     | Field        | Value         |     | Field        | Value         |     | Field        | Value            |
|--------------|------------------|     |--------------|---------------|     |--------------|---------------|     |--------------|------------------|
| Virtual Pin  | V0               |     | Virtual Pin  | V1            |     | Virtual Pin  | V2            |     | Virtual Pin  | V3               |
| Name         | Motion Indicator |     | Name         | Motion Status |     | Name         | Alarm Control |     | Name         | Chip Temperature |
| Data Type    | Integer          |     | Data Type    | String        |     | Data Type    | Integer       |     | Data Type    | Double           |
| Min Value    | 0                |                                          | Min Value    | 0             |     | Min Value    | 0                |
| Max Value    | 1                |                                          | Max Value    | 1             |     | Max Value    | 100              |
                                                                                                                  | Units        | °C               |
                                                                                                                  
> 📌 **Note on Temperature Readings**  
> V3 shows ESP32 chip die temperature — not room temperature. A reading of **45–65°C is completely normal** during operation. Only worry if it goes above 80°C.

#### V5 — Uptime

| Field        | Value       |
|--------------|-------------|
| Virtual Pin  | V5          |
| Name         | ESP32 Uptime|
| Data Type    | String      |

### Step 4 — Create the Motion Detected Event

11. Inside your template click the **Events** tab.
12. Click **+ Add New Event**.
13. Fill in the form:

    | Field                 | Value                                      |
    |-----------------------|--------------------------------------------|
    | Event Code            | `motion_detected`                          |
    | Event Name            | Motion Detected                            |
    | Description           | Critical: Motion detected in security area |
    | Type                  | **Critical**                               |
    | Send Push Notification| Yes (enable toggle)                        |
    | Send Email            | Yes (enable toggle)                        |

14. Click **Save**.

> 💡 Setting the event type to **Critical** causes Blynk to display a prominent red alarm banner in the mobile app and makes the notification bypass silent mode on most phones.

### Step 5 — Build the Mobile Dashboard

Open the Blynk mobile app, tap your device, then tap the **edit (pencil) icon** to add widgets:

| Widget Type       | Pin | Label        | Settings              |
|-------------------|-----|--------------|-----------------------|
| Button            | V2  | Alarm Switch | Mode: Switch          |
| LED               | V0  | Motion       | Color: Red            |
| Label             | V1  | Status       | Font: Medium          |
| Gauge/SuperChart  | V3  | Chip Temp °C | Min: 0, Max: 100      |
| Label             | V5  | Uptime       | Font: Small           |

> 💡 **SuperChart Tip**: Use SuperChart for V3 to see temperature history over time. Set the time range to **Live** for a real-time scrolling graph.

---

## 3. Arduino IDE Setup

### Step 1 — Install Arduino IDE

1. Download Arduino IDE 2.x from: [arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Run the installer and follow the on-screen steps.
3. Launch Arduino IDE after installation.

### Step 2 — Add ESP32 Board Support

4. Go to **File → Preferences** (Windows/Linux) or **Arduino IDE → Preferences** (Mac).
5. Find the field **Additional boards manager URLs** and paste:

   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

6. Click **OK**.
7. Go to **Tools → Board → Boards Manager**.
8. Search for `esp32`, find the entry by **Espressif Systems**, and click **Install**.

### Step 3 — Install Blynk Library

9. Go to **Sketch → Include Library → Manage Libraries**.
10. Search for `Blynk`, find **Blynk by Volodymyr Shymanskyy**, and click **Install**.

### Step 4 — Configure and Upload the Code

11. Open `SecurityAlarm.ino` in Arduino IDE.
12. Find these lines near the top and update them with your credentials:

    ```cpp
    #define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"    // paste your Template ID
    #define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"      // paste your Auth Token

    const char* WIFI_SSID1 = "YOUR_WIFI_NAME";         // your primary WiFi name
    const char* WIFI_PASS1 = "YOUR_WIFI_PASSWORD";     // your primary WiFi password
    const char* WIFI_SSID2 = "YOUR_BACKUP_WIFI_NAME";  // backup WiFi (e.g. mobile hotspot)
    const char* WIFI_PASS2 = "YOUR_BACKUP_PASSWORD";
    ```

13. Plug the ESP32 into your computer via Micro USB.
14. Go to **Tools → Board** and select **ESP32 Dev Module**.
15. Go to **Tools → Port** and select your COM port (e.g. `COM3` on Windows, `/dev/ttyUSB0` on Linux).
16. Click **Upload** or press `Ctrl+U`.
17. Open **Serial Monitor** (baud rate: `115200`) — you should see `Blynk ONLINE`.

### Step 5 — Test the System

18. Open the Blynk app and tap your device dashboard.
19. Tap **Alarm Switch** to turn the alarm **ON**.
20. Wait 5 seconds (PIR warm-up), then walk past the sensor.
21. The Motion LED should turn red and a push notification should arrive.
22. Turn off your WiFi router — the relay should **still** trigger on motion (offline mode).

---

## 4. Offline Mode Explained

The alarm works **completely without internet**. Here is exactly what happens in each scenario:

| Scenario                              | Relay / Buzzer           | Blynk Dashboard                         |
|---------------------------------------|--------------------------|-----------------------------------------|
| WiFi connected, alarm ON              | Triggers on motion       | Notification + LED + label update       |
| WiFi lost mid-session, alarm ON       | Still triggers on motion | Dashboard shows device offline          |
| No WiFi at all, alarm ON              | Still triggers on motion | No update (no connection)               |
| WiFi reconnects                       | Continues normally       | Alarm switch state restored via `syncVirtual(V2)` |

> **How Offline Works in the Code**  
> The relay is driven by `setAlarmOutput()` which calls `digitalWrite(PIN_RELAY, HIGH)` regardless of WiFi state. All `Blynk.virtualWrite()` calls are wrapped in `if (Blynk.connected())` checks so they are skipped when offline. The physical siren always works.

### WiFi Quick-Switch Logic

If the primary WiFi is unavailable, the ESP32 automatically tries the backup network. Each attempt times out after **8 seconds** (`WIFI_RETRY_MS`):

1. `connectToWiFi()` calls `tryConnect(WIFI_SSID1, WIFI_PASS1)` — waits up to 8 s.
2. If that fails, immediately tries `tryConnect(WIFI_SSID2, WIFI_PASS2)` — waits up to 8 s.
3. If both fail, the ESP runs in offline mode. A **non-blocking watchdog** retries every 10 seconds (`WIFI_WATCHDOG_MS`) without pausing motion detection.
4. The moment either network is available, the ESP reconnects, calls `Blynk.config()` + `Blynk.connect()` fresh, and Blynk resumes.

> 💡 **Tip**: Set `ssid2` to your **mobile hotspot**. If your home router goes down, enable your hotspot with the name and password set in `ssid2` — the ESP32 will connect automatically.

---

## 5. Troubleshooting

| Problem | Solution |
|---------|----------|
| Blynk shows device offline even when ESP is running | This is expected when WiFi drops. The `BLYNK_DISCONNECTED()` callback fires and sets the LED low. The watchdog will reconnect within 10 s. Check your router. |
| Temperature shows 53°C or similar at rest | This is normal. `temperatureRead()` returns chip die temperature which is 10–20°C above room temperature. Not broken. |
| `temperatureRead()` causes compile error | You are on ESP32 Arduino core older than 3.x. Update via **Boards Manager** to the latest Espressif Systems release. |
| No push notification received | Check that the event code in Blynk exactly matches `motion_detected` (lowercase, underscore). Enable notifications in the Blynk app settings. |
| PIR triggers immediately when alarm turns ON | The 5-second warm-up window (`PIR_IGNORE_MS`) handles this. If false triggers persist, increase it to `8000` or reduce sensitivity on the HC-SR501 potentiometer. |
| WiFi 2 not connecting | Confirm `WIFI_SSID2` and `WIFI_PASS2` are correct. SSID is case-sensitive. Ensure the second network has DHCP enabled. |
| COM port not visible in Arduino IDE | Install the CP2102 or CH340 USB driver for your ESP32 board. Search for your board model + USB driver online. |
| Upload fails with 'Failed to connect' | Hold the **BOOT** button on the ESP32 while clicking Upload. Release after you see `Connecting...` in the IDE. |

---

## Credits

Made with ❤️ by **Fun With Electronics**

- 📺 YouTube: [@funwithelectronics273](https://www.youtube.com/@funwithelectronics273)
- 💻 GitHub: [Talha2933](https://github.com/Talha2933)

---

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.
