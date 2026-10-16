/**
 * Core/ — CubeMX 生成区（占位）
 *
 * 本仓库不提交完整 CubeMX 工程，避免与你本机芯片型号/外设配置冲突。
 * 请在 STM32CubeMX 中生成代码后，将生成物放在此目录，并做如下接入：
 *
 * 1) 芯片与外设
 *    - FreeRTOS (CMSIS_RTOS_V2 或 CMSIS_RTOS_V1，按 CubeMX 版本)
 *    - USARTx：异步，115200 8N1，开启全局中断
 *    - I2Cx：传感器总线
 *    - 可选 IWDG
 *
 * 2) FreeRTOSConfig.h 建议
 *    #define configUSE_MUTEXES                 1
 *    #define configUSE_TASK_NOTIFICATIONS      1
 *    #define configUSE_COUNTING_SEMAPHORES     1
 *    #define configCHECK_FOR_STACK_OVERFLOW    2
 *    #define configUSE_TRACE_FACILITY          1  // 便于 stats
 *    堆大小按任务栈合计预留（至少 4*256*4 + 队列存储）
 *
 * 3) main.c 接入（USER CODE BEGIN 2）
 *    MX_USARTx_UART_Init();
 *    UART_Driver_BindHandle(&huart1);
 *    // 不要再调用 MX_FREERTOS_Init + osKernelStart 的旧路径混用
 *    App_StartScheduler();  // 内部创建对象、任务并 vTaskStartScheduler
 *
 * 4) 中断接入 stm32xxxx_it.c
 *    void USARTx_IRQHandler(void)
 *    {
 *        HAL_UART_IRQHandler(&huart1);
 *        // 或直接：
 *        // if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
 *        //     uint8_t b = (uint8_t)(huart1.Instance->DR & 0xFF);
 *        //     UART_Driver_RxISR(b);
 *        // }
 *    }
 *    若使用 HAL 接收回调：
 *    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 *    {
 *        UART_Driver_RxISR(rx_byte);
 *        HAL_UART_Receive_IT(huart, &rx_byte, 1);
 *    }
 *
 * 5) CLion / CMake
 *    - 用 CubeMX 插件打开 .ioc，或打开生成的 CMakeLists.txt
 *    - 将本仓库 App/ Tasks/ Drivers/ Middleware/ BSP/ 的 .c 加入 target_sources
 *    - include_directories 增加各 Inc 与 Middleware 子目录
 *
 * 6) 不要把业务代码写进 USER CODE 区之外的生成区；重新 Generate Code 不会覆盖
 *    App/Tasks/Drivers/Middleware/BSP。
 */
