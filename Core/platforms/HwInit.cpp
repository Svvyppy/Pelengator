#include "HwInit.h"

#include "Pins.h"

void Adc1Init(ADC_HandleTypeDef *hadc)
{
    ADC_MultiModeTypeDef multimode = {0};
    ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc->Instance = ADC1;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.GainCompensation = 0;
    hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc->Init.LowPowerAutoWait = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc->Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(hadc) != HAL_OK)
    {
        Error_Handler();
    }

    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(hadc, &multimode) != HAL_OK)
    {
        Error_Handler();
    }

    AnalogWDGConfig.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
    AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
    AnalogWDGConfig.Channel = ADC_CHANNEL_VOPAMP1;
    AnalogWDGConfig.ITMode = ENABLE;
    AnalogWDGConfig.HighThreshold = 3710;
    AnalogWDGConfig.LowThreshold = 0;
    AnalogWDGConfig.FilteringConfig = ADC_AWD_FILTERING_NONE;
    if (HAL_ADC_AnalogWDGConfig(hadc, &AnalogWDGConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_VOPAMP1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Adc2Init(ADC_HandleTypeDef *hadc)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc->Instance = ADC2;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.GainCompensation = 0;
    hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc->Init.LowPowerAutoWait = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc->Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(hadc) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_VOPAMP3_ADC2;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Adc3Init(ADC_HandleTypeDef *hadc)
{
    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc->Instance = ADC3;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.GainCompensation = 0;
    hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc->Init.LowPowerAutoWait = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc->Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(hadc) != HAL_OK)
    {
        Error_Handler();
    }

    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(hadc, &multimode) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_VOPAMP3_ADC3;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Adc5Init(ADC_HandleTypeDef *hadc)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc->Instance = ADC5;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.GainCompensation = 0;
    hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc->Init.LowPowerAutoWait = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc->Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(hadc) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_VOPAMP5;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Dac3Init(DAC_HandleTypeDef *hdac)
{
    DAC_ChannelConfTypeDef sConfig = {0};

    hdac->Instance = DAC3;
    if (HAL_DAC_Init(hdac) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
    sConfig.DAC_DMADoubleDataMode = DISABLE;
    sConfig.DAC_SignedFormat = DISABLE;
    sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
    sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    if (HAL_DAC_ConfigChannel(hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_DAC_ConfigChannel(hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
}

void Dac4Init(DAC_HandleTypeDef *hdac)
{
    DAC_ChannelConfTypeDef sConfig = {0};

    hdac->Instance = DAC4;
    if (HAL_DAC_Init(hdac) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
    sConfig.DAC_DMADoubleDataMode = DISABLE;
    sConfig.DAC_SignedFormat = DISABLE;
    sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
    sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    if (HAL_DAC_ConfigChannel(hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_DAC_ConfigChannel(hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
}

void Opamp1Init(OPAMP_HandleTypeDef *hopamp)
{
    hopamp->Instance = OPAMP1;
    hopamp->Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
    hopamp->Init.Mode = OPAMP_PGA_MODE;
    hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
    hopamp->Init.InternalOutput = ENABLE;
    hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
    hopamp->Init.PgaGain = OPAMP_PGA_GAIN_64_OR_MINUS_63;
    hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
    if (HAL_OPAMP_Init(hopamp) != HAL_OK)
    {
        Error_Handler();
    }
}

void Opamp3Init(OPAMP_HandleTypeDef *hopamp)
{
    hopamp->Instance = OPAMP3;
    hopamp->Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
    hopamp->Init.Mode = OPAMP_PGA_MODE;
    hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
    hopamp->Init.InternalOutput = ENABLE;
    hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
    hopamp->Init.PgaGain = OPAMP_PGA_GAIN_64_OR_MINUS_63;
    hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
    if (HAL_OPAMP_Init(hopamp) != HAL_OK)
    {
        Error_Handler();
    }
}

void Opamp4Init(OPAMP_HandleTypeDef *hopamp)
{
    hopamp->Instance = OPAMP4;
    hopamp->Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
    hopamp->Init.Mode = OPAMP_PGA_MODE;
    hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
    hopamp->Init.InternalOutput = ENABLE;
    hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
    hopamp->Init.PgaGain = OPAMP_PGA_GAIN_64_OR_MINUS_63;
    hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
    if (HAL_OPAMP_Init(hopamp) != HAL_OK)
    {
        Error_Handler();
    }
}

void Opamp5Init(OPAMP_HandleTypeDef *hopamp)
{
    hopamp->Instance = OPAMP5;
    hopamp->Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
    hopamp->Init.Mode = OPAMP_PGA_MODE;
    hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
    hopamp->Init.InternalOutput = ENABLE;
    hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
    hopamp->Init.PgaGain = OPAMP_PGA_GAIN_64_OR_MINUS_63;
    hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
    if (HAL_OPAMP_Init(hopamp) != HAL_OK)
    {
        Error_Handler();
    }
}

void Tim6Init(TIM_HandleTypeDef *htim)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim->Instance = TIM6;
    htim->Init.Prescaler = 169;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = 3;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(htim) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Tim7Init(TIM_HandleTypeDef *htim)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim->Instance = TIM7;
    htim->Init.Prescaler = 16999;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = 4;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(htim) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Uart1Init(UART_HandleTypeDef *huart)
{
    huart->Instance = USART1;
    huart->Init.BaudRate = 115200;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK)
    {
        Error_Handler();
    }
}

void GpioInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = TX_En_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TX_En_GPIO_Port, &GPIO_InitStruct);
}

void DMAInit(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Channel1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    /* DMA1_Channel2_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    /* DMA1_Channel3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    /* DMA1_Channel4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}