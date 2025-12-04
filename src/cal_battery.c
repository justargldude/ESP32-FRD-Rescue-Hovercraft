/**
 * @file cal_battery.c
 * @brief Battery Monitoring System (Optimized for 2S LiPo/Li-Ion)
 * @details
 * Features:
 * - ADC Reading with Oversampling (Noise Reduction)
 * - Hardware Calibration (Curve/Line Fitting)
 * - Software Filter (EMA - Exponential Moving Average)
 * - Safety Health Check & Alerts
 */

#include "cal_battery.h"
#include <stdint.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

// PRIVATE CONFIGURATION
static const char *TAG = "CAL_BATTERY";

// Max consecutive read errors before triggering system fault
#define MAX_ERROR_COUNT 20

// Hardware Configuration
#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_3      // GPIO 4
// NOTE: Using 2.5dB attenuation (Range: 0-1250mV).

#define ADC_ATTEN       ADC_ATTEN_DB_2_5   
#define ADC_SAMPLES     50                 // Oversampling count 

// Voltage Divider Config (Measured values)
#define R1_VAL          22300.0f           // 22kΩ
#define R2_VAL          3340.0f            // 3.3kΩ
#define VOLT_DIV_RATIO  7.6766f            // (R1+R2)/R2

// EMA Filter Coefficient
// Tác dụng: Loại bỏ nhiễu sụt áp khi động cơ tăng tốc đột ngột.
#define EMA_ALPHA       0.05f

// PRIVATE STATIC VARIABLES
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static float voltage_filter_val = 0.0f;    // Filtered result state
static bool is_initialized = false;
static bool is_calibrated = false;

// PRIVATE HELPER FUNCTIONS

/**
 * @brief Initialize ADC Calibration Scheme
 * @note Supports both Curve Fitting (Priority) and Line Fitting based on eFuse.
 */
static bool init_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool cali_success = false;

    // Priority 1: Curve Fitting
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!cali_success) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            cali_success = true;
            ESP_LOGI(TAG, "Calibration Scheme: Curve Fitting");
        }
    }
#endif

    // Priority 2: Line Fitting 
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!cali_success) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            cali_success = true;
            ESP_LOGI(TAG, "Calibration Scheme: Line Fitting");
        }
    }
#endif

    cali_handle = handle;
    return cali_success;
}

/**
 * @brief Read ADC with explicit logging for debugging
 */
static esp_err_t read_adc_averaged(int *out_raw) {
    if (!is_initialized || out_raw == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int adc_raw_sum = 0;
    int valid_samples = 0;

    for (int i = 0; i < ADC_SAMPLES; i++) {
        int raw = 0;
        esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);
        
        if (ret == ESP_OK) {
            adc_raw_sum += raw;
            valid_samples++;
        }

        // Small delay to allow sampling capacitor to discharge
        esp_rom_delay_us(100);
    }

    if (valid_samples == 0) {
        return ESP_FAIL;
    }

    *out_raw = (int)(adc_raw_sum / valid_samples);

    return ESP_OK;
}

/**
 * @brief Convert Raw ADC to Voltage (mV)
 */
static esp_err_t raw_to_gpio_voltage(int raw_adc, int *out_mv) {
    if (out_mv == NULL) return ESP_ERR_INVALID_ARG;

    if (is_calibrated && cali_handle != NULL) {
        return adc_cali_raw_to_voltage(cali_handle, raw_adc, out_mv);
    } else {
        // Fallback: Manual calculation for DB_2_5
        *out_mv = (raw_adc * 1250) / 4095;
        return ESP_OK;
    }
}

// PUBLIC API IMPLEMENTATION 

void battery_init(void) {
    if (is_initialized) return;

    // 1. Init Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 2. Config Channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // 3. Init Calibration
    is_calibrated = init_calibration(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN);
    
    if (is_calibrated) {
        ESP_LOGI(TAG, "Initialization DONE (Calibrated)");
    } else {
        ESP_LOGW(TAG, "Initialization DONE (Uncalibrated - Accuracy Reduced)");
    }

    is_initialized = true;
}

float battery_get_voltage(void) {
    if (!is_initialized) return 0.0f;

    static int error_count = 0;
    int adc_raw_avg = 0;
    int voltage_gpio_mv = 0;

    // Read raw
    if(read_adc_averaged(&adc_raw_avg) != ESP_OK) {
        error_count++;
        if(error_count >= MAX_ERROR_COUNT) ESP_LOGE(TAG, "Sensor Failure: Read Error");
        return voltage_filter_val; // Keep previous value (Fail-safe)
    }

    // Convert to Voltage 
    if(raw_to_gpio_voltage(adc_raw_avg, &voltage_gpio_mv) != ESP_OK) {
        error_count++;
        if(error_count >= MAX_ERROR_COUNT) ESP_LOGE(TAG, "Sensor Failure: Convert Error");
        return voltage_filter_val;
    }

    error_count = 0; // Reset error on success

    // Calculate Real Voltage
    float instant_voltage = (float)voltage_gpio_mv * VOLT_DIV_RATIO / 1000.0f;

    // EMA Filter
    if (voltage_filter_val == 0.0f ){
        // Cold Start: Lấy giá trị tức thời lần đầu để tránh độ trễ
        voltage_filter_val = instant_voltage; 
    } else {
        // Y[n] = alpha*X[n] + (1-alpha)*Y[n-1]
        voltage_filter_val = (EMA_ALPHA * instant_voltage) + ((1.0f - EMA_ALPHA) * voltage_filter_val);
    }
    
    return voltage_filter_val;
}

float battery_get_percentage(void) {
    float voltage = battery_get_voltage();

    if (voltage >= BATTERY_MAX_V) return 100.0f;
    if (voltage <= BATTERY_MIN_V) return 0.0f;
    
    return (voltage - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f;
}

void battery_check_health(void) {
    // Observability Logs
    ESP_LOGI(TAG, "--- Health Check ---"); 
    
    float voltage = battery_get_voltage();
    float percentage = battery_get_percentage();
    
    ESP_LOGI(TAG, "Status: %.2fV (%.1f%%)", voltage, percentage);

    if (voltage < BATTERY_CRIT_V) {
        // Critical: Pin < 6.0V. Nguy cơ hỏng pin
        ESP_LOGE(TAG, ">> CRITICAL: %.2fV - FORCE RETURN! <<", voltage);
        // TODO: Trigger Event -> Force Return Home
        
    } else if (voltage < BATTERY_MIN_V) {
        // Warning: Pin yếu, nên cân nhắc quay về
        ESP_LOGW(TAG, ">> WARNING: Low Battery - Consider Landing");
        
    } else if (voltage < 4.0f || voltage > 9.0f) { 
        // Abnormal: Sai mức điện áp của pin 2S 
        ESP_LOGE(TAG, ">> ABNORMAL: Voltage out of 2S range");   
    }
}

int battery_get_raw_adc(void) {
    if (!is_initialized) return -1;
    int raw = 0;
    if (read_adc_averaged(&raw) == ESP_OK) return raw;
    return -1;
}