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


#define FRONT_ULTRASONIC_TRIG 12
#define LEFT_ULTRASONIC_TRIG 10
#define RIGHT_ULTRASONIC_TRIG 3

#define FRONT_ULTRASONIC_ECHO 11
#define LEFT_ULTRASONIC_ECHO 9
#define RIGHT_ULTRASONIC_ECHO 8

#define PD_SCK_FRONT 17
#define DOUT_FRONT 18

#define PD_SCK_LEFT  15
#define DOUT_LEFT    16

#define PD_SCK_RIGHT 7
#define DOUT_RIGHT   6

// ==========================================================
// 3. LOADCELL CALIBRATION VALUES
// Run CALIBRATION_MODE to obtain these values
// ==========================================================
#define SCALE_FRONT   1.0f    // TODO: Replace with actual value after calibration
#define SCALE_LEFT    1.0f    // TODO: Replace with actual value after calibration  
#define SCALE_RIGHT   1.0f    // TODO: Replace with actual value after calibration

// Operation Mode Control
// 1: Run Calibration Wizard (Blocks normal operation to find scale factors)
// 0: Normal Rescue Mode (Runs detection logic with current config)
#define CALIBRATION_MODE        1

// Reference Weight for Calibration
// Unit: Grams (g)
// MUST match the exact weight of the object placed on sensors during calibration
#define CALIBRATION_WEIGHT_G    199.0f

// ==========================================================
// 2. POWER SYSTEM (2S LiPo)
#define BATTERY_MAX_V   8.4f  ///< Fully charged (4.2V/cell × 2)
#define BATTERY_MIN_V   7.0f  ///< Low battery (3.5V/cell × 2)
#define BATTERY_CRIT_V  6.6f  ///< Critical - must land NOW (3.3V/cell × 2)

// ==========================================================


// Motor Speed Scale
// Convention: 1.00% = 100 units
// Example: 50.5% speed = 5050
#define MOTOR_SCALE_FACTOR      100
#define MOTOR_SPEED_MAX_RAW     (100 * MOTOR_SCALE_FACTOR) // = 10000


#ifdef __cplusplus
}
#endif

#endif // APP_CONFIG_H