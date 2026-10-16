# CLion / CMake 接入片段
# 在 CubeMX 生成的 CMakeLists.txt 中合并以下内容（路径按实际工程根调整）。

cmake_minimum_required(VERSION 3.20)
# project(DeviceMonitor C)  # 由 CubeMX 已生成则不要重复

# ---- 增加 include ----
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/RingBuffer
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/CLI
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)

# ---- 增加源文件（在 add_executable / add_library 的 sources 列表中追加）----
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/sensor_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/comm_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/logger_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/monitor_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Src/sensor_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Src/uart_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/RingBuffer/ring_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/CLI/cli.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Src/bsp_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/freertos_app.c
)
# target_sources(${PROJECT_NAME} PRIVATE ${APP_SOURCES})
