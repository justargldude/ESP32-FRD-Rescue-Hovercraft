/**
 * @file drv_motor.c
 * @brief BLDC Motor & ESC Driver Implementation
 */

#include "drv_motor.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_config.h" 

static const char *TAG = "DRV_MOTOR";

// --- HELPER FUNCTIONS ---

/**
 * @brief Convert raw speed value (0-10000) to LEDC Duty Tick
 * @note  Formula: Duty = (Pulse_US / Period_US) * Max_Resolution
 */
static uint32_t raw_to_duty(uint16_t raw_val) {
    // Ngăn chặn trường hợp PID tính ra số quá lớn gây lố hành trình ga
    if (raw_val > MOTOR_SPEED_MAX_RAW){
        raw_val = MOTOR_SPEED_MAX_RAW;
    }

    // Calculate Pulse Width in Microseconds (us)
    // Mapping: 0 -> 1000us | 10000 -> 2000us
    uint16_t added_us = raw_val / 10; 
    uint16_t pulse_us = 1000 + added_us;

    // 3. Convert to Duty Cycle (14-bit resolution)
    // Period @ 50Hz = 20000us. Resolution 14-bit = 16383.
    return ((uint32_t)pulse_us * 16383) / 20000;
}

// --- PUBLIC FUNCTIONS ---

void motor_init(void) {

    // 1. Configure Timer (Shared "Heartbeat")
    ledc_timer_config_t timer_conf = {
        .speed_mode       = PWM_MOTOR_MODE,
        .timer_num        = PWM_MOTOR_TIMER,
        .duty_resolution  = PWM_MOTOR_RESOLUTION,
        .freq_hz          = PWM_MOTOR_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure      = false
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // 2. Configure Left Motor Channel
    ledc_channel_config_t left_conf = {
        .gpio_num       = PIN_MOTOR_LEFT,
        .speed_mode     = PWM_MOTOR_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = PWM_MOTOR_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&left_conf));

    // 3. Configure Right Motor Channel
    ledc_channel_config_t right_conf = {
        .gpio_num       = PIN_MOTOR_RIGHT,
        .speed_mode     = PWM_MOTOR_MODE,
        .channel        = LEDC_CHANNEL_1, 
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = PWM_MOTOR_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&right_conf));

    // 4. ESC Arming Sequence (Safety First)
    // Gửi tín hiệu Min Throttle (0%) ngay lập tức để ESC mở khóa an toàn.
    motor_set_speed(MOTOR_IDLE_RAW, MOTOR_IDLE_RAW);
    
    vTaskDelay(pdMS_TO_TICKS(3000)); // Block 3s for ESC initialization
}

void motor_set_speed(uint16_t left_raw, uint16_t right_raw) {
// Lưu ý: 5000 là đứng im. >5000 là tiến. <5000 là lùi
    
    // Calculate Duty Cycles
    uint32_t duty_left = raw_to_duty(left_raw);
    uint32_t duty_right = raw_to_duty(right_raw);

    // Update Hardware
    ledc_set_duty(PWM_MOTOR_MODE, LEDC_CHANNEL_0, duty_left);
    ledc_update_duty(PWM_MOTOR_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(PWM_MOTOR_MODE, LEDC_CHANNEL_1, duty_right);
    ledc_update_duty(PWM_MOTOR_MODE, LEDC_CHANNEL_1);
}

void motor_stop_all(void) {
    motor_set_speed(MOTOR_IDLE_RAW, MOTOR_IDLE_RAW);
    ESP_LOGW(TAG, "MOTORS EMERGENCY STOP!");
}