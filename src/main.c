#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "drv_ultrasonic.h"
#include "app_config.h"
#include "drv_loadcell.h"

static const char *TAG = "MAIN_APP";

static loadcell_t filter_front = {0};

void app_main(void) {
    // 1. Khởi tạo 3 cảm biến
    loadcell_t sensor_front = {
        .pin_sck = PD_SCK_FRONT,
        .pin_dout = DOUT_FRONT,
    };
    init_loadcell(&sensor_front);

    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // ===== CHẾ ĐỘ HIỆU CHỈNH =====
    // Chỉ chạy 1 LẦN để tìm SCALE_FACTOR
    
    // Hiệu chỉnh cảm biến trước =
    loadcell_calibrate(&sensor_front, 199.0f);
    
    // Hiệu chỉnh cảm biến sau trái
    // loadcell_calibrate(&sensor_rear_left, 1000.0f);
    
    // Hiệu chỉnh cảm biến sau phải
    // loadcell_calibrate(&sensor_rear_right, 1000.0f);
    
    // SAU KHI CÓ SCALE_FACTOR, LƯU VÀO CODE:
    // loadcell_set_scale(&sensor_front, 420.5f);
    // loadcell_set_scale(&sensor_rear_left, 415.2f);
    // loadcell_set_scale(&sensor_rear_right, 418.7f);
    
    // ===== CHẾ ĐỘ HOẠT ĐỘNG BÌNH THƯỜNG =====
    while (1) {
        float w1 = get_weight(&sensor_front);
        
        
        ESP_LOGI(TAG, "Front: %.1f g", w1);
        
        // Hoặc dùng debug để xem chi tiết
        // loadcell_debug_print(&sensor_front);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}