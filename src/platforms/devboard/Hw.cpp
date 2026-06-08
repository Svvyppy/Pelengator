#include "Hw.h"

#include "HwInit.h"
#include "cordic.h"

namespace hydrv::hw {

static SignalRecorder* activeRecorder = nullptr;
static UartTelemetry* activeTelemetry = nullptr;
static TIM_HandleTypeDef halTickTimer{};

uint32_t enterCriticalSection() noexcept
{
    const uint32_t interruptState = __get_PRIMASK();
    __disable_irq();
    return interruptState;
}

void exitCriticalSection(uint32_t interruptState) noexcept
{
    __set_PRIMASK(interruptState);
}

void initialize(SignalRecorder& recorder, UartTelemetry& telemetry) noexcept
{
    SCB->VTOR = FLASH_BASE;
    __DSB();
    __ISB();

    HAL_Init();
    configureSystemClock();

    initializeGpio();
    initializeDma();
    initializeSamplingTimer(recorder.samplingTimerHandle());
    initializeServiceTimer(recorder.serviceTimerHandle());
    initializeUart1(telemetry.uartHandle());
    initializeDac4(recorder.dac4Handle());
    initializeOpamp4(recorder.opamp4Handle());
    initializeAdc1(recorder.adc1Handle());
    initializeAdc2(recorder.adc2Handle());
    initializeAdc4(recorder.adc4Handle());
    initializeAdc5(recorder.adc5Handle());
    MX_CORDIC_Init();

    activeRecorder  = &recorder;
    activeTelemetry = &telemetry;
}

[[noreturn]] void stop() noexcept
{
    __disable_irq();
    while (true) {
    }
}

extern "C" TIM_HandleTypeDef* GetHalTickTimer(void)
{
    return &halTickTimer;
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* completedAdc)
{
    activeRecorder->onTransferComplete(completedAdc);
}

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* completedAdc)
{
    activeRecorder->onHalfTransferComplete(completedAdc);
}

extern "C" void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(activeRecorder->adc1Handle()->DMA_Handle);
}

extern "C" void DMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(activeRecorder->adc5Handle()->DMA_Handle);
}

extern "C" void DMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(activeRecorder->adc2Handle()->DMA_Handle);
}

extern "C" void DMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(activeRecorder->adc4Handle()->DMA_Handle);
}

extern "C" void TIM1_BRK_TIM15_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&halTickTimer);
}

extern "C" void TIM6_DAC_IRQHandler(void)
{}

extern "C" void TIM7_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(activeRecorder->serviceTimerHandle());
    HAL_DAC_IRQHandler(activeRecorder->dac4Handle());
}

extern "C" void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(activeTelemetry->uartHandle());
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* timer)
{
    if (timer == &halTickTimer) {
        HAL_IncTick();
    }
}

} // namespace hydrv::hw
