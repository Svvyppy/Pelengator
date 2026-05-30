#include "main.h"

#include "Hw.h"
#include "Peleng.hpp"
#include "UartTelemetry.hpp"
#include "cordic.h"

Peleng g_peleng;

int main(void)
{
    InitHw();
    SetPelengDebugSource(&g_peleng);
    event << "Peleng Init";
    g_peleng.Init();
    event << "Signal acquisition started";

    uint32_t delay_frame_decimator          = 0U;
    constexpr uint32_t kDelayTxEveryNFrames = 16U;

    while (1) {
        UartTelemetryProcess();
        g_peleng.Process();

        DelayMeasurements delays;
        if (g_peleng.TryGetLatestDelays(&delays)) {
            if ((++delay_frame_decimator % kDelayTxEveryNFrames) == 0U) {
                (void)SendDelayTelemetryUart(delays);
            }
        }
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM15) {
        HAL_IncTick();
    }
}
