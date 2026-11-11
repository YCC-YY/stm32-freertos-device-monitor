#ifndef BSP_LED_H
#define BSP_LED_H

/*
 * bsp_led.h —— 板载 LED 接口（BSP 层）
 *
 * BSP = Board Support Package（板级支持包），把"哪个引脚、什么电平"封装在
 * bsp_led.c 内部，上层任务（如 MonitorTask）只调用这里的函数，不直接碰寄存器。
 * 以后换开发板/换 LED 引脚，只改 bsp_led.c，不用动任务代码。
 *
 * 本项目：PC13，低电平点亮（蓝丸常见接法）。
 *
 * 接口：
 *   BSP_LED_Init()    初始化引脚（配置为推挽输出），默认熄灭
 *   BSP_LED_On()      点亮
 *   BSP_LED_Off()     熄灭
 *   BSP_LED_Toggle()  翻转（MonitorTask 每周期调用，肉眼可见"活着"）
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_LED_Init(void);
void BSP_LED_On(void);
void BSP_LED_Off(void);
void BSP_LED_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
