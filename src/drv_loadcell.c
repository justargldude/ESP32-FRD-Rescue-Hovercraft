#include <stdint.h>
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
static bool is_cout = false;
static esp_err_t err;
static int32_t value = 0;

esp_err_t init_loadcell(void){
    gpio_config_t PD_SCK = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PD_SCK_FRONT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    err = gpio_config(&PD_SCK);
    if (err != ESP_OK) return err;

    gpio_config_t DOUT = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << DOUT_FRONT),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    err = gpio_config(&DOUT);
    if (err != ESP_OK) return err;
    
    gpio_set_level(PD_SCK_FRONT, 0);

    is_initialized = true;
    return ESP_OK;
}

void loadcell_cout(void){
    if (!is_initialized) return US_ERROR_CODE;

    while (gpio_get_level(DOUT_FRONT) == 1) {
        ets_delay_us(5);
        return US_ERROR_CODE;
    }
    value = 0;
    for (int i = 0; i < 24; i++){
        value <<= 1; 

        gpio_set_level(PD_SCK_FRONT, 1);
        ets_delay_us(2);

        if (gpio_get_level(DOUT_FRONT))
            value |= 1;

        gpio_set_level(PD_SCK_FRONT, 0);
        ets_delay_us(2);
    }

    gpio_set_level(PD_SCK_FRONT, 1);
    ets_delay_us(2);
    gpio_set_level(PD_SCK_FRONT, 0);
    is_cout = true;
}
void chuyen_du_lieu(){
    if(value & (1 << 23)){
        value |= HIEU_CHUAN;
    }
}
