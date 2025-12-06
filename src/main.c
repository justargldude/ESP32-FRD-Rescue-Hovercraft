#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "cal_battery.h" 
#include "sys_monitor.h"

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    
    ESP_LOGI(TAG, ">>> WAKING FRD SYSTEM <<<");
    sys_mon_check_memory();
    battery_init();
    
    while (1) {
        battery_check_health();
        
        // In RAM mỗi 10 lần loop (40 giây)
        static int count = 0;
        if (++count >= 10) {
            count = 0;
            sys_mon_check_memory();
        }
        
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

