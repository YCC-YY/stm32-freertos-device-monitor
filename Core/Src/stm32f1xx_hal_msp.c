/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f1xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/*
 * ============================================================
 * 【分层导读】Core 层 —— stm32f1xx_hal_msp.c（HAL 底层支撑，CubeMX 生成）
 *
 * MSP = MCU Support Package（单片机支撑包）：HAL 只负责"外设怎么用"，
 * 而"外设靠哪些引脚、时钟、中断优先级"这类芯片级细节由 MSP 函数完成。
 *
 * 本文件只有 HAL_MspInit（全局初始化，外设级 MspInit 在各外设文件里）：
 *   __HAL_RCC_AFIO_CLK_ENABLE()   打开 AFIO 时钟（引脚重映射/调试口配置需要）
 *   __HAL_RCC_PWR_CLK_ENABLE()    打开电源管理时钟
 *   HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0)
 *        PendSV 是 FreeRTOS 做任务切换用的异常，必须设为最低优先级 15，
 *        否则任务切换会打断其他中断，导致系统不稳
 *   __HAL_AFIO_REMAP_SWJ_NOJTAG() 关闭 JTAG、只保留 SWD 调试口，
 *        这样 PB3/PB4/PA15 这几个 JTAG 占用的引脚可以当普通 IO 用
 * ============================================================
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  /** NOJTAG: JTAG-DP Disabled and SW-DP Enabled
  */
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
