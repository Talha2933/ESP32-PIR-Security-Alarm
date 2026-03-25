# ESP32 PIR Security Alarm System

This project is a smart security alarm system using ESP32 and PIR motion sensor.  
The system detects motion and triggers an alarm using Blynk IoT.

## Features

- Motion detection using PIR sensor
- Alarm control from Blynk mobile app
- WiFi automatic reconnect
- False trigger protection
- Motion hold logic
- ESP32 uptime monitoring

## Hardware Components

- ESP32
- PIR Motion Sensor
- Relay Module
- LED
- Buzzer (optional)

## Pin Configuration

PIR Sensor -> GPIO 27  
Relay -> GPIO 22  
Status LED -> GPIO 2  

## Blynk Virtual Pins

V0 → Motion indicator  
V1 → Motion status text  
V2 → Alarm ON/OFF button  
V4 → ESP32 online status  
V5 → ESP32 uptime  

## WiFi Setup

Update WiFi credentials in the code:

```cpp
const char* ssid1 = "YourWiFi";
const char* pass1 = "Password";
