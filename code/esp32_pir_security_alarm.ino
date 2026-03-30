/*
 * ================================================================
 *   ESP32 IoT Security Alarm System
 *   Author  : Fun With Electronics
 *   YouTube : https://www.youtube.com/@funwithelectronics273
 *   GitHub  : https://github.com/Talha2933
 *   Version : 3.2
 *   Platform: ESP32 + Blynk IoT
 * ================================================================
 *
 *  Virtual Pins
 *  ────────────
 *  V0  Motion Indicator LED    (Integer  0/1)
 *  V1  Motion Status Text      (String)
 *  V2  Alarm ON/OFF Switch     (Integer  0/1)
 *  V3  ESP32 Chip Temperature  (Double   °C)
 *  V5  ESP32 Uptime            (String   HH:MM:SS)
 *
 *  Physical Pins
 *  ─────────────
 *  GPIO 27  PIR Motion Sensor (INPUT_PULLDOWN)
 *  GPIO 22  Relay / Buzzer    (OUTPUT)
 *  GPIO 2   Onboard LED       (OUTPUT)
 *
 *  */

// ── Blynk credentials ──────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   "BLYNK_TEMPLATE_ID"   // <- paste your TEMPLATE ID here
#define BLYNK_TEMPLATE_NAME "Security Alarm"      // <- paste your TEMPLATE NAME here
#define BLYNK_AUTH_TOKEN    "BLYNK_AUTH_TOKEN"    // <- paste your token here
#define BLYNK_PRINT         Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <esp_chip_info.h>   // provides temperatureRead() in core 3.x+
                             // (replaces the removed temprature_sens_read ROM hack)

// ── WiFi credentials (primary + fallback) ──────────────────────
const char* WIFI_SSID1 = "Redmi";                     // primary WiFi name
const char* WIFI_PASS1 = "funwithelectronics";        // primary password
const char* WIFI_SSID2 = "YOUR_BACKUP_WIFI_NAME";               // fallback WiFi name
const char* WIFI_PASS2 = "YOUR_BACKUP_PASSWORD";               // fallback password

// ── Physical pin assignments ────────────────────────────────────
const int PIN_PIR   = 27;    // Gpio 27 Pir Sensor Pin
const int PIN_RELAY = 22;    // 5v Relay Pin 
const int PIN_LED   = 2;     // Buildin LED Pin Esp32

// ── Runtime state ───────────────────────────────────────────────
bool motionDetected  = false;
bool alarmActive     = false;
bool blynkWasOnline  = false;
bool blynkConfigured = false;   // ← NEW: tracks if Blynk.config() has been called
                                //   after the current WiFi session

// ── Timing constants (milliseconds) ────────────────────────────
const unsigned long PIR_IGNORE_MS    = 5000;
const unsigned long MOTION_HOLD_MS   = 5000;
const unsigned long WIFI_RETRY_MS    = 8000;
const unsigned long WIFI_WATCHDOG_MS = 10000;  // check every 10s (was 20s)

// ── Timestamps ──────────────────────────────────────────────────
unsigned long bootTime       = 0;
unsigned long alarmStartTime = 0;
unsigned long lastMotionTime = 0;
unsigned long lastWifiCheck  = 0;

BlynkTimer timer;

// ================================================================
//  HELPER: drive relay + onboard LED
// ================================================================

void setAlarmOutput(bool on) {
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  if (on && !blynkWasOnline) digitalWrite(PIN_LED, HIGH);
}

// ================================================================
//  WiFi CONNECT
//
//  tryConnect()   — attempts one network for WIFI_RETRY_MS
//  connectToWiFi() — tries primary, falls back to secondary
//
//  KEY FIX: blynkConfigured is reset to false whenever WiFi
//  drops. When WiFi comes back up, Blynk.config() + Blynk.connect()
//  are called fresh. Without this, Blynk never reconnects after
//  a router restart because the socket is permanently closed.
// ================================================================

bool tryConnect(const char* ssid, const char* pass) {
  Serial.printf("  Trying: %s ... ", ssid);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_RETRY_MS) {
      Serial.println("FAILED");
      return false;
    }
    delay(200);
  }
  Serial.printf("CONNECTED (%s)\n", WiFi.localIP().toString().c_str());
  return true;
}

void connectToWiFi() {
  Serial.println("[WiFi] Connecting...");
  WiFi.mode(WIFI_STA);

  bool wifiOK = tryConnect(WIFI_SSID1, WIFI_PASS1);
  if (!wifiOK) wifiOK = tryConnect(WIFI_SSID2, WIFI_PASS2);

  if (!wifiOK) {
    Serial.println("[WiFi] Both networks failed. Running OFFLINE.");
    blynkConfigured = false;   // reset so we retry Blynk next time WiFi returns
    return;
  }

  // ── WiFi is up → (re)configure and connect Blynk ─────────────
  // This must happen every time WiFi reconnects, not just at boot.
  // Calling Blynk.config() again after a drop is safe and required.
  Serial.println("[Blynk] Configuring...");
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(3000);
  blynkConfigured = true;
}

