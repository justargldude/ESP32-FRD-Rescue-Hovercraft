#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "drv_ultrasonic.h"
#include "app_config.h"

static const char *TAG = "MAIN_APP";

static ultrasonic_filter_t filter_front = {0};

void app_main(void) {

    ultrasonic_filter_reset(&filter_front);

    while (1)
    {
        uint16_t raw = ultrasonic_measure((gpio_num_t)FRONT_ULTRASONIC_TRIG, (gpio_num_t)FRONT_ULTRASONIC_ECHO);

        uint16_t distance = ultrasonic_filter_apply(raw, &filter_front);

        if (distance == US_ERROR_CODE) {
             ESP_LOGW(TAG, "Sensor Error or Out of Range");
        } else {
             ESP_LOGI(TAG, "Khoang Cach: %d mm", distance);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}