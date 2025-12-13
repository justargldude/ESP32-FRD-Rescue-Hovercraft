/**
 * @file drv_loadcell.c
 * @brief Implementation of HX711 Driver & Rescue Logic
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
#include "drv_loadcell.h"

// DRIVER IMPLEMENTATION

esp_err_t loadcell_init(loadcell_t *sensor) {
    esp_err_t err;

    //  Configure SCK (Output)
    gpio_config_t conf_sck = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << sensor->pin_sck),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    err = gpio_config(&conf_sck);
    if (err != ESP_OK) return err;

    // Configure DOUT (Input)
    gpio_config_t conf_dout = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << sensor->pin_dout),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE, 
    };
    err = gpio_config(&conf_dout);
    if (err != ESP_OK) return err;
    
    // Set Idle State
    gpio_set_level(sensor->pin_sck, 0);

    // Clean Memory (CRITICAL for Logic)
    for(int i = 0; i < FILTER_BUFFER_SIZE; i++) {
        sensor->filter_buffer[i] = 0; 
    }
    sensor->buffer_head = 0;
    sensor->is_buffer_full = false;
    
    // Reset Logic States
    sensor->stable_counter = 0;
    sensor->is_human_detected = false;
    
    sensor->last_raw_weight = 0;
    sensor->is_collision_detected = false;
    sensor->last_collision_time_us = 0;

    sensor->is_initialized = true;
    return ESP_OK;
}

int32_t loadcell_read_raw(loadcell_t const *sensor) {
    if (!sensor->is_initialized) return LC_ERROR_CODE;

    int32_t raw = 0;
    uint16_t timeout = 0;

    // Wait for Data Ready (DOUT goes LOW)
    while (gpio_get_level(sensor->pin_dout) == 1) {
        vTaskDelay(pdMS_TO_TICKS(1)); 
        timeout++;
        if (timeout > LC_READ_TIMEOUT) return LC_ERROR_CODE;
    }

    // Critical Section: Bit-banging
    portDISABLE_INTERRUPTS(); 
    
    // Read 24 bits
    for (int i = 0; i < 24; i++) {
        gpio_set_level(sensor->pin_sck, 1);
        ets_delay_us(1); 
        
        raw = raw << 1;
        
        if (gpio_get_level(sensor->pin_dout)) {
            raw |= 1;
        }
        
        gpio_set_level(sensor->pin_sck, 0);
        ets_delay_us(1);
    }

    // Set Gain 128 (Channel A)
    gpio_set_level(sensor->pin_sck, 1);
    ets_delay_us(1);
    gpio_set_level(sensor->pin_sck, 0);
    ets_delay_us(1);
    
    portENABLE_INTERRUPTS();

    // Handle 24-bit Sign Extension
    if (raw & (1 << 23)) {
        raw |= HX711_SIGN_MASK;
    }
    return raw;
}

esp_err_t loadcell_read_average_raw(loadcell_t *sensor, uint8_t times) {
    if (times < 1) times = 1;
    
    int32_t sum = 0; 
    uint8_t valid_count = 0;
    
    for (uint8_t i = 0; i < times; i++) {
        int32_t raw = loadcell_read_raw(sensor);
        
        if (raw != LC_ERROR_CODE) {
            sum += raw;
            valid_count++;
        }
        // Small delay between samples for stability
        vTaskDelay(pdMS_TO_TICKS(12)); 
    }

    if (valid_count == 0) return ESP_FAIL;
    
    sensor->offset = (sum / valid_count);
    return ESP_OK;
}

int32_t loadcell_get_weight(loadcell_t *sensor) {
    int32_t raw = loadcell_read_raw(sensor);
    
    if (raw == LC_ERROR_CODE) return LC_ERROR_CODE;

    // Weight = (Raw - Tare) / Scale
    // Cast to float for proper division, then back to int32_t
    return (int32_t)((float)(raw - sensor->offset) / sensor->scale_factor);
}


// LOGIC IMPLEMENTATION


int32_t loadcell_get_smooth_weight(loadcell_t *sensor, int32_t new_weight) {
    // Returns last known average instead of corrupting the filter
    if (new_weight == LC_ERROR_CODE) {
        // Return last known average if we have data
        uint8_t count = sensor->is_buffer_full ? FILTER_BUFFER_SIZE : sensor->buffer_head;
        if (count == 0) return 0;
        int32_t sum = 0;
        for (int i = 0; i < count; i++) {
            sum += sensor->filter_buffer[i];
        }
        return (int32_t)(sum / count);
    }

    // Overwrite oldest value in Ring Buffer
    sensor->filter_buffer[sensor->buffer_head] = new_weight;
    sensor->buffer_head++;

    // Wrap around logic
    if (sensor->buffer_head >= FILTER_BUFFER_SIZE) {
        sensor->is_buffer_full = true;
        sensor->buffer_head = 0;
    } 

    // Determine current sample size
    // If buffer is full, divide by SIZE. If starting up, divide by current index.
    uint8_t count = sensor->is_buffer_full ? FILTER_BUFFER_SIZE : sensor->buffer_head;

    if (count == 0) return new_weight;

    // Calculate Average
    int32_t sum = 0;
    for (int i = 0; i < count; i++) {
        sum += sensor->filter_buffer[i];
    }
    
    return (int32_t)(sum / count);
}

void logic_detect_human(loadcell_t *left_sensor, loadcell_t *right_sensor) {
    // Acquire Data (smooth_weight handles LC_ERROR_CODE internally)
    int32_t raw_left = loadcell_get_weight(left_sensor);
    int32_t smooth_left = loadcell_get_smooth_weight(left_sensor, raw_left);

    int32_t raw_right = loadcell_get_weight(right_sensor);
    int32_t smooth_right = loadcell_get_smooth_weight(right_sensor, raw_right);
    
    // Skip processing if sensor errors (smooth returns last known or 0)
    bool left_valid = (raw_left != LC_ERROR_CODE);
    bool right_valid = (raw_right != LC_ERROR_CODE);

    // Process LEFT Sensor
    if (left_valid) {
        if (smooth_left >= THRESH_HUMAN_TRIGGER) {
            // Accumulate confidence
            left_sensor->stable_counter++;
            
            // Saturation logic: Cap the counter to prevent overflow
            if (left_sensor->stable_counter > COUNTER_DETECT_REQ) left_sensor->stable_counter = COUNTER_DETECT_REQ;

            // Trigger condition
            if (left_sensor->stable_counter >= COUNTER_DETECT_REQ) {
                left_sensor->is_human_detected = true;
            }
        } 
        else if (smooth_left <= THRESH_HUMAN_RELEASE) { 
            if (left_sensor->stable_counter > 0) left_sensor->stable_counter--;
            
            // Release condition
            if (left_sensor->stable_counter == 0) {
                left_sensor->is_human_detected = false;
            }
        }
    }

    // Process RIGHT Sensor
    if (right_valid) {
        if (smooth_right >= THRESH_HUMAN_TRIGGER) {
            right_sensor->stable_counter++;
            if (right_sensor->stable_counter > COUNTER_DETECT_REQ) right_sensor->stable_counter = COUNTER_DETECT_REQ;

            if (right_sensor->stable_counter >= COUNTER_DETECT_REQ) {
                right_sensor->is_human_detected = true;
            }
        } 
        else if (smooth_right <= THRESH_HUMAN_RELEASE) {
            if (right_sensor->stable_counter > 0) right_sensor->stable_counter--;
            
            if (right_sensor->stable_counter == 0) {
                right_sensor->is_human_detected = false;
            }
        }
    }
}

void logic_detect_collision(loadcell_t *front_sensor) {
    // Collision = sudden impulse force, must detect within 1 sample cycle.
    int32_t raw = loadcell_get_weight(front_sensor);
    
    // Skip if sensor error
    if (raw == LC_ERROR_CODE) return;

    // Initialize on first valid read
    if (front_sensor->last_raw_weight == 0) {
        front_sensor->last_raw_weight = raw;
        return;
    }

    // Calculate Delta 
    int32_t delta = abs(raw - front_sensor->last_raw_weight);

    // Get current time for cooldown check
    int64_t now_us = esp_timer_get_time();
    int64_t cooldown_us = COLLISION_COOLDOWN_MS * 1000LL;

    // Collision detection with cooldown
    // Cooldown prevents multiple triggers from post-impact oscillations
    if (delta > THRESH_COLLISION_DELTA) {
        // Only trigger if cooldown has passed
        if ((now_us - front_sensor->last_collision_time_us) > cooldown_us) {
            front_sensor->is_collision_detected = true;
            front_sensor->last_collision_time_us = now_us;
        }
        // If within cooldown, keep the flag but don't update timestamp
    } else {
        // Only clear flag after cooldown expires 
        if ((now_us - front_sensor->last_collision_time_us) > cooldown_us) {
            front_sensor->is_collision_detected = false;
        }
    }

    // Store RAW value for next cycle
    front_sensor->last_raw_weight = raw;
}

