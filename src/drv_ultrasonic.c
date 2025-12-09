/**
 * @file drv_ultrasonic.c
 * @brief Optimized Implementation of Ultrasonic Driver for JSN-SR04T
 */
#include <stdint.h>
#include <stdlib.h>
#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_config.h"
#include "drv_ultrasonic.h"

static const char *TAG = "DRV_US";
static bool is_initialized = false;
esp_err_t err;

esp_err_t ultrasonic_init(void) {
    is_initialized = false;

    // Configure TRIGGER Pins
    gpio_config_t conf_trig = {
        .pin_bit_mask = (1ULL << FRONT_ULTRASONIC_TRIG) | (1ULL << LEFT_ULTRASONIC_TRIG) | (1ULL << RIGHT_ULTRASONIC_TRIG),
        .mode = GPIO_MODE_OUTPUT, 
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&conf_trig);
    if (err != ESP_OK){
        return err;
    }

    // Configure ECHO Pins as Input with Pull-down
    gpio_config_t conf_echo = {
        .pin_bit_mask = (1ULL << FRONT_ULTRASONIC_ECHO) | (1ULL << LEFT_ULTRASONIC_ECHO) | (1ULL << RIGHT_ULTRASONIC_ECHO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE, 
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&conf_echo);
    if (err != ESP_OK){
        return err;
    }

    // Set all TRIG pins to LOW 
    gpio_set_level(FRONT_ULTRASONIC_TRIG, 0);
    gpio_set_level(LEFT_ULTRASONIC_TRIG, 0);
    gpio_set_level(RIGHT_ULTRASONIC_TRIG, 0);

    is_initialized = true;

    return ESP_OK;
}

uint16_t ultrasonic_measure(gpio_num_t trig_pin, gpio_num_t echo_pin) {
    if (!is_initialized) return US_ERROR_CODE;

    // Generate trigger pulse 
    gpio_set_level(trig_pin, 0);
    ets_delay_us(2);
    gpio_set_level(trig_pin, 1);
    ets_delay_us(20); 
    gpio_set_level(trig_pin, 0);

    // Wait for echo pin to go HIGH
    int64_t wait_start = esp_timer_get_time();
    while (gpio_get_level(echo_pin) == 0) {
        if ((esp_timer_get_time() - wait_start) > US_ECHO_WAIT_TIMEOUT_US) {
            return US_ERROR_CODE; 
        }
    }

    // Measure echo pulse width 
    int64_t time_start = esp_timer_get_time();
    while (gpio_get_level(echo_pin) == 1) {
        if ((esp_timer_get_time() - time_start) > US_PULSE_TIMEOUT_US) {
            return US_ERROR_CODE; 
        }
    }
    int64_t time_end = esp_timer_get_time();

    // Calculate distance
    uint32_t time = (uint32_t)(time_end - time_start);
    uint32_t raw_distance = (time * 10) / 58;

    return (uint16_t)raw_distance;
}

uint16_t ultrasonic_filter_apply(uint16_t raw_distance, ultrasonic_filter_t *filter) {
    // Handle sensor error, timeout
    if (raw_distance == US_ERROR_CODE) {
        filter->error_count++;
        
        // Too many consecutive errors, reset to safe distance
        if (filter->error_count > FILTER_ERROR_LIMIT) {
            ESP_LOGW(TAG, "Too many sensor errors, resetting to safe distance");
            filter->last_valid_value = FILTER_SAFE_DISTANCE_MM;
            filter->error_count = 0;
            return FILTER_SAFE_DISTANCE_MM;
        }
        
        // Keep previous valid value
        return filter->last_valid_value;
    }

    // Initialize filter
    if (filter->last_valid_value == 0) {
        filter->last_valid_value = raw_distance;
        filter->error_count = 0;
        return raw_distance;
    }

    // Detect JSN-SR04T blind zone anomaly
    // When object is very close (<25cm), sensor may report large distance (>2.5m)
    if (filter->last_valid_value < US_BLIND_ZONE_THRESHOLD_MM && raw_distance > US_BLIND_ZONE_JUMP_MM) {
        return filter->last_valid_value;
    }

    // Calculate absolute difference
    int16_t diff = (int16_t)raw_distance - (int16_t)filter->last_valid_value;

    // Check for "Jump Closer"
    // New object detected or distance decreased significantly
    // Update immediately for safety
    if (diff < -FILTER_SPIKE_THRESHOLD_MM){
        filter->last_valid_value = raw_distance;
        filter->error_count = 0;
        return filter->last_valid_value;
    }

    // Check for "Jump Further" 
    // Distance increased significantly
    // Verify multiple times
    if (diff > FILTER_SPIKE_THRESHOLD_MM) {
        filter->error_count++;
        
        // Only accept the new "far" value if it persists
        if (filter->error_count >= FILTER_SPIKE_TOLERANCE){
            filter->last_valid_value = raw_distance;
            filter->error_count = 0;
            return filter->last_valid_value;
        }
        
        // Not confirmed yet -> Keep safe "close" value
        return filter->last_valid_value;
    }

    // Normal measurement - accept and reset error counter
    filter->last_valid_value = raw_distance;
    filter->error_count = 0;
    return raw_distance;
}

void ultrasonic_filter_reset(ultrasonic_filter_t *filter) {
    if (filter != NULL) {
        filter->last_valid_value = 0;
        filter->error_count = 0;
    }
}