#include "main.h"

#include "Peleng.hpp"
#include "UartTelemetry.hpp"
#include "cordic.h"

Peleng g_peleng;

int main(void)
{
    InitHw();
    g_peleng.Init();

    while (1)
    {
        UartTelemetryProcess();
        g_peleng.Process();

        DelayMeasurements delays;
        if (g_peleng.TryGetLatestDelays(&delays))
        {
            (void)SendDelayTelemetryUart(delays);
        }
    }
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) { g_peleng.DmaTransferCompleteCallback(hadc); }

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) { g_peleng.DmaHalfTransferCallback(hadc); }

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15)
    {
        HAL_IncTick();
    }
}
