#include "Hw.h"

#include "HwInit.h"
#include "cordic.h"

static hydrv::hw::HwInstances HwInstances{};

static hydrv::hw::SignalRecorder recorder{};

inline void HandleAdcDmaIrq(ADC_HandleTypeDef* hadc)
{
    HAL_DMA_IRQHandler(hadc->DMA_Handle);
}
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    recorder.fullReady();
}

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    recorder.halfReady();
}

bool InitHw(void)
{
    GpioInit();
    DMAInit();
    Tim6Init(recorder.tim6Handle());
    Tim7Init(recorder.tim7Handle());

    Dac4Init(recorder.dac4Handle());
    Opamp4Init(recorder.opamp4Hanle());
    Adc1Init(recorder.adc1Handle());
    Adc2Init(recorder.adc2Handle());
    Adc4Init(recorder.adc4Handle());
    Adc5Init(recorder.adc5Handle());
    HwInstances.recorder = &recorder;
    recorder.startRecord();

    Uart1Init(&hw->huart1);

    HAL_Init();
    SystemClock_Config();

    MX_CORDIC_Init();

    return true;
}

extern "C" void DMA1_Channel1_IRQHandler(void)
{
    HandleAdcDmaIrq(recorder.adc1Handle());
}

/**
 * @brief This function handles DMA1 channel3 global interrupt.
 */
extern "C" void DMA1_Channel3_IRQHandler(void)
{
    HandleAdcDmaIrq(recorder.adc2Handle());
}

/**
 * @brief This function handles DMA1 channel3 global interrupt.
 */
extern "C" void DMA1_Channel4_IRQHandler(void)
{
    HandleAdcDmaIrq(recorder.adc4Handle());
}

/**
 * @brief This function handles DMA1 channel2 global interrupt.
 */
extern "C" void DMA1_Channel2_IRQHandler(void)
{
    HandleAdcDmaIrq(recorder.adc5Handle());
}

/**
 * @brief This function handles TIM1 break interrupt and TIM15 global interrupt.
 */
extern "C" void TIM1_BRK_TIM15_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&GetHwInstances()->htim15);
}

/**
 * @brief This function handles TIM6 global interrupt, DAC1 and DAC3 channel
 * underrun error interrupts.
 */
extern "C" void TIM6_DAC_IRQHandler(void)
{}

/**
 * @brief This function handles TIM7 global interrupt, DAC2 and DAC4 channel
 * underrun error interrupts.
 */
extern "C" void TIM7_DAC_IRQHandler(void)
{
    /* USER CODE BEGIN TIM7_DAC_IRQn 0 */

    /* USER CODE END TIM7_DAC_IRQn 0 */
    HAL_TIM_IRQHandler(recorder.tim7Handle);
    HAL_DAC_IRQHandler(recorder.dac4Handle);
    /* USER CODE BEGIN TIM7_DAC_IRQn 1 */

    /* USER CODE END TIM7_DAC_IRQn 1 */
}

extern "C" void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&GetHwInstances()->huart1);
}
