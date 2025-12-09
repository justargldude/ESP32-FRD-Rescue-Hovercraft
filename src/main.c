#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "drv_ultrasonic.h"
#include "app_config.h"

static const char *TAG = "MAIN_APP";

static ultrasonic_filter_t filter_front = {0};

void app_main(void) {
    
    // Initialize sensor
    esp_err_t err = ultrasonic_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "FAILED!");
        return;
    }
    
    // Continuous monitoring loop
    ultrasonic_filter_reset(&filter_front);
    
    while (1) {
        uint16_t raw = ultrasonic_measure(
            (gpio_num_t)FRONT_ULTRASONIC_TRIG, 
            (gpio_num_t)FRONT_ULTRASONIC_ECHO
        );
        
        uint16_t distance = ultrasonic_filter_apply(raw, &filter_front);
        
        if (raw == US_ERROR_CODE) {
            ESP_LOGW(TAG, "Distance: ERROR (using filtered: %d mm)", distance);
        } else {
            // Only log if distance changed significantly (>10mm)
            static uint16_t last_logged = 0;
            if (abs((int16_t)(distance - last_logged)) > 10) {
                ESP_LOGI(TAG, "Distance: %d mm (%.2f cm) [raw: %d mm]", 
                         distance, distance / 10.0f, raw);
                last_logged = distance;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}