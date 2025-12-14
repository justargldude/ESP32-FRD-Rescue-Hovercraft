/**
 * @file main.c
 * @brief ESP32-FRD Rescue Hovercraft - Main Application
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "app_config.h"
#include "drv_loadcell.h"

static const char *TAG = "MAIN_APP";

/*----------------------------------------
        GLOBAL SENSOR INSTANCES
  ----------------------------------------*/

static loadcell_t sensor_front = {
    .pin_sck = PD_SCK_FRONT,
    .pin_dout = DOUT_FRONT,
    .scale_factor = SCALE_FRONT
};

static loadcell_t sensor_left = {
    .pin_sck = PD_SCK_LEFT,
    .pin_dout = DOUT_LEFT,
    .scale_factor = SCALE_LEFT
};

static loadcell_t sensor_right = {
    .pin_sck = PD_SCK_RIGHT,
    .pin_dout = DOUT_RIGHT,
    .scale_factor = SCALE_RIGHT
};

/*----------------------------------------
        TASK: CALIBRATION MODE
  ----------------------------------------*/

// First-time setup: Set CALIBRATION_MODE=1 in app_config.h
// After calibration: Copy SCALE values to app_config.h, set CALIBRATION_MODE=0, reflash
#if CALIBRATION_MODE

void task_calibration(void *arg) {
    ESP_LOGW(TAG, "=== CALIBRATION MODE ===");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Run calibration wizard
    loadcell_calibrate_all(&sensor_front, &sensor_left, &sensor_right, CALIBRATION_WEIGHT_G);
    
    ESP_LOGW(TAG, "Copy values to app_config.h, set CALIBRATION_MODE=0, reflash");
    
    // Debug loop to verify
    while (1) {
        loadcell_debug_print(&sensor_front, "F");
        loadcell_debug_print(&sensor_left, "L");
        loadcell_debug_print(&sensor_right, "R");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#endif

/*----------------------------------------
        TASK: NORMAL OPERATION
  ----------------------------------------*/

#if !CALIBRATION_MODE

void task_sensor_monitor(void *arg) {
    loadcell_tare(&sensor_front);
    loadcell_tare(&sensor_left);
    loadcell_tare(&sensor_right);
    
    while (1) {
        // Run detection logic
        logic_detect_human(&sensor_left, &sensor_right);
        logic_detect_collision(&sensor_front);
        
        // Handle collision event
        if (sensor_front.is_collision_detected) {
            ESP_LOGE(TAG, "COLLISION!");
            // TODO: motor_emergency_stop();
        }
        
        // Handle human detection
        if (sensor_left.is_human_detected) {
            ESP_LOGW(TAG, "Human LEFT");
            // TODO: Slow down + buzzer
        }
        
        if (sensor_right.is_human_detected) {
            ESP_LOGW(TAG, "Human RIGHT");
            // TODO: Slow down + buzzer
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 20Hz sampling
    }
}

#endif

/*----------------------------------------
        MAIN ENTRY POINT
  ----------------------------------------*/

void app_main(void) {
    ESP_LOGI(TAG, "ESP32-FRD Rescue Hovercraft [%s]", __DATE__);
    
    // Initialize loadcells
    loadcell_init(&sensor_front);
    loadcell_init(&sensor_left);
    loadcell_init(&sensor_right);
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
#if CALIBRATION_MODE
    xTaskCreate(task_calibration, "cal", 4096, NULL, 5, NULL);
#else
    xTaskCreate(task_sensor_monitor, "mon", 4096, NULL, 5, NULL);
#endif
}