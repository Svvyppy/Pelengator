#pragma once

#include <stdbool.h>

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct HwInstances
    {
        ADC_HandleTypeDef hadc1;
        ADC_HandleTypeDef hadc2;
        ADC_HandleTypeDef hadc3;
        ADC_HandleTypeDef hadc5;
        DAC_HandleTypeDef hdac3;
        DAC_HandleTypeDef hdac4;
        OPAMP_HandleTypeDef hopamp1;
        OPAMP_HandleTypeDef hopamp3;
        OPAMP_HandleTypeDef hopamp4;
        OPAMP_HandleTypeDef hopamp5;
        TIM_HandleTypeDef htim6;
        TIM_HandleTypeDef htim7;
        UART_HandleTypeDef huart1;
        TIM_HandleTypeDef htim15;
    };

    struct HwInstances *GetHwInstances(void);

    bool InitHw(void);

#ifdef __cplusplus
}
#endif
