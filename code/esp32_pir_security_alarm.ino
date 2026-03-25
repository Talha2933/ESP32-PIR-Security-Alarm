#define BLYNK_TEMPLATE_ID "TMPL6MAIMdAca"
#define BLYNK_TEMPLATE_NAME "Security Alarm"
#define BLYNK_AUTH_TOKEN "iyW8HjAmaFVYMKciwhbINUr52wjfWJ_d"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// ---------------- WiFi ----------------
const char* ssid1 = "Wifi Name"; // Wifi Name
const char* pass1 = "Password"; //  Password
const char* ssid2 = "Second Wifi Name"; //Second Wifi Name
const char* pass2 = "Password"; //Password

// ---------------- Pins ----------------
const int motionsensor = 27;
const int relay1 = 22;
const int ledPin = 2;

// ---------------- Variables ----------------
bool motionDetected = false;
bool alarmActive = false;
bool wifiConnected = false;

unsigned long startTime;

// 🔥 NEW: Ignore false trigger
unsigned long alarmStartTime = 0;
const unsigned long sensorIgnoreTime = 5000; // 5 sec ignore

// 🔥 NEW: Hold motion for smooth behavior
unsigned long lastMotionTime = 0;
const unsigned long motionHoldTime = 5000; // 5 sec hold

BlynkTimer timer;

// ---------------- Blynk Connected ----------------
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
} 

// ---------------- WiFi ----------------
void connectToWiFi() {
  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid1, pass1);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nTrying second WiFi...");
    WiFi.begin(ssid2, pass2);

    start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      Serial.print(".");
      delay(500);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    wifiConnected = true;
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
  } else {
    Serial.println("\nWiFi Failed");
    wifiConnected = false;
  }
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  pinMode(motionsensor, INPUT_PULLDOWN);
  pinMode(relay1, OUTPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(relay1, LOW);

  connectToWiFi();

  startTime = millis();

  timer.setInterval(1000L, sendUptime);
  timer.setInterval(1000L, updateESPStatus);
}

// ---------------- Loop ----------------
void loop() {
  Blynk.run();
  timer.run();

  // 🔄 Motion System
  if (alarmActive) {

    // 🔥 Ignore first few seconds after ON
    if (millis() - alarmStartTime < sensorIgnoreTime) {
      return;
    }

    int val = digitalRead(motionsensor);

    if (val == HIGH) {
      lastMotionTime = millis();

      if (!motionDetected) {
        motionDetected = true;
        digitalWrite(relay1, HIGH);

        Serial.println("Motion Detected");

        if (Blynk.connected()) {
          Blynk.virtualWrite(V0, 1);
          Blynk.virtualWrite(V1, "Motion Detected");
          Blynk.logEvent("motion_detected");
        }
      }
    }

    // Hold logic (smooth OFF)
    if (motionDetected && (millis() - lastMotionTime > motionHoldTime)) {
      motionDetected = false;
      digitalWrite(relay1, LOW);

      Serial.println("Motion Stopped");

      if (Blynk.connected()) {
        Blynk.virtualWrite(V0, 0);
        Blynk.virtualWrite(V1, "Motion Stopped");
      }
    }
  }
}

// ---------------- ESP Status ----------------
void updateESPStatus() {
  int status = (WiFi.status() == WL_CONNECTED && Blynk.connected()) ? 1 : 0;
  digitalWrite(ledPin, status ? HIGH : LOW);
  Blynk.virtualWrite(V4, status);
}

// ---------------- Uptime ----------------
void sendUptime() {
  unsigned long seconds = (millis() - startTime) / 1000;

  int h = seconds / 3600;
  int m = (seconds % 3600) / 60;
  int s = seconds % 60;

  char timeStr[20];
  sprintf(timeStr, "%02d:%02d:%02d", h, m, s);

  if (Blynk.connected()) {
    Blynk.virtualWrite(V5, timeStr);
  }
}

// ---------------- Alarm Button ----------------
BLYNK_WRITE(V2) {
  int state = param.asInt();

  if (state == 1) {
    alarmActive = true;
    alarmStartTime = millis(); // start ignore timer
    Serial.println("Alarm ON (Stabilizing...)");
  } else {
    alarmActive = false;
    motionDetected = false;
    digitalWrite(relay1, LOW);

    Blynk.virtualWrite(V0, 0);
    Blynk.virtualWrite(V1, "Alarm OFF");

    Serial.println("Alarm OFF");
  }
}
