#include "bsp_led.h"
#include "stm32f1xx_hal.h" /* adjust with your CubeMX device header */

#ifndef BSP_LED_GPIO_PORT
#define BSP_LED_GPIO_PORT GPIOC
#endif
#ifndef BSP_LED_GPIO_PIN
#define BSP_LED_GPIO_PIN  GPIO_PIN_13
#endif

void BSP_LED_Init(void)
{
    /* Prefer CubeMX-generated MX_GPIO_Init for pin clocks.
     * This fallback enables basic push-pull output if user assigns pins. */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = BSP_LED_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_LED_GPIO_PORT, &gpio);
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_SET);
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
