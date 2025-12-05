/**
 * @file sys_monitor.h
 * @brief System Health Monitoring Module
 * @details Handles memory (RAM) statistics, task stack usage, and system diagnostics.
 */

#ifndef SYS_MONITOR_H
#define SYS_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Print detailed Heap/RAM statistics to console.
 * @note  Includes 'Watermark' (Lowest free heap ever reached).
 */
void sys_mon_check_memory(void);

#ifdef __cplusplus
}
#endif

#endif