/*#include <stdint.h>
#include <stdlib.h>
#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_config.h"
#include "drv_loadcell.h"

static bool is_initialized = false;
static esp_err_t err;

esp_err_t init_loadcell(loadcell_t *sensor){
    gpio_config_t PD_SCK = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << sensor->pin_sck),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    err = gpio_config(&PD_SCK);
    if (err != ESP_OK) return err;

    gpio_config_t DOUT = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << sensor->pin_dout),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    err = gpio_config(&DOUT);
    if (err != ESP_OK) return err;
    
    gpio_set_level(sensor->pin_sck, 0);

    is_initialized = true;
    return ESP_OK;
}

int32_t loadcell_read_raw(loadcell_t *sensor){
    uint8_t timeout = 0;
    if (!is_initialized) return LC_ERROR_CODE;

    while (gpio_get_level(sensor->pin_dout) == 1) {
        timeout++;
        vTaskDelay(pdMS_TO_TICKS(1));
        if(timeout == TIME_OUT){
            raw_value = LC_ERROR_CODE;
            return raw_value;
        }
    }

    raw_value = 0;
    for (int i = 0; i < 24; i++){
        raw_value <<= 1; 

        gpio_set_level(sensor->pin_sck, 1);
        ets_delay_us(2);

        if (gpio_get_level(sensor->pin_dout))
            raw_value |= 1;

        gpio_set_level(sensor->pin_sck, 0);
        ets_delay_us(2);
    }

    gpio_set_level(sensor->pin_sck, 1);
    ets_delay_us(2);
    gpio_set_level(sensor->pin_sck, 0);

    if(raw_value & (1 << 23)){
    raw_value |= HX711_SIGN_MASK;
    }

    return raw_value;

}
void avg_cal_started(loadcell_t *sensor){
    int32_t sum = 0;
    int32_t raw = 0;
    uint8_t count = 0;
    while(count < AVG_STARTED){
        raw = loadcell_read_raw(sensor);
        if(raw > LC_ERROR_CODE){
            count++;
            sum += raw;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    sensor->offset = sum / count;
}

uint16_t get_weight(loadcell_t *sensor){
    int32_t raw = loadcell_read_raw(sensor);
    uint16_t weight = (raw - sensor->offset) / SCALE_FACTOR;
    return weight;
}*/
#include <stdint.h>
#include <stdlib.h>
#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv_loadcell.h"

static const char *TAG = "LOADCELL";
static bool is_initialized = false;

esp_err_t init_loadcell(loadcell_t *sensor) {
    gpio_config_t PD_SCK = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << sensor->pin_sck),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    esp_err_t err = gpio_config(&PD_SCK);
    if (err != ESP_OK) return err;

    gpio_config_t DOUT = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << sensor->pin_dout),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Sửa: dùng pull-up
    };

    err = gpio_config(&DOUT);
    if (err != ESP_OK) return err;
    
    gpio_set_level(sensor->pin_sck, 0);
    
    // Khởi tạo giá trị mặc định
    sensor->offset = 0;
    sensor->scale_factor = 1.0f;

    is_initialized = true;
    ESP_LOGI(TAG, "Loadcell initialized on SCK:%d DOUT:%d", 
             sensor->pin_sck, sensor->pin_dout);
    return ESP_OK;
}

int32_t loadcell_read_raw(loadcell_t *sensor) {
    int32_t raw = 0;
    uint16_t timeout = 0;

    // Chờ dữ liệu sẵn sàng
    while (gpio_get_level(sensor->pin_dout) == 1) {
        vTaskDelay(pdMS_TO_TICKS(1)); 
        timeout++;
        if (timeout > TIME_OUT) {
            ESP_LOGW(TAG, "Timeout reading sensor");
            return LC_ERROR_CODE;
        }
    }

    // Đọc 24 bit
    portDISABLE_INTERRUPTS(); 
    
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

    // Xung thứ 25 (Gain 128, Channel A)
    gpio_set_level(sensor->pin_sck, 1);
    ets_delay_us(1);
    gpio_set_level(sensor->pin_sck, 0);
    ets_delay_us(1);
    
    portENABLE_INTERRUPTS();

    // Xử lý dấu (two's complement)
    if (raw & (1<<23)) {
        raw |= HX711_SIGN_MASK;
    }

    return raw;
}

