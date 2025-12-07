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

// Filter Configuration Constants
#define FILTER_SPIKE_THRESHOLD_MM   500    // Max allowed jump in mm
#define FILTER_SPIKE_TOLERANCE      3      // Consecutive spikes before accepting
#define FILTER_ERROR_LIMIT          10     // Max consecutive errors before reset
#define FILTER_SAFE_DISTANCE_MM     300    // Fallback safe distance

// Measurement Range Constants
#define US_BLIND_ZONE_THRESHOLD_MM  300    // Threshold for blind zone detection
#define US_BLIND_ZONE_JUMP_MM       3000   // Suspicious jump indicating blind zone

// --- CONFIGURATION MACROS ---
#define MODE_OUTPUT     GPIO_MODE_OUTPUT
#define MODE_INPUT      GPIO_MODE_INPUT

#define US_PULSE_TIMEOUT_US         35000  // 35ms timeout for echo pulse (6m max)
#define US_ECHO_WAIT_TIMEOUT_US     20000  // 20ms timeout for echo start

// Error Code (Max uint16_t)
#define US_ERROR_CODE   0xFFFF 

// --- DATA STRUCTURES ---

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

// --- FUNCTION PROTOTYPES ---

/**
 * @brief Initialize all Ultrasonic GPIOs
 * @details 
 * Configures Trig (Output) and Echo (Input) pins.
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

void ultrasonic_filter_reset(ultrasonic_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif // DRV_ULTRASONIC_H