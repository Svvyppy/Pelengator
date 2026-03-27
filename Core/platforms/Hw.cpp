#include "Hw.h"

#include "HwInit.h"
#include "cordic.h"

// Singleton implementation
HwInstances *GetHwInstances(void)
{
    static HwInstances instances = {};
    return &instances;
}

bool InitHw(void)
{
    HwInstances *hw = GetHwInstances();

    HAL_Init();
    SystemClock_Config();

    GpioInit();
    DMAInit();
    Tim6Init(&hw->htim6);
    Tim7Init(&hw->htim7);
    Uart1Init(&hw->huart1);
    Dac3Init(&hw->hdac3);
    Dac4Init(&hw->hdac4);
    Opamp1Init(&hw->hopamp1);
    Opamp3Init(&hw->hopamp3);
    Opamp4Init(&hw->hopamp4);
    Opamp5Init(&hw->hopamp5);
    Adc1Init(&hw->hadc1);
    Adc2Init(&hw->hadc2);
    Adc3Init(&hw->hadc3);
    Adc5Init(&hw->hadc5);
    MX_CORDIC_Init();

    return true;
}

void DMA1_Channel1_IRQHandler(void)
{
    /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

    /* USER CODE END DMA1_Channel1_IRQn 0 */
    HAL_DMA_IRQHandler(GetHwInstances()->hadc1.DMA_Handle);
    /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

    /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel2 global interrupt.
 */
void DMA1_Channel2_IRQHandler(void)
{
    /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */

    /* USER CODE END DMA1_Channel2_IRQn 0 */
    HAL_DMA_IRQHandler(GetHwInstances()->hadc2.DMA_Handle);
    /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */

    /* USER CODE END DMA1_Channel2_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel3 global interrupt.
 */
void DMA1_Channel3_IRQHandler(void)
{
    /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */

    /* USER CODE END DMA1_Channel3_IRQn 0 */
    HAL_DMA_IRQHandler(GetHwInstances()->hadc3.DMA_Handle);
    /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

    /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel4 global interrupt.
 */
void DMA1_Channel4_IRQHandler(void)
{
    /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */

    /* USER CODE END DMA1_Channel4_IRQn 0 */
    HAL_DMA_IRQHandler(GetHwInstances()->hadc5.DMA_Handle);
    /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */

    /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
 * @brief This function handles TIM1 break interrupt and TIM15 global interrupt.
 */
void TIM1_BRK_TIM15_IRQHandler(void)
{
    /* USER CODE BEGIN TIM1_BRK_TIM15_IRQn 0 */

    /* USER CODE END TIM1_BRK_TIM15_IRQn 0 */
    HAL_TIM_IRQHandler(&GetHwInstances()->htim15);
    /* USER CODE BEGIN TIM1_BRK_TIM15_IRQn 1 */

    /* USER CODE END TIM1_BRK_TIM15_IRQn 1 */
}

/**
 * @brief This function handles TIM6 global interrupt, DAC1 and DAC3 channel
 * underrun error interrupts.
 */
void TIM6_DAC_IRQHandler(void)
{
    /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

    /* USER CODE END TIM6_DAC_IRQn 0 */
    HAL_TIM_IRQHandler(&GetHwInstances()->htim6);
    HAL_DAC_IRQHandler(&GetHwInstances()->hdac3);
    /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

    /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
 * @brief This function handles TIM7 global interrupt, DAC2 and DAC4 channel
 * underrun error interrupts.
 */
void TIM7_DAC_IRQHandler(void)
{
    /* USER CODE BEGIN TIM7_DAC_IRQn 0 */

    /* USER CODE END TIM7_DAC_IRQn 0 */
    HAL_TIM_IRQHandler(&GetHwInstances()->htim7);
    HAL_DAC_IRQHandler(&GetHwInstances()->hdac4);
    /* USER CODE BEGIN TIM7_DAC_IRQn 1 */

    /* USER CODE END TIM7_DAC_IRQn 1 */
}