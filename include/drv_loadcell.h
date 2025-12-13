/**
 * @file drv_loadcell.h
 * @brief HX711 Loadcell Driver with Ring Buffer Filtering & Rescue Logic
 * @version 2.1
 * @date 2025-12-14
 * * Features:
 * - Ring Buffer Moving Average (Low-pass filter)
 * - Hysteresis logic for Human Detection (Debounce)
 * - Impact detection algorithm for Collision Sensing
 */

#ifndef DRV_LOADCELL_H
#define DRV_LOADCELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/*----------------------------------------
        CONFIGURATION CONSTANT
  ----------------------------------------*/


// Hardware & Protocol
#define HX711_SIGN_MASK         0xFF000000      // Mask for 24-bit two's complement extension
#define LC_ERROR_CODE           -2147483648     // INT32_MIN as error indicator
#define LC_READ_TIMEOUT         100             // Max loop iterations to wait for DOUT low
#define LC_TARE_SAMPLES         50              // Number of samples for Tare operation

//  Signal Processing
#define FILTER_BUFFER_SIZE      10              // Size of Ring Buffer (Moving Average)

// Logic Thresholds 
// Human detection uses Hysteresis 
#define THRESH_HUMAN_TRIGGER    8000            // Upper bound: Trigger "Human Detected"
#define THRESH_HUMAN_RELEASE    6000            // Lower bound: Release "Human Detected"
#define THRESH_COLLISION_DELTA  3000            // Delta change required to trigger collision

// Logic Timing
#define COUNTER_DETECT_REQ      5               // Consecutive samples required to confirm human presence
#define COLLISION_COOLDOWN_MS   500             // Cooldown after collision detection (prevent retriggering)

/*----------------------------------------
            DATA STRUCTURES
  ----------------------------------------*/

/**
 * @brief Loadcell object structure
 * * Contains hardware config, calibration data, and runtime buffers.
 * User only needs to initialize `pin_sck`, `pin_dout`, and `scale_factor`.
 */
typedef struct {
    // User Configuration
    int pin_sck;                // GPIO number for Serial Clock (Output)
    int pin_dout;               // GPIO number for Data Out (Input)
    float scale_factor;         // Calibration factor (Raw / Scale = Weight)

    // Calibration Data 
    int32_t offset;             // Zero point value (Tare value)
    bool is_initialized;        // Flag indicating if GPIO/Driver is ready

    // Ring Buffer (Moving Average Filter) 
    int32_t filter_buffer[FILTER_BUFFER_SIZE];  // Circular buffer for raw data
    uint8_t buffer_head;                        // Current write index
    bool is_buffer_full;                        // Flag: true if buffer has wrapped around

    // Human Detection Logic 
    uint8_t stable_counter;     // Counter for debouncing human presence
    bool is_human_detected;     // Output flag: True if human is confirmed

    // Collision Detection Logic 
    // Note: Uses RAW values (not smoothed) to preserve spike signals
    int32_t last_raw_weight;        // RAW weight from previous cycle (not smoothed!)
    bool is_collision_detected;     // True if impact detected
    int64_t last_collision_time_us; // Timestamp of last collision (for cooldown)
} loadcell_t;

/**
 * @brief Initialize Loadcell Driver
 * * Configures GPIO, resets buffers, and clears internal states.
 * @param sensor Pointer to loadcell_t struct
 * @return ESP_OK on success, ESP_FAIL on GPIO error
 */
esp_err_t loadcell_init(loadcell_t *sensor);

/**
 * @brief Read Raw Data from HX711 (Bit-banging)
 * * @note This function disables interrupts briefly to ensure timing accuracy.
 * @param sensor Pointer to loadcell_t struct
 * @return 24-bit signed integer (Raw value) or LC_ERROR_CODE on timeout
 */
int32_t loadcell_read_raw(loadcell_t const *sensor);

/**
 * @brief Measure average raw value (Blocking)
 * * Useful for Taring (Zeroing) or Calibration.
 * @param sensor Pointer to loadcell_t struct
 * @param times Number of samples to average
 * @return ESP_OK if successful, ESP_FAIL if sensor timeout
 */
esp_err_t loadcell_read_average_raw(loadcell_t *sensor, uint8_t times);

/**
 * @brief Calculate real weight based on offset and scale
 * * Formula: (Raw - Offset) / Scale
 * @param sensor Pointer to loadcell_t struct
 * @return Weight in grams (int32_t)
 */
int32_t loadcell_get_weight(loadcell_t *sensor);

/**
 * @brief Apply Ring Buffer Filter to get smooth weight
 * * Adds new weight to buffer and returns the moving average.
 * @param sensor Pointer to loadcell_t struct
 * @param new_weight Latest measured weight
 * @return Smoothed weight value
 */
int32_t loadcell_get_smooth_weight(loadcell_t *sensor, int32_t new_weight);

/**
 * @brief Logic: Detect Human on Side Sensors
 * * Uses Hysteresis and Persistence Counter to filter wave noise.
 * Updates `is_human_detected` flag in the struct.
 * * @param left_sensor Pointer to Left Loadcell
 * @param right_sensor Pointer to Right Loadcell
 */
void logic_detect_human(loadcell_t *left_sensor, loadcell_t *right_sensor);

/**
 * @brief Logic: Detect Collision on Front Sensor
 * * Uses Derivative (Delta) check to detect sudden impacts.
 * Updates `is_collision_detected` flag in the struct.
 * * @param front_sensor Pointer to Front Loadcell
 */
void logic_detect_collision(loadcell_t *front_sensor);

#ifdef __cplusplus
}
#endif

#endif // DRV_LOADCELL_H