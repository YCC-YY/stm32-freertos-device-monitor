/* USER CODE BEGIN Header */
/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */
/* USER CODE END Header */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * These parameters and more are described within the 'configuration' section of the
 * FreeRTOS API documentation available on the FreeRTOS.org web site.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/*
 * ============================================================
 * 【分层导读】Core 层 —— FreeRTOSConfig.h（FreeRTOS 全局配置）
 *
 * 这个文件决定了"FreeRTOS 怎么运行"，初学者重点看这几项：
 *   configTICK_RATE_HZ = 1000
 *       → 1 秒 1000 个 tick，即 1 tick = 1 毫秒。
 *         这就是 App_GetTick() 返回毫秒、pdMS_TO_TICKS(1000)=1000 的原因。
 *   configTOTAL_HEAP_SIZE = 10240
 *       → FreeRTOS 自己的堆大小（字节）。所有任务栈、队列、互斥锁都从这里分配；
 *         stats 命令里的 FreeHeap / MinEverHeap 就是它。
 *         改小会导致创建任务失败，改大要确认芯片 RAM 够（C8T6 只有 20KB）。
 *   configMAX_PRIORITIES = 56
 *       → 优先级数量（0 最低，数字越大越优先）。本项目只用了 1/2/3。
 *   configUSE_MUTEXES = 1
 *       → 允许使用互斥锁（statusMutex / i2cMutex 依赖它）。
 *   configUSE_TRACE_FACILITY = 1
 *       → 打开后才有 uxTaskGetStackHighWaterMark / uxQueueMessagesWaiting，
 *         stats 命令的"队列占用 + 栈水位"靠它。
 *   configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5
 *       → 只有优先级数值 ≥5 的中断里，才允许调用 FreeRTOS 的 FromISR 函数
 *         （数值越大优先级越低）。串口中断的优先级必须在 5 以下（数值≥5），
 *         否则 vTaskNotifyGiveFromISR 行为未定义。
 *   configASSERT
 *       → 断言失败时关中断并死循环，调试时能立刻停在出错点。
 *
 * 改这里有风险：栈/堆/优先级都是"牵一发动全身"，改完先看 stats 命令。
 * ============================================================ */

/* USER CODE BEGIN Includes */
/* Section where include file can be added */
/* USER CODE END Includes */

/* Ensure definitions are only used by the compiler, and not by the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32f1xx.h"
#endif /* CMSIS_device_header */

#define configUSE_PREEMPTION                     1    /* 1=抢占式调度：高优先级任务就绪会立刻抢 CPU */
#define configSUPPORT_STATIC_ALLOCATION          1    /* 支持静态创建（内存由用户提供） */
#define configSUPPORT_DYNAMIC_ALLOCATION         1    /* 支持动态创建：内存从 FreeRTOS 堆里分配，本项目用这个方案 */
#define configUSE_IDLE_HOOK                      0    /* 空闲钩子：0=不启用 */
#define configUSE_TICK_HOOK                      0    /* tick 钩子：0=不启用 */
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )  /* CPU 频率，取 72MHz */
#define configTICK_RATE_HZ                       ((TickType_t)1000)   /* 1 秒 1000 tick → 1 tick=1ms */
#define configMAX_PRIORITIES                     ( 56 )               /* 优先级档数，数值越大越优先 */
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)      /* 空闲任务的最小栈（单位 word=4字节） */
/* 4 tasks + timer/idle + queues need more than CubeMX default 3072 */
#define configTOTAL_HEAP_SIZE                    ((size_t)10240)      /* FreeRTOS 堆总大小(字节)，任务栈/队列都从这里来 */
#define configMAX_TASK_NAME_LEN                  ( 16 )               /* 任务名最大长度 */
#define configUSE_TRACE_FACILITY                 1                   /* 1=开启统计接口（栈水位/队列计数靠它） */
#define configUSE_16_BIT_TICKS                   0                   /* 0=32 位 tick（49 天才溢出，够用） */
#define configUSE_MUTEXES                        1                   /* 1=允许互斥锁（statusMutex/i2cMutex 依赖） */
#define configQUEUE_REGISTRY_SIZE                8                   /* 队列注册表容量（调试器查看队列用） */
#define configUSE_RECURSIVE_MUTEXES              1                   /* 允许递归互斥锁 */
#define configUSE_COUNTING_SEMAPHORES            1                   /* 允许计数信号量 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0                   /* 0=用通用方式选下一个任务（Cortex-M3 通用实现） */

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( 2 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

/* The following flag must be enabled only when using newlib */
#define configUSE_NEWLIB_REENTRANT          1

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskCleanUpResources       0
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTimerPendFunctionCall      1
#define INCLUDE_xQueueGetMutexHolder        1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_eTaskGetState               1

/*
 * The CMSIS-RTOS V2 FreeRTOS wrapper is dependent on the heap implementation used
 * by the application thus the correct define need to be enabled below
 */
#define USE_FreeRTOS_HEAP_4   /* 使用 heap_4.c 管理内存：能合并相邻空闲块，适合反复创建/删除 */

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
 /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
 #define configPRIO_BITS         __NVIC_PRIO_BITS
#else
 #define configPRIO_BITS         4
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
/* 只有优先级数值 >= 5 的中断，才允许调用 FreeRTOS 的 xxxFromISR 接口
 * （数值越大优先级越低；数值 <5 的高优先级中断里调用会导致系统崩溃） */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
/* USER CODE BEGIN 1 */
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}
/* USER CODE END 1 */

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

/* IMPORTANT: After 10.3.1 update, Systick_Handler comes from NVIC (if SYS timebase = systick), otherwise from cmsis_os2.c */

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0

/* USER CODE BEGIN Defines */
/* Section where parameter definitions can be added (for instance, to override default ones in FreeRTOS.h) */
/* USER CODE END Defines */

#endif /* FREERTOS_CONFIG_H */
