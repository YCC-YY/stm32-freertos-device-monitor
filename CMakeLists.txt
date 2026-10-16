cmake_minimum_required(VERSION 3.22)

# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Key)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        # ========== App 层 ==========
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_status.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_tasks.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/app_scheduler.c    # 【新增】调度器入口，对应App_StartScheduler
        # ========== 任务层 ==========
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/sensor_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/comm_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/logger_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Src/monitor_task.c
        # Tasks headers —— 明确交给 IDE/CMake 管理
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Inc/sensor_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Inc/comm_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Inc/logger_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Inc/monitor_task.h
        # ========== 业务驱动层 ==========
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Src/sensor_driver.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Src/bme280.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Src/uart_driver.c
        # ========== 中间件 ==========
        ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/CLI/cli.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/CLI/comm_channel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/RingBuffer/ring_buffer.c
        # ========== 板级支持 ==========
        ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Src/bsp_led.c
        # ========== 临时兼容：后续建议移到App层 ==========
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/freertos_app.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # 业务层头文件
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/CLI
        ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/RingBuffer
        ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Inc
        # 【新增】CubeMX生成的核心头文件（main.h / 外设句柄声明等）
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
        # Add user defined libraries
)
