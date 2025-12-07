/**
 * @file sys_monitor.c
 * @brief Implementation of System Monitor
 */

#include "sys_monitor.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

// Standard Tag for System Monitor
static const char *TAG = "SYS_MON"; 

void sys_mon_check_memory(void) {
    // Get Real-time Memory Stats
    
    // Current Free Heap 
    size_t free_heap = esp_get_free_heap_size();
    
    // Minimum Free Heap
    // Risk of crash if too low
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    
    // Total Available Heap
    size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    
    // Calculate Usage Percentage
    size_t used_heap = total_heap - free_heap;
    float used_pct = (float)used_heap * 100.0f / total_heap;

    // Diagnostic Report
    ESP_LOGI(TAG, "========== MEMORY DIAGNOSTICS ==========");
    ESP_LOGI(TAG, "Total Heap:    %6u B  (%u KB)", total_heap, total_heap / 1024);
    ESP_LOGI(TAG, "Current Used:  %6u B  (%.1f%%)", used_heap, used_pct);
    ESP_LOGI(TAG, "Current Free:  %6u B", free_heap);
    
    // Highlight the Watermark
    ESP_LOGI(TAG, "Min Free Ever: %6u B  (Watermark)", min_free_heap); 

    // Warning Logic
    // Cảnh báo rò rỉ bộ nhớ hoặc thiếu RAM
    if (min_free_heap < 10000) { // Threshold: 10KB
        ESP_LOGW(TAG, "WARNING: Low Memory Watermark! Check for leaks.");
    }

    ESP_LOGI(TAG, "========================================");
}