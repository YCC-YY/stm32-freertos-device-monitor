#include "bsp_led.h"
#include "stm32f1xx_hal.h"

/*
 * bsp_led.c —— 板载 LED（PC13，蓝丸常见接法：低电平点亮）
 *
 * BSP = Board Support Package，板级小封装：
 *   上层任务只调 BSP_LED_Toggle()，不写寄存器，
 *   以后换板子只改这个文件。
 */

#ifndef BSP_LED_GPIO_PORT
#define BSP_LED_GPIO_PORT GPIOC
#endif
#ifndef BSP_LED_GPIO_PIN
#define BSP_LED_GPIO_PIN  GPIO_PIN_13
#endif

void BSP_LED_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = BSP_LED_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_LED_GPIO_PORT, &gpio);
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_SET); /* 默认灭 */
}

void BSP_LED_On(void)
{
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_RESET);
}

void BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_SET);
}

void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN);
}
