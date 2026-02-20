#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* Basic scheduling */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configCPU_CLOCK_HZ                      ((uint32_t)84000000UL)
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS

#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* Memory */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ((size_t)(20 * 1024))

/* Synchronization/features */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

/* Cortex-M interrupt priorities (STM32F4 -> 4 bits) */
#define configPRIO_BITS                               4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY       15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5
#define configKERNEL_INTERRUPT_PRIORITY               (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY          (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;){} }

/* Critical: map FreeRTOS handlers to libopencm3 vector names */
#define vPortSVCHandler      sv_call_handler
#define xPortPendSVHandler   pend_sv_handler
#define xPortSysTickHandler  sys_tick_handler

/*************** Optional API includes ***************/
#define INCLUDE_vTaskDelay                    1
#define INCLUDE_vTaskDelete                   1
#define INCLUDE_vTaskSuspend                  1
#define INCLUDE_xTaskGetCurrentTaskHandle     1
#define INCLUDE_xTaskGetSchedulerState        1
#define INCLUDE_uxTaskPriorityGet             1
#define INCLUDE_vTaskPrioritySet              1

#endif /* FREERTOS_CONFIG_H */