// ================================================================
//  BLYNK CALLBACKS
// ================================================================

BLYNK_CONNECTED() {
  blynkWasOnline = true;
  digitalWrite(PIN_LED, HIGH);
  Blynk.syncVirtual(V2);
  Serial.println("[Blynk] ONLINE");
}

BLYNK_DISCONNECTED() {
  blynkWasOnline = false;
  digitalWrite(PIN_LED, LOW);
  Serial.println("[Blynk] OFFLINE");
}

// ================================================================
//  SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n[Boot] ESP32 Security Alarm v3.1");
  Serial.println("[Boot] YouTube: @funwithelectronics273\n");

  pinMode(PIN_PIR,   INPUT_PULLDOWN);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED,   OUTPUT);

  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED,   LOW);

  connectToWiFi();
  bootTime = millis();

  timer.setInterval(1000L,  sendUptime);
  timer.setInterval(2000L,  sendTemperature);
}

// ================================================================
//  MAIN LOOP
// ================================================================

void loop() {

  // ── WiFi + Blynk watchdog ────────────────────────────────────
  //
  //  Runs every WIFI_WATCHDOG_MS (10s). Non-blocking — uses
  //  millis() so motion detection is never paused.
  //
  //  Two failure cases handled:
  //  1. WiFi dropped  → reconnect WiFi, then reconfigure Blynk
  //  2. WiFi OK but Blynk socket died (e.g. server timeout)
  //     → call Blynk.connect() again without redoing WiFi
  //
  if (millis() - lastWifiCheck > WIFI_WATCHDOG_MS) {
    lastWifiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      // ── Case 1: WiFi is down ──────────────────────────────────
      Serial.println("[WiFi] Disconnected — reconnecting...");
      blynkConfigured = false;   // force full Blynk re-init on reconnect
      connectToWiFi();

    } else if (!Blynk.connected()) {
      // ── Case 2: WiFi up but Blynk socket closed ───────────────
      // This happens after a router restart where the ESP kept its
      // IP but Blynk's TCP connection was dropped server-side.
      Serial.println("[Blynk] Socket lost — reconnecting Blynk...");
      Blynk.config(BLYNK_AUTH_TOKEN);
      Blynk.connect(3000);
    }
  }

  if (Blynk.connected()) Blynk.run();
  timer.run();

  // ── Motion detection (works online AND offline) ──────────────
  if (!alarmActive) return;
  if (millis() - alarmStartTime < PIR_IGNORE_MS) return;

  int pirVal = digitalRead(PIN_PIR);

  if (pirVal == HIGH) {
    lastMotionTime = millis();
    if (!motionDetected) {
      motionDetected = true;
      setAlarmOutput(true);
      Serial.println("[Motion] DETECTED");

      if (Blynk.connected()) {
        Blynk.virtualWrite(V0, 1);
        Blynk.virtualWrite(V1, "!! MOTION DETECTED !!");
        Blynk.logEvent("motion_detected");
      }
    }
  }

  if (motionDetected && millis() - lastMotionTime > MOTION_HOLD_MS) {
    motionDetected = false;
    setAlarmOutput(false);
    Serial.println("[Motion] CLEARED");

    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, 0);
      Blynk.virtualWrite(V1, "Area Clear");
    }
  }
}

// ================================================================
//  ESP32 INTERNAL TEMPERATURE  →  V3
// ================================================================

void sendTemperature() {
  if (!Blynk.connected()) return;

  // temperatureRead() is the official API in ESP32 Arduino core 3.x+
  // Returns chip die temperature directly in °C — no conversion needed.
  // The old temprature_sens_read() ROM function was removed in core 3.x.
  float tempC = temperatureRead();

  // Sanity check — discard obviously invalid readings
  if (tempC < -40.0f || tempC > 125.0f) {
    Serial.println("[Temp] Reading out of range, skipping.");
    return;
  }

  Blynk.virtualWrite(V3, tempC);
  Serial.printf("[Temp] Chip: %.1f C\n", tempC);
}

// ================================================================
//  UPTIME  →  V5
// ================================================================

void sendUptime() {
  if (!Blynk.connected()) return;
  unsigned long s = (millis() - bootTime) / 1000;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", s / 3600, (s % 3600) / 60, s % 60);
  Blynk.virtualWrite(V5, buf);
}

// ================================================================
//  ALARM SWITCH  →  V2
// ================================================================

BLYNK_WRITE(V2) {
  int state = param.asInt();

  if (state == 1) {
    alarmActive    = true;
    alarmStartTime = millis();
    motionDetected = false;
    Serial.println("[Alarm] ON — PIR warm-up 5s...");
    if (Blynk.connected()) {
      Blynk.virtualWrite(V1, "Alarm Armed — Stabilizing...");
    }
  } else {
    alarmActive    = false;
    motionDetected = false;
    setAlarmOutput(false);
    Serial.println("[Alarm] OFF");
    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, 0);
      Blynk.virtualWrite(V1, "Alarm Disarmed");
    }
  }
}
