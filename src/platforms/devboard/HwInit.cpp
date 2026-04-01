#include "HwInit.h"

#include "Pins.h"

namespace
{
void CheckHalStatus(HAL_StatusTypeDef status)
{
    if (status != HAL_OK)
    {
        Error_Handler();
    }
}

void ConfigureAdcBase(ADC_HandleTypeDef *hadc, ADC_TypeDef *instance)
{
    hadc->Instance = instance;
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
    hadc->Init.DMAContinuousRequests = ENABLE;
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc->Init.OversamplingMode = DISABLE;

    CheckHalStatus(HAL_ADC_Init(hadc));
}

void ConfigureAdcIndependentMode(ADC_HandleTypeDef *hadc)
{
    ADC_MultiModeTypeDef multimode = {0};
    multimode.Mode = ADC_MODE_INDEPENDENT;
    CheckHalStatus(HAL_ADCEx_MultiModeConfigChannel(hadc, &multimode));
}

void ConfigureAdcWatchdog(ADC_HandleTypeDef *hadc, uint32_t channel)
{
    ADC_AnalogWDGConfTypeDef watchdog_config = {0};

    watchdog_config.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
    watchdog_config.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
    watchdog_config.Channel = channel;
    watchdog_config.ITMode = ENABLE;
    watchdog_config.HighThreshold = 3710;
    watchdog_config.LowThreshold = 0;
    watchdog_config.FilteringConfig = ADC_AWD_FILTERING_NONE;

    CheckHalStatus(HAL_ADC_AnalogWDGConfig(hadc, &watchdog_config));
}

void ConfigureAdcRegularChannel(ADC_HandleTypeDef *hadc, uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {0};

    config.Channel = channel;
    config.Rank = ADC_REGULAR_RANK_1;
    config.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    config.SingleDiff = ADC_SINGLE_ENDED;
    config.OffsetNumber = ADC_OFFSET_NONE;
    config.Offset = 0;

    CheckHalStatus(HAL_ADC_ConfigChannel(hadc, &config));
}

void ConfigureDac(DAC_HandleTypeDef *hdac, DAC_TypeDef *instance)
{
    DAC_ChannelConfTypeDef config = {0};

    hdac->Instance = instance;
    CheckHalStatus(HAL_DAC_Init(hdac));

    config.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
    config.DAC_DMADoubleDataMode = DISABLE;
    config.DAC_SignedFormat = DISABLE;
    config.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    config.DAC_Trigger = DAC_TRIGGER_NONE;
    config.DAC_Trigger2 = DAC_TRIGGER_NONE;
    config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    config.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
    config.DAC_UserTrimming = DAC_TRIMMING_FACTORY;

    CheckHalStatus(HAL_DAC_ConfigChannel(hdac, &config, DAC_CHANNEL_1));
    CheckHalStatus(HAL_DAC_ConfigChannel(hdac, &config, DAC_CHANNEL_2));
}

void ConfigureOpampPga(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *instance)
{
    hopamp->Instance = instance;
    hopamp->Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
    hopamp->Init.Mode = OPAMP_PGA_MODE;
    hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
    hopamp->Init.InternalOutput = ENABLE;
    hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
    hopamp->Init.PgaGain = OPAMP_PGA_GAIN_64_OR_MINUS_63;
    hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;

    CheckHalStatus(HAL_OPAMP_Init(hopamp));
}

void ConfigureBaseTimer(TIM_HandleTypeDef *htim, TIM_TypeDef *instance, uint32_t prescaler, uint32_t period,
                        uint32_t master_trigger)
{
    TIM_MasterConfigTypeDef master_config = {0};

    htim->Instance = instance;
    htim->Init.Prescaler = prescaler;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = period;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    CheckHalStatus(HAL_TIM_Base_Init(htim));

    master_config.MasterOutputTrigger = master_trigger;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    CheckHalStatus(HAL_TIMEx_MasterConfigSynchronization(htim, &master_config));
}
} // namespace

void Adc1Init(ADC_HandleTypeDef *hadc)
{
    ConfigureAdcBase(hadc, ADC1);
    ConfigureAdcIndependentMode(hadc);
    ConfigureAdcWatchdog(hadc, ADC_CHANNEL_VOPAMP1);
    ConfigureAdcRegularChannel(hadc, ADC_CHANNEL_VOPAMP1);
}

void Adc2Init(ADC_HandleTypeDef *hadc)
{
    ConfigureAdcBase(hadc, ADC2);
    ConfigureAdcRegularChannel(hadc, ADC_CHANNEL_VOPAMP3_ADC2);
}

void Adc3Init(ADC_HandleTypeDef *hadc)
{
    ConfigureAdcBase(hadc, ADC3);
    ConfigureAdcIndependentMode(hadc);
    ConfigureAdcRegularChannel(hadc, ADC_CHANNEL_VOPAMP3_ADC3);
}

void Adc5Init(ADC_HandleTypeDef *hadc)
{
    ConfigureAdcBase(hadc, ADC5);
    ConfigureAdcRegularChannel(hadc, ADC_CHANNEL_VOPAMP5);
}

void Dac3Init(DAC_HandleTypeDef *hdac) { ConfigureDac(hdac, DAC3); }

void Dac4Init(DAC_HandleTypeDef *hdac) { ConfigureDac(hdac, DAC4); }

void Opamp1Init(OPAMP_HandleTypeDef *hopamp) { ConfigureOpampPga(hopamp, OPAMP1); }

void Opamp3Init(OPAMP_HandleTypeDef *hopamp) { ConfigureOpampPga(hopamp, OPAMP3); }

void Opamp4Init(OPAMP_HandleTypeDef *hopamp) { ConfigureOpampPga(hopamp, OPAMP4); }

void Opamp5Init(OPAMP_HandleTypeDef *hopamp) { ConfigureOpampPga(hopamp, OPAMP5); }

void Tim6Init(TIM_HandleTypeDef *htim) { ConfigureBaseTimer(htim, TIM6, 169U, 3U, TIM_TRGO_UPDATE); }

void Tim7Init(TIM_HandleTypeDef *htim) { ConfigureBaseTimer(htim, TIM7, 16999U, 4U, TIM_TRGO_RESET); }

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

    CheckHalStatus(HAL_UART_Init(huart));
    CheckHalStatus(HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8));
    CheckHalStatus(HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8));
    CheckHalStatus(HAL_UARTEx_DisableFifoMode(huart));
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
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
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
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif /* USE_FULL_ASSERT */
