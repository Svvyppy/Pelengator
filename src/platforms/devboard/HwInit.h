#pragma once

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
namespace hydrv::hw {

void initializeAdc1(ADC_HandleTypeDef* adc);
void initializeAdc2(ADC_HandleTypeDef* adc);
void initializeAdc4(ADC_HandleTypeDef* adc);
void initializeAdc5(ADC_HandleTypeDef* adc);
void initializeDac4(DAC_HandleTypeDef* dac);
void initializeOpamp4(OPAMP_HandleTypeDef* opamp);
void initializeSamplingTimer(TIM_HandleTypeDef* timer);
void initializeServiceTimer(TIM_HandleTypeDef* timer);
void initializeUart1(UART_HandleTypeDef* uart);
void initializeGpio();
void initializeDma();
void configureSystemClock();

} // namespace hydrv::hw

extern "C" [[noreturn]] void Error_Handler(void);
#else
void Error_Handler(void);
#endif
