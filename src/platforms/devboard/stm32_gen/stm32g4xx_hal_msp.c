#include "HwInit.h"
#include "Pins.h"
#include "main.h"

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    HAL_PWREx_DisableUCPDDeadBattery();
}

static uint32_t HAL_RCC_ADC12_CLK_ENABLED = 0;
static uint32_t HAL_RCC_ADC345_CLK_ENABLED = 0;
static DMA_HandleTypeDef hdma_adc1;
static DMA_HandleTypeDef hdma_adc2;
static DMA_HandleTypeDef hdma_adc3;
static DMA_HandleTypeDef hdma_adc5;

void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (adcHandle->Instance == ADC1)
    {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC12;
        PeriphClkInit.Adc12ClockSelection = RCC_ADC12CLKSOURCE_SYSCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        HAL_RCC_ADC12_CLK_ENABLED++;
        if (HAL_RCC_ADC12_CLK_ENABLED == 1)
        {
            __HAL_RCC_ADC12_CLK_ENABLE();
        }

        hdma_adc1.Instance = DMA1_Channel1;
        hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_adc1.Init.Mode = DMA_CIRCULAR;
        hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc1);
    }
    else if (adcHandle->Instance == ADC2)
    {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC12;
        PeriphClkInit.Adc12ClockSelection = RCC_ADC12CLKSOURCE_SYSCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        HAL_RCC_ADC12_CLK_ENABLED++;
        if (HAL_RCC_ADC12_CLK_ENABLED == 1)
        {
            __HAL_RCC_ADC12_CLK_ENABLE();
        }

        hdma_adc2.Instance = DMA1_Channel2;
        hdma_adc2.Init.Request = DMA_REQUEST_ADC2;
        hdma_adc2.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc2.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc2.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_adc2.Init.Mode = DMA_CIRCULAR;
        hdma_adc2.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_adc2) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc2);
    }
    else if (adcHandle->Instance == ADC3)
    {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC345;
        PeriphClkInit.Adc345ClockSelection = RCC_ADC345CLKSOURCE_SYSCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }
        HAL_RCC_ADC345_CLK_ENABLED++;
        if (HAL_RCC_ADC345_CLK_ENABLED == 1)
        {
            __HAL_RCC_ADC345_CLK_ENABLE();
        }

        hdma_adc3.Instance = DMA1_Channel3;
        hdma_adc3.Init.Request = DMA_REQUEST_ADC3;
        hdma_adc3.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc3.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc3.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc3.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc3.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_adc3.Init.Mode = DMA_CIRCULAR;
        hdma_adc3.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_adc3) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc3);
    }
    else if (adcHandle->Instance == ADC5)
    {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC345;
        PeriphClkInit.Adc345ClockSelection = RCC_ADC345CLKSOURCE_SYSCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        HAL_RCC_ADC345_CLK_ENABLED++;
        if (HAL_RCC_ADC345_CLK_ENABLED == 1)
        {
            __HAL_RCC_ADC345_CLK_ENABLE();
        }

        hdma_adc5.Instance = DMA1_Channel4;
        hdma_adc5.Init.Request = DMA_REQUEST_ADC5;
        hdma_adc5.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc5.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc5.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc5.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc5.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_adc5.Init.Mode = DMA_CIRCULAR;
        hdma_adc5.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_adc5) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc5);
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *adcHandle)
{
    if (adcHandle->Instance == ADC1)
    {
        HAL_RCC_ADC12_CLK_ENABLED--;
        if (HAL_RCC_ADC12_CLK_ENABLED == 0)
        {
            __HAL_RCC_ADC12_CLK_DISABLE();
        }

        HAL_DMA_DeInit(adcHandle->DMA_Handle);
    }
    else if (adcHandle->Instance == ADC2)
    {
        HAL_RCC_ADC12_CLK_ENABLED--;
        if (HAL_RCC_ADC12_CLK_ENABLED == 0)
        {
            __HAL_RCC_ADC12_CLK_DISABLE();
        }

        HAL_DMA_DeInit(adcHandle->DMA_Handle);
    }
    else if (adcHandle->Instance == ADC3)
    {
        HAL_RCC_ADC345_CLK_ENABLED--;
        if (HAL_RCC_ADC345_CLK_ENABLED == 0)
        {
            __HAL_RCC_ADC345_CLK_DISABLE();
        }

        HAL_DMA_DeInit(adcHandle->DMA_Handle);
    }
    else if (adcHandle->Instance == ADC5)
    {
        HAL_RCC_ADC345_CLK_ENABLED--;
        if (HAL_RCC_ADC345_CLK_ENABLED == 0)
        {
            __HAL_RCC_ADC345_CLK_DISABLE();
        }

        HAL_DMA_DeInit(adcHandle->DMA_Handle);
    }
}

void HAL_DAC_MspInit(DAC_HandleTypeDef *dacHandle)
{
    if (dacHandle->Instance == DAC3)
    {
        __HAL_RCC_DAC3_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    }
    else if (dacHandle->Instance == DAC4)
    {
        __HAL_RCC_DAC4_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM7_DAC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM7_DAC_IRQn);
    }
}

void HAL_DAC_MspDeInit(DAC_HandleTypeDef *dacHandle)
{
    if (dacHandle->Instance == DAC3)
    {
        __HAL_RCC_DAC3_CLK_DISABLE();
    }
    else if (dacHandle->Instance == DAC4)
    {
        __HAL_RCC_DAC4_CLK_DISABLE();
    }
}

void HAL_OPAMP_MspInit(OPAMP_HandleTypeDef *opampHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (opampHandle->Instance == OPAMP1)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = CH1_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(CH1_GPIO_Port, &GPIO_InitStruct);
    }
    else if (opampHandle->Instance == OPAMP3)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = CH2_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(CH2_GPIO_Port, &GPIO_InitStruct);
    }
    else if (opampHandle->Instance == OPAMP4)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = CH3_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(CH3_GPIO_Port, &GPIO_InitStruct);
    }
    else if (opampHandle->Instance == OPAMP5)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = CH4_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(CH4_GPIO_Port, &GPIO_InitStruct);
    }
}

void HAL_OPAMP_MspDeInit(OPAMP_HandleTypeDef *opampHandle)
{
    if (opampHandle->Instance == OPAMP1)
    {
        HAL_GPIO_DeInit(CH1_GPIO_Port, CH1_Pin);
    }
    else if (opampHandle->Instance == OPAMP3)
    {
        HAL_GPIO_DeInit(CH2_GPIO_Port, CH2_Pin);
    }
    else if (opampHandle->Instance == OPAMP4)
    {
        HAL_GPIO_DeInit(CH3_GPIO_Port, CH3_Pin);
    }
    else if (opampHandle->Instance == OPAMP5)
    {
        HAL_GPIO_DeInit(CH4_GPIO_Port, CH4_Pin);
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM6)
    {
        __HAL_RCC_TIM6_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    }
    else if (tim_baseHandle->Instance == TIM7)
    {
        __HAL_RCC_TIM7_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM7_DAC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM7_DAC_IRQn);
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM6)
    {
        __HAL_RCC_TIM6_CLK_DISABLE();
    }
    else if (tim_baseHandle->Instance == TIM7)
    {
        __HAL_RCC_TIM7_CLK_DISABLE();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (uartHandle->Instance == USART1)
    {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{
    if (uartHandle->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
    }
}
