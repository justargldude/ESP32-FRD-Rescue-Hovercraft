// drv_loadcell.h
#ifndef LOADCELL_H
#define LOADCELL_H
 
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int pin_sck;      
    int pin_dout;     
    int32_t offset;
    float scale_factor;  // Lưu scale factor riêng cho mỗi cảm biến
} loadcell_t;

#define HX711_SIGN_MASK  0xFF000000 
#define LC_ERROR_CODE    -2147483648 

#define TIME_OUT 100
#define AVG_STARTED 50

// Khởi tạo
esp_err_t init_loadcell(loadcell_t *sensor);

// Đọc giá trị raw (chưa xử lý)
int32_t loadcell_read_raw(loadcell_t *sensor);

// Đọc giá trị đã trừ offset (dùng để tính scale)
int32_t loadcell_get_value(loadcell_t *sensor);

// Tare - Đặt điểm 0
void loadcell_tare(loadcell_t *sensor);

// Đọc trung bình nhiều lần
int32_t loadcell_read_average(loadcell_t *sensor, uint8_t times);

// Hiệu chỉnh với trọng lượng đã biết
void loadcell_calibrate(loadcell_t *sensor, float known_weight);

// Đặt scale factor thủ công
void loadcell_set_scale(loadcell_t *sensor, float scale);

// Lấy trọng lượng (gram)
float get_weight(loadcell_t *sensor);

// Debug: In ra các giá trị
void loadcell_debug_print(loadcell_t *sensor);

#ifdef __cplusplus
}
#endif
#endif
