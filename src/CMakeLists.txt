set(MX_Defines_Syms
    USE_HAL_DRIVER
    STM32G473xx
    $<$<CONFIG:Debug>:DEBUG>
)

# This warning is only relevant for the C++ sources.
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>)

if(NOT DEFINED BOARD OR BOARD STREQUAL "")
    set(BOARD "devboard")
endif()

set(BOARD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platforms/${BOARD}")
set(BOARD_GEN_DIR "${BOARD_DIR}/stm32_gen")

if(NOT EXISTS "${BOARD_DIR}")
    message(FATAL_ERROR "Unknown BOARD '${BOARD}' - expected directory ${BOARD_DIR}")
endif()

set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/peleng
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms
    ${BOARD_DIR}
    ${BOARD_GEN_DIR}
    ${PELENG_HAL_DRIVER_DIR}/Inc
    ${PELENG_HAL_DRIVER_DIR}/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
)

set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peleng/Peleng.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peleng/Filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peleng/DelayEstimator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peleng/UartTelemetry.cpp
    ${BOARD_DIR}/Hw.cpp
    ${BOARD_DIR}/HwInit.cpp
    ${BOARD_GEN_DIR}/cordic.c
    ${BOARD_GEN_DIR}/stm32g4xx_it.c
    ${BOARD_GEN_DIR}/stm32g4xx_hal_msp.c
    ${BOARD_GEN_DIR}/stm32g4xx_hal_timebase_tim.c
    ${BOARD_GEN_DIR}/sysmem.c
    ${BOARD_GEN_DIR}/syscalls.c
    ${BOARD_DIR}/startup/startup_stm32g473xx.s
)

set(STM32_Drivers_Src
    ${BOARD_GEN_DIR}/system_stm32g4xx.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_tim.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_tim_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_adc.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_adc_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_ll_adc.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_rcc.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_rcc_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_flash.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_flash_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_flash_ramfunc.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_gpio.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_exti.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_dma.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_dma_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_pwr.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_pwr_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_cortex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_cordic.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_dac.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_dac_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_opamp.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_opamp_ex.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_uart.c
    ${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal_uart_ex.c
)

add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})
target_link_libraries(${CMAKE_PROJECT_NAME} STM32_Drivers)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(FATAL_ERROR "Generated code requires C11 or higher")
endif()
