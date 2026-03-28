/*
 * ================================================================
 *   ESP32 IoT Security Alarm System
 *   Author  : Fun With Electronics
 *   YouTube : https://www.youtube.com/@funwithelectronics273
 *   GitHub  : https://github.com/Talha2933/ESP32-PIR-Security-Alarm.git
 *   Version : 3.0
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
 * ================================================================
 */

// ── Blynk credentials ──────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   "TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Security Alarm"
#define BLYNK_AUTH_TOKEN    "paste_your_token_here"    // <- paste your token here
#define BLYNK_PRINT         Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// ── ESP32 internal temperature sensor (no library needed) ───────
// ROM function — note the intentional SDK typo "temprature"
extern "C" { uint8_t temprature_sens_read(); }

// ── WiFi credentials (primary + fallback) ──────────────────────
const char* WIFI_SSID1 = "Redmi";               // primary WiFi name
const char* WIFI_PASS1 = "funwithelectronics";  // primary password
const char* WIFI_SSID2 = "esp32";               // fallback WiFi name
const char* WIFI_PASS2 = "Password";            // fallback password

// ── Physical pin assignments ────────────────────────────────────
const int PIN_PIR   = 27;   // PIR motion sensor signal pin
const int PIN_RELAY = 22;   // relay / buzzer output
const int PIN_LED   = 2;    // onboard blue LED (status indicator)

// ── Runtime state ───────────────────────────────────────────────
bool motionDetected  = false;
bool alarmActive     = false;
bool blynkWasOnline  = false;   // tracks last known Blynk state

// ── Timing constants (milliseconds) ────────────────────────────
const unsigned long PIR_IGNORE_MS    = 5000;   // ignore PIR for 5s after alarm ON (warm-up)
const unsigned long MOTION_HOLD_MS   = 5000;   // keep relay ON 5s after last motion
const unsigned long WIFI_RETRY_MS    = 8000;   // how long to attempt each WiFi network
const unsigned long WIFI_WATCHDOG_MS = 20000;  // check WiFi health every 20s

// ── Timestamps ──────────────────────────────────────────────────
unsigned long bootTime       = 0;
unsigned long alarmStartTime = 0;
unsigned long lastMotionTime = 0;
unsigned long lastWifiCheck  = 0;

BlynkTimer timer;

// ================================================================
//  OFFLINE BUZZER (works without WiFi)
//
//  When the alarm is active and motion is detected, the relay
//  (GPIO 22) fires regardless of WiFi state. This means the
//  physical buzzer/siren will always sound locally even if the
//  ESP is completely offline.
// ================================================================

// ── Helper: drive relay + onboard LED ───────────────────────────
void setAlarmOutput(bool on) {
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  // Onboard LED blinks fast when alarm triggered offline
  if (on && !blynkWasOnline) digitalWrite(PIN_LED, HIGH);
}

// ================================================================
//  WiFi QUICK-SWITCH
//
//  tryConnect() attempts one network for WIFI_RETRY_MS.
//  connectToWiFi() tries primary first, instantly falls back to
//  secondary if primary fails — no long blocking delays.
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
  Serial.println("[WiFi] Starting connection...");
  WiFi.mode(WIFI_STA);
  if (!tryConnect(WIFI_SSID1, WIFI_PASS1)) {
    if (!tryConnect(WIFI_SSID2, WIFI_PASS2)) {
      Serial.println("[WiFi] Both networks failed. Running in OFFLINE mode.");
      return;
    }
  }
  // WiFi connected — now connect Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(3000);   // 3s non-blocking timeout
}

// ================================================================
//  BLYNK CALLBACKS — event-driven status (not polled)
//
//  BLYNK_CONNECTED   fires the instant auth succeeds
//  BLYNK_DISCONNECTED fires inside Blynk stack while socket is
//                    still alive — this is the only reliable place
//                    to send a "device went offline" message
// ================================================================

