#include "main.h"

#include "Hw.h"

#include "Peleng.hpp"
#include "UartTelemetry.hpp"
#include "cordic.h"

Peleng g_peleng;

extern "C"
{
    volatile uint32_t g_core_clock_hz                          = 0U;
    volatile uint32_t g_adc_sample_period_cycles               = 0U;
    volatile uint32_t g_signal_block_period_cycles             = 0U;
    volatile uint32_t g_main_loop_cycles_last                  = 0U;
    volatile uint32_t g_main_loop_cycles_max                   = 0U;
    volatile uint32_t g_main_loop_sample_period_exceed_count   = 0U;
    volatile uint32_t g_peleng_process_cycles_last             = 0U;
    volatile uint32_t g_peleng_process_cycles_max              = 0U;
    volatile uint32_t g_peleng_process_block_period_miss_count = 0U;
}

namespace {
void EnableCycleCounter()
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT       = 0U;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t ReadCycleCounter()
{
    return DWT->CYCCNT;
}

void RecordCycles(volatile uint32_t* last, volatile uint32_t* maximum, uint32_t cycles)
{
    *last = cycles;
    if (cycles > *maximum) {
        *maximum = cycles;
    }
}

void InitTimingBudgets()
{
    g_core_clock_hz = HAL_RCC_GetHCLKFreq();

    const auto sample_rate_hz = static_cast<uint32_t>(SAMPLE_RATE_HZ);
    if (sample_rate_hz == 0U) {
        return;
    }

    g_adc_sample_period_cycles   = g_core_clock_hz / sample_rate_hz;
    g_signal_block_period_cycles = g_adc_sample_period_cycles * SIGNAL_BLOCK_SIZE;
}
} // namespace

int main(void)
{
    InitHw();
    EnableCycleCounter();
    InitTimingBudgets();
    SetPelengDebugSource(&g_peleng);
    event << "Peleng Init";
    g_peleng.Init();
    event << "Signal acquisition started";

    uint32_t delay_frame_decimator          = 0U;
    constexpr uint32_t kDelayTxEveryNFrames = 16U;

    while (1) {
        const uint32_t loop_start_cycles = ReadCycleCounter();
        UartTelemetryProcess();

        const uint32_t process_start_cycles = ReadCycleCounter();
        const bool processed_signal_block   = g_peleng.Process();
        const uint32_t process_cycles       = ReadCycleCounter() - process_start_cycles;
        if (processed_signal_block) {
            RecordCycles(&g_peleng_process_cycles_last, &g_peleng_process_cycles_max, process_cycles);
            if ((g_signal_block_period_cycles > 0U) && (process_cycles > g_signal_block_period_cycles)) {
                ++g_peleng_process_block_period_miss_count;
            }
        }

        DelayMeasurements delays;
        if (g_peleng.TryGetLatestDelays(&delays)) {
            if ((++delay_frame_decimator % kDelayTxEveryNFrames) == 0U) {
                (void)SendDelayTelemetryUart(delays);
            }
        }

        const uint32_t loop_cycles = ReadCycleCounter() - loop_start_cycles;
        RecordCycles(&g_main_loop_cycles_last, &g_main_loop_cycles_max, loop_cycles);
        if ((g_adc_sample_period_cycles > 0U) && (loop_cycles > g_adc_sample_period_cycles)) {
            ++g_main_loop_sample_period_exceed_count;
        }
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM15) {
        HAL_IncTick();
    }
}
