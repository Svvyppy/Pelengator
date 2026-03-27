#include "main.h"

#include "Peleng.hpp"
#include "cordic.h"
#include "platform/UartTelemetry.hpp"

Peleng peleng;

int main(void)
{
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

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif /* USE_FULL_ASSERT */