BLYNK_CONNECTED() {
  blynkWasOnline = true;
  digitalWrite(PIN_LED, HIGH);
  Blynk.syncVirtual(V2);         // restore alarm switch after reconnect
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
  Serial.println("\n[Boot] ESP32 Security Alarm — Fun With Electronics");
  Serial.println("[Boot] YouTube: @funwithelectronics273\n");

  pinMode(PIN_PIR,   INPUT_PULLDOWN);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED,   OUTPUT);

  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED,   LOW);

  connectToWiFi();
  bootTime = millis();

  // ── Timer schedule (staggered to avoid packet collision) ───
  timer.setInterval(1000L,  sendUptime);        // uptime every 1s   → V5
  timer.setInterval(2000L,  sendTemperature);   // chip temp every 2s → V3 (real-time)
}

// ================================================================
//  MAIN LOOP
// ================================================================

void loop() {

  // ── WiFi watchdog: quick reconnect if connection drops ───────
  if (millis() - lastWifiCheck > WIFI_WATCHDOG_MS) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Lost — attempting reconnect...");
      connectToWiFi();
    }
  }

  if (Blynk.connected()) Blynk.run();
  timer.run();

  // ── Motion detection (runs online AND offline) ───────────────
  if (!alarmActive) return;

  // Skip PIR reading during warm-up window after alarm is turned ON
  if (millis() - alarmStartTime < PIR_IGNORE_MS) return;

  int pirVal = digitalRead(PIN_PIR);

  // Motion START
  if (pirVal == HIGH) {
    lastMotionTime = millis();
    if (!motionDetected) {
      motionDetected = true;
      setAlarmOutput(true);          // relay ON — works offline too
      Serial.println("[Motion] DETECTED");

      if (Blynk.connected()) {
        Blynk.virtualWrite(V0, 1);
        Blynk.virtualWrite(V1, "!! MOTION DETECTED !!");
        // Critical alarm notification (push + email via Blynk event)
        Blynk.logEvent("motion_detected");
      }
    }
  }

  // Motion STOP (hold window expired — no signal for MOTION_HOLD_MS)
  if (motionDetected && millis() - lastMotionTime > MOTION_HOLD_MS) {
    motionDetected = false;
    setAlarmOutput(false);           // relay OFF
    Serial.println("[Motion] CLEARED");

    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, 0);
      Blynk.virtualWrite(V1, "Area Clear");
    }
  }
}

// ================================================================
//  ESP32 INTERNAL TEMPERATURE  →  V3
//
//  temprature_sens_read() returns the raw die temperature in
//  Fahrenheit (this is a quirk of the ESP32 ROM — the value is
//  always Fahrenheit regardless of locale).
//
//  Correct conversion:  °C = (raw_F - 32) / 1.8
//
//  The reading is the CHIP DIE temperature, which runs roughly
//  10–20°C above ambient. A reading of 50–65°C is completely
//  normal when the chip is active. Above 80°C indicates a
//  thermal issue (poor airflow, high load, or hardware fault).
//
//  Sent every 2 seconds for real-time monitoring on dashboard.
// ================================================================

void sendTemperature() {
  if (!Blynk.connected()) return;

  uint8_t rawF = temprature_sens_read();

  // Sensor returns 0 or 255 when not yet ready — skip that reading
  if (rawF == 0 || rawF == 255) {
    Serial.println("[Temp] Sensor not ready, skipping.");
    return;
  }

  // Convert Fahrenheit → Celsius (correct formula for this ROM function)
  float tempC = (float)(rawF - 32) / 1.8f;

  Blynk.virtualWrite(V3, tempC);
  Serial.printf("[Temp] Chip: %.1f C (raw ROM value: %d F)\n", tempC, rawF);
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
//  ALARM SWITCH  (V2 — Button widget in Blynk dashboard)
// ================================================================

BLYNK_WRITE(V2) {
  int state = param.asInt();

  if (state == 1) {
    alarmActive    = true;
    alarmStartTime = millis();
    motionDetected = false;         // clear stale state from previous session
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
