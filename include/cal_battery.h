/**
 * @file cal_battery.h
 * @brief Battery monitoring system for 2S Li-Po (7.0V - 8.4V)
 * @author Your Name
 * @version 1.0
 */

#ifndef CAL_BATTERY_H
#define CAL_BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// --- CÁC NGƯỠNG PIN (Public Constants) ---
#define BATTERY_MAX_V   8.4f  ///< Fully charged (4.2V/cell × 2)
#define BATTERY_MIN_V   7.0f  ///< Low battery (3.5V/cell × 2)
#define BATTERY_CRIT_V  6.6f  ///< Critical - must land NOW (3.3V/cell × 2)

// --- KHAI BÁO HÀM (PUBLIC API) ---

/**
 * @brief Initialize ADC, calibration and battery monitoring system
 * 
 * Must be called once before using other battery functions.
 * Configures ADC1 Channel 3 (GPIO4) with 12-bit resolution.
 * 
 * @note This function will abort if ADC initialization fails
 */
void battery_init(void);

/**
 * @brief Read battery voltage with noise filtering
 * 
 * This function:
 * - Reads ADC 10 times and averages
 * - Applies calibration (if available)
 * - Calculates real voltage through voltage divider
 * - Applies EMA filter for smooth readings
 * 
 * @return Battery voltage in Volts (float)
 * @retval 0.0f if ADC not initialized (shouldn't happen with proper init)
 */
float battery_get_voltage(void);

/**
 * @brief Calculate battery percentage from voltage
 * 
 * Uses linear interpolation between BATTERY_MIN_V and BATTERY_MAX_V.
 * For more accurate results, consider using a LUT (Look-Up Table).
 * 
 * @return Battery percentage 0.0 - 100.0%
 * @note Returns 0.0% if voltage < BATTERY_MIN_V
 * @note Returns 100.0% if voltage >= BATTERY_MAX_V
 */
float battery_get_percentage(void);

/**
 * @brief Check battery health and log warnings
 * 
 * Logs different severity levels:
 * - ERROR: Voltage < BATTERY_CRIT_V (6.6V) - CRITICAL!
 * - WARN:  Voltage < BATTERY_MIN_V (7.0V) - Low battery
 * - INFO:  Normal operation
 * 
 * Should be called periodically (recommended: every 2-5 seconds)
 */
void battery_check_health(void);

/**
 * @brief Get raw ADC value (for debugging)
 * 
 * @return Raw ADC reading (0-4095 for 12-bit)
 * @note This is mainly for debugging/calibration purposes
 */
int battery_get_raw_adc(void);

#ifdef __cplusplus
}
#endif

#endif // CAL_BATTERY_H