# ESP32-FRD-Rescue-Hovercraft

This project implements an autonomous floating rescue device (FRD) built on the ESP32-S3 platform. The system is designed to operate as a fast-response hovercraft capable of locating a drowning victim, navigating to the target, confirming rescue, and returning to the starting point. The architecture follows a three-tier IoT model: Device, Cloud, and Client.

## 1. System Overview

The FRD unit is equipped with onboard sensors, a GPS module, a digital compass, an ultrasonic module, a load cell, a 4G communication module, and a brushless propulsion system. A shore-based camera system performs AI detection and sends navigation updates to the hovercraft. The ESP32-S3 acts as the central controller, executing navigation phases and managing power.

The primary goals of the project are:

* Reliable autonomous operation in water environments
* Minimal launch time through sensor and modem optimization
* Stable communication with a cloud backend
* Real-time monitoring and manual override from a web or mobile client

## 2. Hardware Components

* ESP32-S3 (central controller)
* 4G module A7682S
* GPS NEO-M8 (SMA antenna)
* Compass MPU-9250
* Load cell + HX711 amplifier
* Ultrasonic module JSN-SR04T
* ESC Flycolor 120A
* Brushless motor 3650 2300KV
* LiPo 2s 5C
* LiPo 4S 6000mAh 150C
* Step-down power modules

## 3. Operating Phases

The system operates through six sequential phases:

1. Standby
   The device remains in deep sleep to conserve power. GPS VBAT and the 4G module remain in low-power modes for fast wake-up.

2. Alert and Dispatch
   The shore-side AI camera detects a drowning person, computes the bearing, and sends the initial command to the FRD.

3. Coarse Approach
   The device navigates toward the target bearing using compass feedback. Speed is kept high. GPS updates are processed every few seconds.

4. Fine Approach
   Navigation commands are received continuously from the camera server. Speed is reduced for accuracy.

5. Rescue Confirmation
   Load cell and ultrasonic readings are used to confirm that the victim is holding onto the device.

6. Return Home
   The FRD returns to the initial point using GPS guidance.

## 4. Software Architecture

The firmware is written using ESP-IDF and FreeRTOS. Major tasks include:

* Sensor acquisition task
* Motor control and ESC handling
* 4G communication (MQTT/HTTP)
* GPS and compass fusion
* System watchdog and fault handling
* Power optimization and sleep management

The backend service (Node.js or FastAPI) receives device telemetry, logs mission data, and forwards commands. The client application (ReactJS or React Native) provides real-time monitoring, control, and mission playback.

## 5. Communication Model

The system follows an IoT three-tier model:

* Device Layer: ESP32-S3 publishes sensor data and receives commands.
* Cloud Layer: Backend server processes messages, stores logs, and performs routing.
* Client Layer: Web/mobile interface for monitoring and control.

Primary protocols:

* MQTT for telemetry and instruction updates
* HTTP for critical commands or mission start signals

## 6. Development Goals

The project is developed as a full-stack IoT system demonstrating:

* Embedded programming with ESP32-S3
* Real-time sensor fusion and motor control
* Cloud backend design
* Client-side visualization and control
* Optimization for power and response time
