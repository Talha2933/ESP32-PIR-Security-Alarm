# Blynk IoT Setup for ESP32 Security Alarm

This project uses **Blynk IoT** to monitor and control the ESP32 motion detection alarm system remotely.

## Step 1 – Create Blynk Template

1. Open the Blynk IoT app.
2. Click **Create Template**.
3. Use the following settings:

Template Name: Security Alarm
Hardware: ESP32
Connection Type: WiFi

After creating the template, copy the **Template ID** and **Auth Token** into the Arduino code.

---

## Step 2 – Create Datastreams

Create the following Virtual Pin datastreams:

### Motion Indicator

Virtual Pin: V0
Data Type: Integer
Min: 0
Max: 1

### Motion Status

Virtual Pin: V1
Data Type: String

### Alarm Control

Virtual Pin: V2
Data Type: Integer
Min: 0
Max: 1

### ESP Status

Virtual Pin: V4
Data Type: Integer
Min: 0
Max: 1

### ESP Uptime

Virtual Pin: V5
Data Type: String

---

## Step 3 – Create Event Notification

Create an event with the following settings:

Event Name: motion_detected
Description: Motion detected in security area
Notification: Enable Push Notification

This event is triggered from the ESP32 code using:

Blynk.logEvent("motion_detected");

---

## Step 4 – Mobile Dashboard Widgets

Add the following widgets to the dashboard:

Button → V2 → Alarm ON/OFF
LED → V0 → Motion Indicator
Label → V1 → Motion Status
LED → V4 → ESP32 Online Status
Label → V5 → ESP32 Uptime

---

## Step 5 – Upload Code to ESP32

After configuring the Blynk template and datastreams, upload the Arduino code to the ESP32.

Make sure to update these fields in the code:

#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

---

## System Features

• PIR Motion Detection
• IoT Notification Alerts
• Remote Alarm Control
• ESP32 Status Monitoring
• WiFi Redundancy Connection
• Real-Time Uptime Display

---
