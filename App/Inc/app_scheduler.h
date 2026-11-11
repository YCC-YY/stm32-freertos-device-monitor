#ifndef APP_SCHEDULER_H
#define APP_SCHEDULER_H

/*
 * app_scheduler.h —— 系统启动调度入口（App 层）
 *
 * 整个程序只有这一个"点火"函数：
 *   main() 完成时钟/外设初始化后调用 App_StartScheduler()，
 *   它内部负责：初始化状态 → 建队列/锁 → 初始化串口 → 创建四个任务 → vTaskStartScheduler()。
 *
 * 注意：调度器启动成功后此函数永不返回，main() 里它后面的代码不会再执行。
 */

/* 启动 FreeRTOS 调度器（见 app_scheduler.c） */
void App_StartScheduler(void);

#endif /* APP_SCHEDULER_H */

