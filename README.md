# ESP32-FRD: Autonomous Rescue USV (Unmanned Surface Vehicle)

![Project Status](https://img.shields.io/badge/Status-Active_Development-green)
![License](https://img.shields.io/badge/License-MIT-blue)
![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3-red)

**ESP32-FRD** (Floating Rescue Device) is the firmware implementation for an autonomous surface vehicle designed for rapid water rescue operations. Built on the **ESP32-S3** platform using **ESP-IDF v5.5**, it features hybrid navigation (GPS/IMU fusion), LTE telemetry, and intelligent obstacle avoidance.


---

## 1. System Architecture

The system uses a modular architecture separating hardware abstraction (HAL), sensor fusion, and mission control logic.

### Hardware Specifications

| Component | Model | Interface | Purpose |
| :--- | :--- | :--- | :--- |
| **MCU** | ESP32-S3 DevKitC-1 | - | Central Processing Unit |
| **IMU** | MPU-9250 | I2C | Heading & Motion tracking (9-DOF) |
| **GPS** | NEO-M8N | UART | Geolocation & COG |
| **Modem** | SimCom A7682S | UART | 4G LTE Telemetry & MQTT |
| **Propulsion** | 3650 BLDC + 120A ESC | PWM | Main Thrust (Air prop) |
| **Sensors** | 3x JSN-SR04T | GPIO | Obstacle Avoidance (Waterproof) |
| **Load Cell** | HX711 + 50kg Cell | GPIO | Payload/Rescue Detection |

### Software Stack

- **Framework:** ESP-IDF v5.5 (FreeRTOS based)
- **Communication:** MQTT over TLS 1.2, AT Command Parser (Ring Buffer)
- **Algorithms:** - **Madgwick Filter:** Sensor fusion for stable heading under vibration.
  - **PID Control:** Closed-loop heading hold.
  - **Asymmetric Filter:** "Fast Attack - Slow Decay" logic for ultrasonic noise rejection.

---

## 2. Directory Structure

```text
ESP32-FRD/
├── include/
│   ├── app_config.h     # Pin mapping & System constants
│   ├── drv_motor.h      # ESC Driver (MCPWM/LEDC)
│   ├── drv_ultrasonic.h # JSN-SR04T Driver with Filters
│   └── sys_monitor.h    # Health check (Heap/Stack/Battery)
├── src/
│   ├── main.c           # FreeRTOS Task Scheduler & FSM
│   ├── drv_ultrasonic.c # Hardware implementation
│   ├── drv_motor.c      # ESC Arming & Control
│   └── ...
└── platformio.ini       # Build configuration
````

-----

## 3\. Functional Description (Finite State Machine)

The system operates on a strict FSM to ensure safety:

1.  **STANDBY:** Deep sleep / Low power. Waits for MQTT "WAKE" command.
2.  **DISPATCH:** GPS Lock acquired. Course plotted to target coordinates.
3.  **COURSE\_LOCK:** Long-range navigation using GPS Course Over Ground (COG) to mitigate magnetic interference from the motor.
4.  **FINE\_APPROACH:** Short-range maneuvering using fused Compass/Gyro data + Ultrasonic obstacle avoidance.
5.  **RESCUE:** Triggered when Load Cell detects weight \> 15kg. Motor cuts off immediately to protect the victim.
6.  **RETURN\_TO\_HOME (RTH):** Activated upon mission completion or Failsafe (Signal Loss \> 30s / Low Batt).

-----

## 4\. Key Technical Challenges & Solutions

### A. Sensor Fusion in High EMI Environment

The 120A ESC generates significant electromagnetic interference affecting the magnetometer.

  * **Solution:** Implemented a **Madgwick Filter (100Hz)** fusing Gyroscope and Accelerometer data to compensate for Compass drift during high-throttle events.

### B. Ultrasonic Noise & Water Reflections

Water surface reflections caused "ghost" readings.

  * **Solution:** Custom driver with **Pull-down Resistors** on Echo pins (to prevent floating noise) and an **Asymmetric Filter** that prioritizes close-range obstacles (Fast Attack) while rejecting transient noise (Slow Decay).

### C. Power Management

Motor inrush current caused brownout resets on the ESP32.

  * **Solution:** Isolated power rails using **6N137 Optocouplers** and separate Buck converters for Logic and Power lines.

-----

## 5\. Getting Started

### Wiring

| ESP32 Pin | Component | Function |
| :--- | :--- | :--- |
| GPIO 13 | Motor Left | PWM Signal |
| GPIO 14 | Motor Right | PWM Signal |
| GPIO 17 | Ultrasonic Trig | Output |
| GPIO 16 | Ultrasonic Echo | Input (Pull-down) |
| ... | ... | Check `app_config.h` for full map |

### Build & Flash

1.  Clone the repository:
    ```bash
    git clone [https://github.com/justargldude/ESP32-FRD-Rescue-Hovercraft.git](https://github.com/justargldude/ESP32-FRD-Rescue-Hovercraft.git)
    ```
2.  Configure WiFi/LTE credentials in `menuconfig` or `app_config.h`.
3.  Build and Flash:
    ```bash
    idf.py build
    idf.py -p COMx flash monitor
    ```

-----

## Safety Warning

This device operates high-current motors (120A) in a water environment. Always test the **Failsafe (RTH)** logic in a controlled environment before open-water deployment.

## Author

**Thanh Tung Bui** - [buitung161@gmail.com](mailto:buitung161@gmail.com)

```