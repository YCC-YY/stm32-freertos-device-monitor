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

/*
 * BSP_LED_Init：把 LED 引脚配成推挽输出
 * 逐行解释（都是 STM32 HAL 的固定套路）：
 *   1. __HAL_RCC_GPIOC_CLK_ENABLE()
 *        打开 GPIOC 端口的时钟。STM32 外设默认断电，
 *        必须先开时钟才能操作寄存器（省电设计）。
 *   2. GPIO_InitTypeDef gpio = {0}
 *        定义一个"引脚配置结构体"并清零，然后填三个字段：
 *        Pin   ：选 PC13 这个引脚
 *        Mode  ：GPIO_MODE_OUTPUT_PP = 推挽输出（能输出高/低电平）
 *        Pull  ：GPIO_NOPULL = 不用上下拉（LED 电路自带限流电阻）
 *        Speed ：GPIO_SPEED_FREQ_LOW = 低速，LED 翻转不需要高速
 *   3. HAL_GPIO_Init(port, &gpio)：把上面配置真正写入寄存器
 *   4. HAL_GPIO_WritePin(..., GPIO_PIN_SET)：初始写 1（高电平 = 灭）
 *       因为本板 LED 是低电平点亮（见文件头说明）
 */
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

/*
 * BSP_LED_On：点亮 LED
 * 本板是"低电平点亮"，所以写 RESET(0) = 输出低电平 = 亮
 */
void BSP_LED_On(void)
{
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_RESET);
}

/*
 * BSP_LED_Off：熄灭 LED
 * 写 SET(1) = 输出高电平 = 灭
 */
void BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_SET);
}

/*
 * BSP_LED_Toggle：翻转 LED 状态（亮→灭，灭→亮）
 * MonitorTask 每 2 秒调一次，是"系统还活着"的最直观指示：
 * LED 不闪了，说明监控任务卡住或系统复位了
 */
void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN);
}
