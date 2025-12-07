/**
 * @file drv_motor.h
 * @brief BLDC Motor & ESC Driver Interface
 * @details 
 * - Handles PWM generation via ESP32 LEDC peripheral.
 * - Manages ESC Arming sequence (Safety startup).
 * - Supports Differential Steering control.
 */

#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#define PWM_MOTOR_TIMER LEDC_TIMER_0
#define PWM_MOTOR_RESOLUTION LEDC_TIMER_14_BIT
#define PWM_MOTOR_FREQ 50
#define PWM_MOTOR_MODE LEDC_LOW_SPEED_MODE

#define MOTOR_IDLE_RAW    5000  // Tương đương 1500us (Điểm giữa)

/**
 * @brief Initialize Motor Driver (Timer, Channels & Arming)
 * @note  **BLOCKING FUNCTION**: This will delay execution for ~3 seconds 
 * to perform the ESC Arming sequence (sending Min signal).
 * (Hàm này sẽ dừng hệ thống khoảng 3s để gửi tín hiệu mở khóa ESC.
 * Đừng gọi trong vòng lặp thời gian thực!).
 */
void motor_init(void);

/**
 * @brief Set speed for Differential Drive (Dual Motor)
 * * @param left_raw  Speed for Left Motor (Range: 0 - 10000)
 * @param right_raw Speed for Right Motor (Range: 0 - 10000)
 * * @note  **Scale Factor**: 10000 raw value = 100.00% Throttle.
 * (Quy ước: 100 tương ứng 1%. Ví dụ muốn chạy 50.5% thì truyền 5050).
 */
void motor_set_speed(uint16_t left_raw, uint16_t right_raw);

/**
 * @brief Emergency Stop (Safety Cutoff)
 * @details Immediately sets PWM duty to 0 for both motors.
 * (Ngắt động cơ ngay lập tức - Dùng khi mất sóng, pin yếu hoặc lỗi hệ thống).
 */
void motor_stop_all(void);

#ifdef __cplusplus
}
#endif

#endif // DRV_MOTOR_H