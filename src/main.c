#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cal_battery.h" 

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    while (1) {
        battery_check_health();
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}