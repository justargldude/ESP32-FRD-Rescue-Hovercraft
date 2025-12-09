/**
 * @file drv_ultrasonic.h
 * @brief JSN-SR04T (Mode 0) Ultrasonic Driver Interface
 * @details 
 * This driver uses the Polling method to measure pulse width.
 * Measurement units are standardized to mm (millimeters) using uint16_t.
 */

#ifndef DRV_ULTRASONIC_H
#define DRV_ULTRASONIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "esp_err.h"

/**
 * @brief Spike Rejection Threshold (Chống nhiễu gai)
 * @note If the new value jumps more than this amount (mm) compared to the 
 * last valid value, it is considered a potential "spike" (noise).
 */
#define FILTER_SPIKE_THRESHOLD_MM   500    // Max allowed jump in mm (50cm)

/**
 * @brief Spike Persistence Tolerance
 * @note How many consecutive "spikes" are needed to confirm a real movement.
 * - 3 samples ~ 200ms delay: Good balance between stability and response.
 */
#define FILTER_SPIKE_TOLERANCE      3      // Consecutive spikes before accepting

/**
 * @brief Sensor Error Limit (Failsafe)
 * @note If the sensor returns errors (0xFFFF) for this many consecutive times,
 * the system assumes a hard failure or blind zone lock-up.
 */
#define FILTER_ERROR_LIMIT          10     // Max consecutive errors before reset

/**
 * @brief Safe Distance Fallback
 * @note Value returned when the Failsafe triggers. Forces the vehicle to stop/slow down.
 */
#define FILTER_SAFE_DISTANCE_MM     300    // Fallback safe distance (30cm)

/**
 * @brief Blind Zone Detection
 * @note JSN-SR04T creates false long-distance readings (>2m) when objects are very close (<20-30cm). This logic traps that anomaly.
 */
#define US_BLIND_ZONE_THRESHOLD_MM  300    // Only check if we were previously close (<30cm)
#define US_BLIND_ZONE_JUMP_MM       2500   // If it suddenly jumps >2.5m, ignore it

/**
 * @brief Timing Constraints
 * @note Adjusted for JSN-SR04T characteristics.
 */
#define US_PULSE_TIMEOUT_US         35000  // 35ms wait for echo pulse (~6m max range)
#define US_ECHO_WAIT_TIMEOUT_US     20000  // 20ms wait for echo pin to go HIGH

/**
 * @brief Error Code
 * @note Represents a failed measurement (Timeout, Out of Range, etc.)
 */
#define US_ERROR_CODE   0xFFFF 

/**
 * @brief Filter Context Structure
 * @details
 * This struct stores the "health record" of each sensor for filtering purposes.
 * - last_valid_value: Used to compare against sudden changes.
 * - error_count: Used to count consecutive noise occurrences (debounce).
 */
typedef struct {
    uint16_t last_valid_value;
    uint8_t  error_count;
} ultrasonic_filter_t;

/**
 * @brief Initialize all Ultrasonic GPIOs
 * @details 
 * Configures Trig (Output) and Echo (Input) pins.
 * - Trig: GPIO_MODE_OUTPUT (Push-pull)
 * - Echo: GPIO_MODE_INPUT with PULL-DOWN enabled (Critical for stability!)
 * Sets Trig pin to low (0) immediately to prevent false triggering upon startup.
 * @return ESP_OK on success.
 */
esp_err_t ultrasonic_init(void);

/**
 * @brief Measure raw distance from sensor
 * @param trig_pin GPIO number of Trigger pin
 * @param echo_pin GPIO number of Echo pin
 * @return 
 * - Distance in millimeters (mm)
 * - Returns US_ERROR_CODE (0xFFFF) if a Timeout occurs.
 */
uint16_t ultrasonic_measure(gpio_num_t trig_pin, gpio_num_t echo_pin);

/**
 * @brief Apply Advanced Filter (Blind Zone + Jump Rejection)
 * @details
 * This function addresses 2 physical issues of the JSN-SR04T:
 * 1. Blind Zone Trap: When an object is too close (<20cm), the sensor often reports 
 * random large values -> The function will keep the previous valid value.
 * 2. Jump Filter: Rejects sudden noise spikes by counting consecutive deviations.
 * * @param raw_distance Raw value measured from the measure function
 * @param filter Pointer to the state struct of that specific sensor
 * @return Stable, clean distance value
 */
uint16_t ultrasonic_filter_apply(uint16_t raw_distance, ultrasonic_filter_t *filter);

/**
 * @brief Reset Filter State
 * @details
 * Clears the filter history and error counters. 
 * Call this when initializing the system or after a long standby.
 * @param filter Pointer to the filter struct
 */
void ultrasonic_filter_reset(ultrasonic_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif // DRV_ULTRASONIC_H