# ESP32-FRD: Autonomous Floating Rescue Hovercraft

This project implements the firmware for an autonomous Unmanned Surface Vehicle (USV) designed for water rescue operations. The system is built on the ESP32-S3 platform using ESP-IDF and FreeRTOS, featuring hybrid navigation (GPS and IMU sensor fusion) and LTE-based telemetry.

## System Architecture

The architecture follows a modular structure separating hardware abstraction, sensor processing, communication layers, and high-level application control.

### Hardware Specifications

| Component | Model | Interface |
| :--- | :--- | :--- |
| MCU | ESP32-S3 DevKitC-1 | - |
| IMU | MPU-9250 | I2C |
| GPS | NEO-M8N | UART |
| Modem | SimCom A7682S | UART |
| Motor | 3650 Brushless + 120A ESC | PWM |
| Sensors | JSN-SR04T, HX711 | GPIO |

### Software Stack

- Framework: ESP-IDF v5.x  
- RTOS: FreeRTOS  
- Build System: CMake  
- Communication: MQTT over TLS 1.2, AT Commands  
- Control Algorithms: PID (heading), Madgwick filter (sensor fusion)

## Directory Structure

```text
ESP32-FRD/
├── include/
│   ├── cal_battery.h    # Battery monitor interfaces
│   └── sys_monitor.h    # System health diagnostics
├── src/
│   ├── main.c           # Application entry point
│   ├── cal_battery.c    # ADC handling, calibration & EMA filter
│   └── sys_monitor.c    # Heap & Stack runtime diagnostics
├── lib/                 # Private libraries (PlatformIO standard)
└── platformio.ini       # Project configuration
```

## Functional Description

The firmware uses a Finite State Machine (FSM) with the following states:

1. STANDBY  
   The system enters deep sleep. GPS VBAT is preserved. The 4G modem runs in eDRX mode.

2. DISPATCH  
   Triggered by an MQTT command containing target coordinates. The system initializes sensors and computes the initial heading.

3. COURSE_LOCK  
   Long-distance navigation using GPS Course Over Ground to avoid magnetic disturbance from the motor.

4. FINE_APPROACH  
   Close-range navigation using fused Compass and Gyroscope data via the Madgwick filter. Ultrasonic obstacle detection is active.

5. RESCUE  
   Motor cutoff occurs when load cell threshold exceeds 15 kg. Rescue status is reported to the backend.

6. RETURN_TO_HOME (RTH)  
   Activated after mission completion or failsafe (signal loss longer than 30 s or battery voltage below 14.0 V).

## Technical Implementation Details

### 1. Sensor Fusion

EMI from the 120A ESC affects magnetometer readings. A Madgwick filter operating at 100 Hz combines data from the accelerometer, gyroscope, and magnetometer to produce stable Euler angles during high vibration.

### 2. LTE Connectivity

A custom AT command parser with ring buffer handling is implemented to manage asynchronous UART data from the A7682S. MQTT communication is secured with TLS.

### 3. Power Management

To prevent brownout resets caused by motor inrush current, logic circuits and the propulsion system are isolated on different power rails with optocouplers (6N137) between control and ESC paths.

###  4. System Health Monitoring

Real-time diagnostics module (sys_monitor) tracks Heap Memory fragmentation and Watermark levels to prevent stack overflows. Battery voltage is monitored via a calibrated ADC with an Exponential Moving Average (EMA) filter to eliminate noise from motor voltage sag.

## Getting Started

### Prerequisites

- ESP-IDF v5.0 or later  
- VS Code with Espressif extension  
- Python 3.8+

### Build & Flash

1. Clone the repo:
    
    ```bash
    git clone <https://github.com/justargldude/ESP32-FRD-Rescue-Hovercraft.git>
    
    ```
    
2. Configuration:
Set your 4G APN and MQTT Broker credentials in `app_config.h` or via menuconfig.
3. Build:
    
    ```bash
    idf.py build
    
    ```
    
4. Flash:
    
    ```bash
    idf.py -p COMx flash monitor
    
    ```
    

---

## 6. Future Improvements (Roadmap)

- Implement Over-The-Air (OTA) firmware updates via 4G.
- Add SD Card logging (Blackbox) for mission replay.
- Upgrade to Kalman Filter for better position estimation.

---

## Contribution

- Author: Thanh Tung Bui
- Contact: [buitung161@gmail.com](mailto:buitung161@gmail.com)

---
