#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cal_battery.h" 

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    // GIAI ĐOẠN 1: SETUP
    ESP_LOGI(TAG, "--- WAKING FRD SYSTEM ---");
    
    // Khởi tạo Battery Monitor trước khi vào vòng lặp
    battery_init();
    
    // motor_init();
    // gps_init();

    // GIAI ĐOẠN 2: LOOP
    while (1) {
        // Thực hiện các tác vụ giám sát/điều khiển ở đây
        battery_check_health();

        // Delay để nhường CPU cho các task khác (quan trọng trong FreeRTOS)
        vTaskDelay(pdMS_TO_TICKS(4000)); 
    }
}