// Đọc trung bình nhiều lần
int32_t loadcell_read_average(loadcell_t *sensor, uint8_t times) {
    if (times < 1) times = 1;
    
    int32_t sum = 0;
    uint8_t valid_count = 0;
    
    for (uint8_t i = 0; i < times; i++) {
        int32_t raw = loadcell_read_raw(sensor);
        
        if (raw != LC_ERROR_CODE) {
            sum += raw;
            valid_count++;
        }
        
        vTaskDelay(pdMS_TO_TICKS(12)); // Chờ mẫu mới
    }

    if (valid_count == 0) return LC_ERROR_CODE;
    
    return (sum / valid_count);
}

// Tare - Đặt điểm 0
void loadcell_tare(loadcell_t *sensor) {
    ESP_LOGI(TAG, "Taring... Remove all weight");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    sensor->offset = loadcell_read_average(sensor, AVG_STARTED);
    
    if (sensor->offset != LC_ERROR_CODE) {
        ESP_LOGI(TAG, "Tare completed. Offset: %ld", sensor->offset);
    } else {
        ESP_LOGE(TAG, "Tare failed!");
    }
}

// Đọc giá trị đã trừ offset (quan trọng để tính scale)
int32_t loadcell_get_value(loadcell_t *sensor) {
    int32_t raw = loadcell_read_average(sensor, 5);
    
    if (raw == LC_ERROR_CODE) return LC_ERROR_CODE;
    
    return (raw - sensor->offset);
}

// Hiệu chỉnh với trọng lượng đã biết
void loadcell_calibrate(loadcell_t *sensor, float known_weight) {
    ESP_LOGI(TAG, "=== BẮT ĐẦU HIỆU CHỈNH ===");
    
    // BƯỚC 1: TARE (Lấy điểm 0)
    ESP_LOGW(TAG, ">> B1: Bỏ hết vật nặng ra khỏi cân!");
    ESP_LOGI(TAG, "Đang chờ 3 giây...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Lưu offset lúc không tải
    loadcell_tare(sensor); 
    ESP_LOGI(TAG, "Offset đã lưu: %ld", sensor->offset);

    // BƯỚC 2: ĐO MẪU (Lấy giá trị có tải)
    ESP_LOGW(TAG, ">> B2: Đặt vật nặng %.1f gram lên cân!", known_weight);
    ESP_LOGI(TAG, "Đang chờ 5 giây để ổn định...");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Chờ lâu hơn chút để kịp đặt vật
    
    // Đọc giá trị raw MỚI (khi đã có vật)
    int32_t raw_with_load = loadcell_read_average(sensor, 10);
    
    if (raw_with_load == LC_ERROR_CODE) {
        ESP_LOGE(TAG, "Lỗi đọc cảm biến!");
        return;
    }

    // Tính giá trị chênh lệch (Delta)
    int32_t delta = raw_with_load - sensor->offset;
    
    if (delta == 0) {
        ESP_LOGE(TAG, "Lỗi: Giá trị không đổi! Có chắc đã đặt vật lên chưa?");
        return;
    }

    // Tính Scale Factor
    sensor->scale_factor = (float)delta / known_weight;
    
    ESP_LOGI(TAG, "=== KẾT QUẢ ===");
    ESP_LOGI(TAG, "Raw (Có tải): %ld", raw_with_load);
    ESP_LOGI(TAG, "Delta: %ld", delta);
    ESP_LOGI(TAG, "SCALE_FACTOR TIM ĐƯỢC: %.4f", sensor->scale_factor);
    ESP_LOGW(TAG, "Hãy copy số %.4f vào code init!", sensor->scale_factor);
}

// Đặt scale thủ công
void loadcell_set_scale(loadcell_t *sensor, float scale) {
    sensor->scale_factor = scale;
    ESP_LOGI(TAG, "Scale set to: %.2f", scale);
}

// Lấy trọng lượng (gram)
float get_weight(loadcell_t *sensor) {
    int32_t value = loadcell_get_value(sensor);
    
    if (value == LC_ERROR_CODE) return 0.0f;
    
    return (float)value / sensor->scale_factor;
}

// Debug: In ra các giá trị để quan sát
void loadcell_debug_print(loadcell_t *sensor) {
    int32_t raw = loadcell_read_raw(sensor);
    
    if (raw == LC_ERROR_CODE) {
        ESP_LOGE(TAG, "Read error!");
        return;
    }
    
    int32_t value = raw - sensor->offset;
    float weight = (float)value / sensor->scale_factor;
    
    ESP_LOGI(TAG, "RAW: %ld | OFFSET: %ld | VALUE: %ld | SCALE: %.2f | WEIGHT: %.2f g",
             raw, sensor->offset, value, sensor->scale_factor, weight);
}