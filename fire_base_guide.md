# SecureGuard — ESP32 PIR Security System
## Complete Setup Guide

---

## 🛒 HARDWARE REQUIRED

| Component | Notes |
|-----------|-------|
| ESP32 Dev Board | Any 30-pin or 38-pin variant |
| PIR Sensor (HC-SR501) | Adjust sensitivity pot to medium |
| Buzzer (active, 5V) | Louder = better alarm |
| Red LED + 220Ω resistor | Alarm indicator |
| Green LED + 220Ω resistor | Armed indicator |
| Blue LED + 220Ω resistor | WiFi indicator |
| Jumper wires + breadboard | |
| USB cable (for power) | 5V/2A adapter |

## 🔌 WIRING DIAGRAM

```
PIR Sensor HC-SR501:
  VCC  → ESP32 5V (or 3.3V)
  GND  → ESP32 GND
  OUT  → ESP32 GPIO 23

Buzzer:
  (+)  → ESP32 GPIO 18  (through transistor for louder sound)
  (-)  → GND

LEDs (each with 220Ω in series):
  Red    → GPIO 19 → 220Ω → GND
  Green  → GPIO 21 → 220Ω → GND
  Blue   → GPIO 22 → 220Ω → GND
```

**Optional Transistor Circuit (louder buzzer):**
```
GPIO 18 → 1kΩ → NPN Base (2N2222)
Collector → Buzzer (+)
Emitter → GND
Buzzer (-) → 5V
```

---

## ☁️ FREE CLOUD SETUP — Firebase (100% Free)

Firebase Spark Plan (FREE) includes:
- 1 GB Realtime Database storage
- 10 GB/month bandwidth
- FCM push notifications: UNLIMITED
- Hosting: 10 GB storage, 360 MB/day transfer

### Step 1 — Create Firebase Project
1. Go to: https://console.firebase.google.com
2. Click "Add project" → Name it "esp32-security"
3. Disable Google Analytics (optional) → Create project

