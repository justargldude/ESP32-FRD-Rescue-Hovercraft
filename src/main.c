#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cal_battery.h" 

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    battery_init();

    ESP_LOGI(TAG, "System Initialized. Starting Main Loop...");

    while (1) {
        
        float vol = battery_get_voltage();      // Lấy số Vôn
        float pct = battery_get_percentage();   // Lấy %

        // 4. HIỂN THỊ / XỬ LÝ
        ESP_LOGI(TAG, "Main Loop: Battery = %.2f V (%.1f %%)", vol, pct);

        // --- Logic điều khiển thực tế cho FRD ---
        if (pct < 20.0) {
            ESP_LOGW(TAG, "Low Battery! Triggering Return-to-Home...");
            // return_to_home();
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}