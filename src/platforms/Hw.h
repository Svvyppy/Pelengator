#pragma once

#include <stdbool.h>

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Bundle of initialized STM32 HAL peripheral handles.
     */
    struct HwInstances
    {
        ADC_HandleTypeDef hadc1;
        ADC_HandleTypeDef hadc2;
        ADC_HandleTypeDef hadc4;
        ADC_HandleTypeDef hadc5;
        DAC_HandleTypeDef hdac4;

        OPAMP_HandleTypeDef hopamp4;
        TIM_HandleTypeDef htim6;
        TIM_HandleTypeDef htim7;
        UART_HandleTypeDef huart1;
        TIM_HandleTypeDef htim15;
    };

    /**
     * @brief Return pointer to global hardware handles.
     */
    struct HwInstances *GetHwInstances(void);

    /**
     * @brief Initialize board peripherals required by the firmware.
     * @return true on success.
     */
    bool InitHw(void);

#ifdef __cplusplus
}
#endif
