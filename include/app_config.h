/**
 * @file app_config.h
 * @brief Central Configuration for ESP32-FRD Rescue Hovercraft
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// ==========================================================
// 1. PIN MAPPING (GPIO ASSIGNMENT)
// ==========================================================
// Check schematic before changing!

// Motor (ESC)
#define PIN_MOTOR_LEFT          13
#define PIN_MOTOR_RIGHT         14

// ADC (Battery Monitor)
// Note: GPIO 4 maps to ADC1 Channel 3
#define PIN_BATTERY_ADC         4   
#define ADC_CHANNEL     ADC_CHANNEL_3 

// ==========================================================
// 2. POWER SYSTEM (2S LiPo)
#define BATTERY_MAX_V   8.4f  ///< Fully charged (4.2V/cell × 2)
#define BATTERY_MIN_V   7.0f  ///< Low battery (3.5V/cell × 2)
#define BATTERY_CRIT_V  6.6f  ///< Critical - must land NOW (3.3V/cell × 2)

// ==========================================================


// Motor Speed Scale
// Quy ước: 1.00% = 100 units.
// Ví dụ: Muốn chạy 50.5%, truyền vào 5050.
#define MOTOR_SCALE_FACTOR      100
#define MOTOR_SPEED_MAX_RAW     (100 * MOTOR_SCALE_FACTOR) // = 10000


#ifdef __cplusplus
}
#endif

#endif // APP_CONFIG_H