### Step 2 — Enable Realtime Database
1. Left sidebar → Build → Realtime Database
2. Click "Create Database"
3. Choose location: us-central1 (or nearest)
4. Start in **test mode** (we'll add rules later)
5. Copy your database URL: `https://your-project-default-rtdb.firebaseio.com`

### Step 3 — Get Database Secret (for ESP32)
1. Project Settings (gear icon) → Service accounts
2. Scroll down → Database secrets → Show → Copy

### Step 4 — Get Web App Config
1. Project Settings → General → scroll to "Your apps"
2. Click </> (Web) → Register app
3. Copy the `firebaseConfig` object into `webapp/index.html`

### Step 5 — Enable Cloud Messaging (FCM)
1. Project Settings → Cloud Messaging
2. Copy "Server key" → paste into ESP32 code as `FCM_SERVER_KEY`
3. To get your phone's FCM token, use the Firebase Console test or check your app

### Step 6 — Firebase Security Rules
In Realtime Database → Rules tab, paste:

```json
{
  "rules": {
    "security": {
      ".read": "auth != null",
      ".write": "auth != null",
      "status": {
        ".read": true,
        ".write": "auth != null"
      }
    }
  }
}
```

For testing without auth:
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

---

## 💻 ARDUINO IDE SETUP

### Step 1 — Install ESP32 Board
1. Arduino IDE → Preferences → Additional Board URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Tools → Board Manager → Search "esp32" → Install "esp32 by Espressif"

### Step 2 — Install Libraries (Library Manager)
- `Firebase ESP32 Client` by Mobizt  ← REQUIRED
- `ArduinoJson` by Benoit Blanchon    ← REQUIRED

### Step 3 — Configure the Sketch
Open `esp32_security_system.ino` and fill in:
```cpp
#define WIFI_SSID      "YourWiFiName"
#define WIFI_PASSWORD  "YourPassword"
#define FIREBASE_HOST  "your-project-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH  "your-database-secret-key"
#define FCM_SERVER_KEY "your-fcm-server-key"
```

### Step 4 — Upload
- Board: "ESP32 Dev Module"
- Upload Speed: 115200
- Flash Size: 4MB
- Partition Scheme: Default 4MB with spiffs

---

## 🌐 WEB APP HOSTING (Free — Firebase Hosting)

```bash
# Install Firebase CLI
npm install -g firebase-tools

# Login
firebase login

# Initialize in webapp/ folder
cd webapp
firebase init hosting

# Deploy (FREE, gets HTTPS automatically)
firebase deploy --only hosting
```

Your app will be live at: `https://your-project.web.app`

### Alternative Free Hosts:
- **Netlify** — drag & drop webapp/ folder at netlify.com
- **GitHub Pages** — push to repo, enable Pages
- **Vercel** — `npx vercel` in webapp/ folder

---

## 📱 MOBILE PUSH NOTIFICATIONS SETUP

### For Web (PWA) — Works when screen is locked:
1. Open your hosted web app URL in Chrome (Android)
2. Chrome menu → "Add to Home Screen" → installs as app
3. Open installed app → click "Enable Alerts"
4. Allow notifications when prompted
5. Notifications now appear on lock screen with timestamp!

### Notification Appears On Lock Screen Because:
- Service Worker (`sw.js`) runs in background even when phone is locked
- `requireInteraction: true` — notification stays until dismissed
- `vibrate` pattern set for urgent alert
- Works even when browser is closed

---

## 🔄 OFFLINE MODE

When WiFi is unavailable, the ESP32:
1. Loads last settings from EEPROM (persistent memory)
2. PIR detection still works normally
3. Buzzer + LEDs trigger locally for full alarm duration
4. Auto-reconnects to WiFi every 30 seconds
5. Syncs settings from Firebase when reconnected

---

## 📊 FIREBASE DATABASE STRUCTURE

```
/security/
  ├── status/           ← ESP32 writes here
  │   ├── online: true
  │   ├── system_armed: true
  │   ├── alarm_active: false
  │   ├── motion_detected: false
  │   ├── temperature: 45.2
  │   ├── device_ip: "192.168.1.100"
  │   ├── last_heartbeat: "2024-01-15 14:30:00"
  │   ├── last_motion_time: "..."
  │   └── last_alarm_time: "..."
  │
  ├── control/          ← Web app writes here
  │   ├── system_armed: true
  │   ├── alarm_delay_ms: 3000
  │   ├── alarm_duration_ms: 10000
  │   ├── test_alarm: false
  │   └── stop_alarm: false
  │
  └── logs/             ← Event history
      ├── {timestamp}/
      │   ├── time: "2024-01-15 14:35:22"
      │   ├── event: "ALARM_TRIGGERED"
      │   └── reason: "Motion detected by PIR sensor"
      └── ...
```

---

## 🔧 TROUBLESHOOTING

| Problem | Solution |
|---------|----------|
| No WiFi connection | Check SSID/password, ensure 2.4GHz not 5GHz |
| Firebase not connecting | Verify FIREBASE_HOST (no https://) and AUTH key |
| PIR false triggers | Adjust HC-SR501 sensitivity pot counter-clockwise |
| Temperature reading -40°C | Normal ESP32 internal temp when cold booting |
| Notifications not showing | Check browser permissions, ensure HTTPS hosting |
| Alarm not stopping remotely | Check Firebase write rules, verify stream running |
| Upload fails | Hold BOOT button during upload on some boards |

---

## ⚡ LED STATUS GUIDE

| Color | Pattern | Meaning |
|-------|---------|---------|
| Blue | Solid ON | WiFi connected |
| Blue | Blinking | Connecting to WiFi |
| Green | Solid ON | System armed |
| Green | OFF | System disarmed |
| Red | Flashing | Alarm active |
| Red+Green | Both blink on boot | Startup sequence |

---

## 🔐 SECURITY NOTES

- Change Firebase rules from test mode before deploying publicly
- Use Firebase Authentication to restrict access
- FCM server key should never be exposed in client-side code
- Consider using Firebase App Check for additional security
- Store sensitive keys in environment variables for production

---

*SecureGuard v2.0 — ESP32 + PIR + Firebase | Free Cloud Security System*
