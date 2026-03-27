#include "main.h"

#include "Peleng.hpp"
#include "cordic.h"
#include "UartTelemetry.hpp"

Peleng peleng;

int main(void)
{
    InitHw();
    peleng.Init();

    while (1)
    {
        UartTelemetryProcess();
        peleng.Process();

        DelayMeasurements delays;
        if (peleng.TryGetLatestDelays(&delays))
        {
            (void)SendDelayTelemetryUart(delays);
        }
    }
}

inline void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) { peleng.DmaTransferCompleteCallback(hadc); }

inline void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) { peleng.DmaHalfTransferCallback(hadc); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15)
    {
        HAL_IncTick();
    }
}
