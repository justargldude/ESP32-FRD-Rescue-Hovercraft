/**
 * @file cal_battery.c
 * @brief Implementation of battery monitoring system
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
#include "esp_check.h"  // For ESP_RETURN_ON_ERROR

// --- PRIVATE CONFIGURATION ---
static const char *TAG = "CAL_BATTERY";

#define  MAX_ERROR_COUNT 20

#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_3  // GPIO 4
#define ADC_ATTEN       ADC_ATTEN_DB_2_5  // CRITICAL FIX: Use DB_2_5 for 2S battery!
#define ADC_SAMPLES     50

// Voltage divider (measured values)
#define R1_VAL          22300.0f   // 22kΩ actual
#define R2_VAL          3340.0f   // 3.3kΩ actual
#define VOLT_DIV_RATIO  7.6766f    // Pre-calculated: (R1+R2)/R2

// EMA filter coefficient
#define EMA_ALPHA       0.05f

// --- PRIVATE STATIC VARIABLES ---
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static float voltage_filter_val = 0.0f;
static bool is_initialized = false;
static bool is_calibrated = false;

// --- PRIVATE HELPER FUNCTIONS ---

/**
 * @brief Initialize ADC calibration scheme
 * @return true if calibration successful, false otherwise
 */
static bool init_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool cali_success = false;


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
        }
    }
#endif

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
        }
    }
#endif

    cali_handle = handle;
    return cali_success;
}

/**
 * @brief Read raw ADC with averaging
 * @param[out] out_raw Pointer to store averaged raw value
 * @return ESP_OK on success
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
        
        // Small delay between samples (100us)
        esp_rom_delay_us(100);
    }

    if (valid_samples == 0) {
        return ESP_FAIL;
    }

    *out_raw = adc_raw_sum / valid_samples;
    return ESP_OK;
}

/**
 * @brief Convert raw ADC to GPIO voltage in mV
 * @param raw_adc Raw ADC value
 * @param[out] out_mv Pointer to store voltage in mV
 * @return ESP_OK on success
 */
static esp_err_t raw_to_gpio_voltage(int raw_adc, int *out_mv) {
    if (out_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (is_calibrated && cali_handle != NULL) {
        return adc_cali_raw_to_voltage(cali_handle, raw_adc, out_mv);
    } else {
        // Fallback: Manual calculation for DB_2_5 (0-1250mV range)
        *out_mv = (raw_adc * 1250) / 4095;
        return ESP_OK;
    }
}

// --- PUBLIC API IMPLEMENTATION ---

void battery_init(void) {
    if (is_initialized) {
        return;
    }

    // 1. Initialize ADC Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 2. Configure Channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // 3. Initialize Calibration
    is_calibrated = init_calibration(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN);
    is_initialized = true;
}

float battery_get_voltage(void) {
    if (!is_initialized) {
        return 0.0f;
    }

    // 1. Read averaged raw ADC, convert to VOL and check error
    static int error_count = 0;
    int adc_raw_avg = 0;
    int voltage_gpio_mv = 0;

    if(read_adc_averaged(&adc_raw_avg) || raw_to_gpio_voltage(voltage_gpio_mv, &adc_raw_avg) != ESP_OK){
        error_count++;
        if(error_count == MAX_ERROR_COUNT){
            ESP_LOGI(TAG, "CO LOI VOI ADC! FRD SE QUAY VE");
            return 0.0f;
        }
        return voltage_filter_val;

    }

    error_count = 0;

    // 2. Calculate real battery voltage through voltage divider
    float instant_voltage = (float)voltage_gpio_mv * VOLT_DIV_RATIO / 1000.0f;

    // 3. Apply EMA filter
    if (voltage_filter_val == 0.0f ){
        voltage_filter_val = instant_voltage; // Cold start
    } else {
        voltage_filter_val = (EMA_ALPHA * instant_voltage) + 
                            ((1.0f - EMA_ALPHA) * voltage_filter_val);
    }

    return voltage_filter_val;
}

float battery_get_percentage(void) {
    float voltage = battery_get_voltage();
    
    // Clamp to valid range
    if (voltage >= BATTERY_MAX_V) return 100.0f;
    if (voltage <= BATTERY_MIN_V) return 0.0f;

    // Linear interpolation
    float percentage = (voltage - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f;
    return percentage;
}

void battery_check_health(void) {
    float voltage = battery_get_voltage();
    float percentage = battery_get_percentage();

    if (voltage < BATTERY_CRIT_V) {
        ESP_LOGE(TAG, ": %.2fV (%.1f%%) - DANG QUAY VE!", voltage, percentage);
        // TODO: Add emergency shutdown logic here
    } else if (voltage < BATTERY_MIN_V) {
        ESP_LOGW(TAG, "%.1f%% (%.2fV) - CHU Y SAC!", percentage, voltage);
        // TODO: Add low battery warning (LED, buzzer, etc.)
    } else if (voltage < 5.0f || voltage > 9.0f) {
        ESP_LOGE(TAG, "Sai muc dien ap: %.2fV - NGAT KET NOI PIN NGAY", voltage);
        // TODO: Add warning (LED, buzzer, etc.)
    } else {
        ESP_LOGI(TAG, "%.1f%% (%.2fV)", percentage, voltage);
    }
}

int battery_get_raw_adc(void) {
    if (!is_initialized) {
        return -1;
    }

    int raw = 0;
    if (read_adc_averaged(&raw) == ESP_OK) {
        return raw;
    }
    return -1;
}