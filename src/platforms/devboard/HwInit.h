#pragma once

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void Adc1Init(ADC_HandleTypeDef *hadc);
    void Adc2Init(ADC_HandleTypeDef *hadc);
    void Adc4Init(ADC_HandleTypeDef *hadc);
    void Adc5Init(ADC_HandleTypeDef *hadc);
    
    void Dac4Init(DAC_HandleTypeDef *hdac);
    void Opamp4Init(OPAMP_HandleTypeDef *hopamp);
    
    void Tim6Init(TIM_HandleTypeDef *htim);
    void Tim7Init(TIM_HandleTypeDef *htim);
    
    void Uart1Init(UART_HandleTypeDef *huart1);
    void GpioInit(void);
    void DMAInit(void);
    void Error_Handler(void);
    void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif
