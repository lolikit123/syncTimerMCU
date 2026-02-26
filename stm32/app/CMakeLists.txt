cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

project(syncTimeMCU_stm32 C ASM)

set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
add_compile_options(${MCU_FLAGS} -ffunction-sections -fdata-sections -fno-common -Wall -Wextra)
add_link_options(${MCU_FLAGS} -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs)

set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "" FORCE)
set(FREERTOS_HEAP 4 CACHE STRING "" FORCE)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/FreeRTOS-Kernel
                 ${CMAKE_BINARY_DIR}/freertos-kernel)

# libopencm3 must be build first for lib/libopencm3_stm32f4.a
add_library(opencm3_stm32f4 STATIC IMPORTED GLOBAL)
set_target_properties(opencm3_stm32f4 PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libopencm3/lib/libopencm3_stm32f4.a)

option(APP_ENABLE_TIMEBASE_TESTS "Enable timebase acceptance tests" OFF)

option(APP_BUILD_MASTER "Build master firmware" ON)
option(APP_BUILD_SLAVE  "Build slave firmware" ON)

if(NOT APP_BUILD_MASTER AND NOT APP_BUILD_SLAVE)
    message(FATAL_ERROR "Enable at least one role: APP_BUILD_MASTER or APP_BUILD_SLAVE")
endif()

set(APP_SOURCES
    src/main.c
    src/timebase.c
    src/wire_codec.c
    src/spi_link.c
    src/sync_master.c
    src/sync_slave.c
)

if(APP_ENABLE_TIMEBASE_TESTS)
    list(APPEND APP_SOURCES src/Test.c)
endif()

function(add_role_target role role_define)
    set(target "app_${role}")

    add_executable(${target} ${APP_SOURCES})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "app_${role}.elf")

    target_compile_definitions(${target} PRIVATE
        STM32F4
        ${role_define}=1
        APP_ENABLE_TIMEBASE_TESTS=$<BOOL:${APP_ENABLE_TIMEBASE_TESTS}>
    )

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libopencm3/include
    )

    target_link_options(${target} PRIVATE
        -T${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f411ceu6.ld
        -Wl,-Map=${CMAKE_BINARY_DIR}/app_${role}.map
        -nostartfiles
    )

    target_link_libraries(${target} PRIVATE
        freertos_kernel
        freertos_config
        opencm3_stm32f4
        c m gcc nosys
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary
                $<TARGET_FILE:${target}>
                ${CMAKE_BINARY_DIR}/app_${role}.bin
        COMMAND arm-none-eabi-size $<TARGET_FILE:${target}>
    )
endfunction()

if(APP_BUILD_MASTER)
    add_role_target(master APP_ROLE_MASTER)
endif()

if(APP_BUILD_SLAVE)
    add_role_target(slave APP_ROLE_SLAVE)
endif()